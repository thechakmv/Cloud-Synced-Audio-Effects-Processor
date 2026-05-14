#include "spotify.h"

#include <ArduinoJson.h>
#include <WiFiEspAT.h>

#include "config.h"
#include "preset_manager.h"

namespace spotify {
namespace {

WiFiSSLClient s_tls;
char     s_access_token[400]  = {0};
char     s_last_track_id[64]  = {0};
uint32_t s_last_poll_ms       = 0;

// Base64 for HTTP Basic auth on the token refresh.
void b64_encode(const char *in, char *out, size_t cap) {
    static const char A[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t n = strlen(in), o = 0;
    for (size_t i = 0; i < n; i += 3) {
        uint32_t v = ((uint32_t)(uint8_t)in[i]) << 16;
        if (i + 1 < n) v |= ((uint32_t)(uint8_t)in[i+1]) << 8;
        if (i + 2 < n) v |= ((uint32_t)(uint8_t)in[i+2]);
        if (o + 4 >= cap) break;
        out[o++] = A[(v >> 18) & 0x3F];
        out[o++] = A[(v >> 12) & 0x3F];
        out[o++] = (i + 1 < n) ? A[(v >> 6) & 0x3F] : '=';
        out[o++] = (i + 2 < n) ? A[v & 0x3F] : '=';
    }
    out[o] = 0;
}

// Open TLS, write a pre-built request, read the response. Returns HTTP
// status code, or -1 on transport error. Body (after CRLFCRLF) goes into
// `body`, null-terminated.
int http_request(const char *host, const char *request,
                 char *body, size_t body_cap) {
    s_tls.stop();
    if (!s_tls.connectSSL(host, 443)) return -1;
    s_tls.write((const uint8_t *)request, strlen(request));

    int status = -1, blank_run = 0;
    size_t blen = 0, sli = 0;
    bool in_body = false;
    char status_line[64];
    uint32_t deadline = millis() + 8000;

    while (millis() < deadline) {
        while (s_tls.available()) {
            int c = s_tls.read();
            if (c < 0) break;
            if (!in_body) {
                if (status == -1 && c != '\r' && c != '\n' &&
                    sli < sizeof(status_line) - 1) {
                    status_line[sli++] = (char)c;
                }
                if (c == '\n') {
                    if (status == -1) {
                        status_line[sli] = 0;
                        char *p = strchr(status_line, ' ');
                        if (p) status = atoi(p + 1);
                    }
                    if (++blank_run >= 2) in_body = true;
                } else if (c != '\r') {
                    blank_run = 0;
                }
            } else if (blen + 1 < body_cap) {
                body[blen++] = (char)c;
            }
        }
        if (!s_tls.connected() && !s_tls.available()) break;
        delay(1);
    }
    body[blen] = 0;
    return status;
}

bool refresh_token() {
    char pair[256];
    snprintf(pair, sizeof(pair), "%s:%s",
             SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET);
    char basic[400];
    b64_encode(pair, basic, sizeof(basic));

    char body[300];
    int blen = snprintf(body, sizeof(body),
        "grant_type=refresh_token&refresh_token=%s", SPOTIFY_REFRESH_TOKEN);

    char request[1024];
    snprintf(request, sizeof(request),
        "POST /api/token HTTP/1.1\r\n"
        "Host: accounts.spotify.com\r\n"
        "Authorization: Basic %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n%s",
        basic, blen, body);

    char resp[1500] = {0};
    int code = http_request("accounts.spotify.com", request, resp, sizeof(resp));
    if (code != 200) {
        Serial.printf("[spotify] token refresh failed http=%d\n", code);
        return false;
    }

    JsonDocument doc;
    if (deserializeJson(doc, resp)) return false;
    const char *tok = doc["access_token"] | "";
    strncpy(s_access_token, tok, sizeof(s_access_token) - 1);
    s_access_token[sizeof(s_access_token) - 1] = 0;
    return s_access_token[0] != 0;
}

// Bearer GET against api.spotify.com. Returns true iff response was 200.
// On 401, clears the cached access token so the next call will mint a new
// one — the failed call returns false and the caller just skips this poll.
bool api_get(const char *path, char *body, size_t body_cap) {
    if (!s_access_token[0] && !refresh_token()) return false;

    char request[512];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: api.spotify.com\r\n"
        "Authorization: Bearer %s\r\n"
        "Connection: close\r\n\r\n",
        path, s_access_token);

    int code = http_request("api.spotify.com", request, body, body_cap);
    if (code == 401) s_access_token[0] = 0;
    return code == 200;
}

}  // namespace

bool begin() {
    AT_SERIAL.begin(AT_BAUD);
    WiFi.init(AT_SERIAL);
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("[spotify] ESP-AT modem not responding on Serial1");
        return false;
    }
    Serial.printf("[spotify] joining '%s'\n", WIFI_SSID);
    if (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
        Serial.println("[spotify] Wi-Fi failed");
        return false;
    }
    IPAddress ip = WiFi.localIP();
    Serial.printf("[spotify] IP %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
    return refresh_token();
}

void poll() {
    uint32_t now = millis();
    if (now - s_last_poll_ms < SPOTIFY_POLL_PERIOD_MS) return;
    s_last_poll_ms = now;

    static char resp[6000];
    if (!api_get("/v1/me/player/currently-playing", resp, sizeof(resp))) return;

    JsonDocument doc;
    if (deserializeJson(doc, resp)) return;

    const char *tid = doc["item"]["id"] | "";
    const char *aid = doc["item"]["artists"][0]["id"] | "";
    if (!*tid || !*aid) return;
    if (strcmp(tid, s_last_track_id) == 0) return;   // same song

    char path[80];
    snprintf(path, sizeof(path), "/v1/artists/%s", aid);
    if (!api_get(path, resp, sizeof(resp))) return;
    if (deserializeJson(doc, resp)) return;

    char genres[160] = {0};
    size_t used = 0;
    for (JsonVariant g : doc["genres"].as<JsonArray>()) {
        const char *s = g.as<const char *>();
        if (!s) continue;
        int w = snprintf(genres + used, sizeof(genres) - used,
                         "%s%s", used ? "," : "", s);
        if (w < 0 || (size_t)w >= sizeof(genres) - used) break;
        used += w;
    }

    Serial.printf("[spotify] track=%s genres=[%s]\n", tid, genres);
    strncpy(s_last_track_id, tid, sizeof(s_last_track_id) - 1);
    s_last_track_id[sizeof(s_last_track_id) - 1] = 0;
    presets::on_genre(genres);
}

const char *current_track_id() { return s_last_track_id; }

}  // namespace spotify_client
