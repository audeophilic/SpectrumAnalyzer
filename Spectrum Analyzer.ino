#include "FFT.h"
#include <ADC.h>
#include <Adafruit_NeoPixel.h>


#define STRIP0_PIN 7
#define STRIP1_PIN 8
#define STRIP2_PIN 9
#define STRIP3_PIN 10
#define STRIP4_PIN 11

#define STRIP_LENGTH 12

#define AUDIO_PIN A0

#define NUM_SAMPLES 1024

#define SAMPLE_RATE 50000

const unsigned long sample_period = 1000000 / SAMPLE_RATE;
const int fft_m = log(NUM_SAMPLES) / log(2);

// Lighting
Adafruit_NeoPixel strip[5] =
{ 
    Adafruit_NeoPixel(STRIP_LENGTH, STRIP0_PIN), 
    Adafruit_NeoPixel(STRIP_LENGTH, STRIP1_PIN),
    Adafruit_NeoPixel(STRIP_LENGTH, STRIP2_PIN),
    Adafruit_NeoPixel(STRIP_LENGTH, STRIP3_PIN),
    Adafruit_NeoPixel(STRIP_LENGTH, STRIP4_PIN)
};

//sampling operation variables
short vReal[NUM_SAMPLES], vImag[NUM_SAMPLES];
double interval = 0;
int samplesTaken = 0;
unsigned long lastTime = 0;


void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    lastTime = micros();
}

//Read at a rate of 50kHz - period is 
void loop()
{
    // Internal while loop to decrease jitter
    while (1) {

        //Take 256 samples to perform fft on
        if (samplesTaken < NUM_SAMPLES)
        {
            if (samplesTaken == 0)
            {
                interval = micros();
                digitalWrite(LED_BUILTIN, LOW);
            }

            //Only take samples every two microseconds
            unsigned long period = micros() - lastTime;
            if (period >= sample_period)
            {
                vReal[samplesTaken] = analogRead(AUDIO_PIN) >> 512;
                lastTime += period;
                samplesTaken++;
            }
        }
        //Reset samples taken, do fft, update display
        else
        {
            for (int i = 0; i < NUM_SAMPLES; ++i) vImag[i] = 0;

            digitalWrite(LED_BUILTIN, HIGH);
            interval = micros() - interval; //How many us did last N samples take?
            //Frequency = nsamples/(us*10^6))
            double frequency = NUM_SAMPLES * 1000000 / interval;
            //Serial.println(frequency);
            double freqInterval = frequency / NUM_SAMPLES;

            fix_fft(vReal, vImag, fft_m, 0);


            const int ind = 5;
            Serial.print(ind * freqInterval);
            Serial.print("Hz: ");

            short at = vImag[ind];
            Serial.println(at > 0 ? at : 0 - at);

            //delay(1000);

            samplesTaken = 0;
            lastTime = micros();
        }
    }
}