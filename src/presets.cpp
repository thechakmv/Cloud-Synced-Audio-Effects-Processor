#include "presets.h"

#include <ctype.h>
#include <string.h>

namespace{
    struct Entry{
        const char *match;
        DspParams params;
    };

const Entry kTable[] = {
    // --- electronic: more bass, sparkle on top ---
    { "deep house",     { +4, 0, +2 } },
    { "tech house",     { +3, 0, +3 } },
    { "drum and bass",  { +5, 0, +4 } },
    { "dubstep",        { +6, 0, +3 } },
    { "trance",         { +3, 0, +4 } },
    { "edm",            { +4, 0, +3 } },
    { "house",          { +3, 0, +2 } },
    { "techno",         { +4, 0, +3 } },

    // --- metal / rock: neutral lows, bright highs ---
    { "death metal",    { +2, 0, +2 } },
    { "metal",          { +3, 0, +2 } },
    { "punk",           { +1, 0, +3 } },
    { "rock",           { +2, 0, +2 } },

    // --- hip-hop: heavy bass, modest top ---
    { "hip hop",        { +5, 0, +2 } },
    { "hip-hop",        { +5, 0, +2 } },
    { "rap",            { +5, 0, +2 } },
    { "trap",           { +6, 0, +2 } },

    // --- soul / r&b / funk: warm and present ---
    { "r&b",            { +3, 0, +2 } },
    { "soul",           { +2, 0, +2 } },
    { "funk",           { +4, 0, +2 } },

    // --- jazz / classical / ambient: neutral, airy ---
    { "jazz",           {  0, 0, +2 } },
    { "classical",      { -1, 0, +3 } },
    { "ambient",        {  0, 0, +3 } },
    { "lo-fi",          { +2, 0, -2 } },

    // --- country / folk / indie / pop ---
    { "country",        {  0, 0, +2 } },
    { "folk",           { -1, 0, +2 } },
    { "indie",          { +1, 0, +2 } },
    { "pop",            { +2, 0, +3 } },

    // catch-all (last) — flat
    { "",                {  0,  0,  0 } },
};

const size_t kTableN = sizeof(kTable) / sizeof(kTable[0]);
}

const DspParams &preset_lookup(const char *genre_csv) {
    if (!genre_csv) return kTable[kTableN - 1].params; //returns the default if it's null

    char lower[192];
    size_t n = strnlen(genre_csv, sizeof(lower) - 1);
    for (size_t i = 0; i < n; i++) {    
        lower[i] = (char)tolower((unsigned char)genre_csv[i]);
    }
    lower[n] = 0;

    for (size_t i = 0; i < kTableN; i++) { //Check all the genres in the table
        if (kTable[i].match[0] == 0) continue;  // skip catch-all on primary pass
        if (strstr(lower, kTable[i].match)) {
            return kTable[i].params;    //return reference to the  preset
        }
    }
    return kTable[kTableN - 1].params;  //If no match return the reference to the default preset
}
