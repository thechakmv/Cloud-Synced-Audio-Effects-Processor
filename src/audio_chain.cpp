#include <Audio.h>
#include "audio_chain.h"

#include<Wire.h>
#include <SPI.h>
#include <math.h>

static constexpr float kLowFreqHz   = 250.0f;
static constexpr float kMidFreqHz = 1500.0f;
static constexpr float kHighFreqHz = 3000.0f;

static constexpr int   kLowShelfStage  = 0;  // stage index inside the biquad obj
static constexpr int   kHighShelfStage = 1;

static AudioInputI2S            audioInput; //Audio input from Audio Shield line in
static AudioFilterBiquad        eqa_L, eqa_R; //
static AudioOutputI2S           audioOutput;  //Output to Audio Shield Line out

/*AudioConnection(
sourceObject,               Audio object creating the signal
outputchannel,              The output pin on the object to take the signal from
destination,                The audio object that recieves the signal
destinationInputchannel     Which input pin on the object to send the signal to
);
*/
static AudioConnection c01 (audioInput, 0, eqa_L,       0);
static AudioConnection c02 (audioInput, 1, eqa_R,       0); //left channel
static AudioConnection c03 (eqa_L,      0, audioOutput, 0);
static AudioConnection c04 (eqa_R,      0, audioOutput, 1);

static AudioControlSGTL5000 audioShield; //Audio Shield control object

static DspParams currentParams = {0.0f, 0.0f, 0.0f}; //Current DSP parameters (in dB)

static inline float db_to_mult(float db){
    return powf(10.0f, db / 20.0f);
}

static void push(const DspParams &p){
    const float low_mult = db_to_mult(p.low_db);
    //const float mid_mult = db_to_mult(p.mid_db);
    const float high_mult = db_to_mult(p.high_db);

    eqa_L.setLowShelf (kLowShelfStage, kLowFreqHz, low_mult);
    eqa_L.setHighShelf (kHighShelfStage, kHighFreqHz, high_mult);
    eqa_R.setLowShelf (kLowShelfStage, kLowFreqHz, low_mult);
    eqa_R.setHighShelf (kHighShelfStage, kHighFreqHz, high_mult);

}

//Public API

namespace audio_chain {
    bool begin(){
        AudioMemory(12); //Allocates memory for audio processing

        if (!audioShield.enable()){
            Serial.println("Failed to enable audio shield");
            return false;
        }
        audioShield.inputSelect(AUDIO_INPUT_LINEIN); //Set input
        audioShield.volume(0.7); //Perhaps hook up a potentiometer to control
        audioShield.lineInLevel(5);
        audioShield.lineOutLevel(13);

        push(currentParams);
        Serial.println("Audio chain initialized");
        return true;
    }

    void apply(const DspParams &p){
        currentParams = p;
        push(currentParams);
    }

    const DspParams &current() { 
        return currentParams;
    }
}