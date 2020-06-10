#include <Adafruit_NeoPixel.h>
#include "FFT.h"
#include "SaturatingBuffer.h"

#define STRIP0_PIN 7
#define STRIP1_PIN 8
#define STRIP2_PIN 9
#define STRIP3_PIN 10
#define STRIP4_PIN 11

#define NUM_STRIPS 5
#define LED_PER_STRIP 12

#define AUDIO_PIN A0

#define NUM_SAMPLES 1024
#define SAMPLE_RATE 50000   // per second

//For light sensitivity
#define MIN_VU 20

#ifdef abs 
    #undef abs
    #define abs(x) (x >= 0 ? x : -x)
#endif

#ifdef max 
    #undef max
    #define max(x, y) (x > y ? x : y)
#endif

const unsigned long sample_period = 1000000 / SAMPLE_RATE;  // The sample period in microseconds
const int fft_m = log(NUM_SAMPLES) / log(2);                // argument "m" to pass into fix_fft()
const double freqInterval = SAMPLE_RATE / NUM_SAMPLES;      // The interval of each bin's frequency in vImag

// Lighting
Adafruit_NeoPixel strip[5] =
{ 
    Adafruit_NeoPixel(LED_PER_STRIP, STRIP0_PIN), 
    Adafruit_NeoPixel(LED_PER_STRIP, STRIP1_PIN),
    Adafruit_NeoPixel(LED_PER_STRIP, STRIP2_PIN),
    Adafruit_NeoPixel(LED_PER_STRIP, STRIP3_PIN),
    Adafruit_NeoPixel(LED_PER_STRIP, STRIP4_PIN)
};

// Keep track of maximum values for all FFTs
SaturatingBuffer<short> maxqueue;

// sampling operation variables
short vReal[NUM_SAMPLES], vImag[NUM_SAMPLES];

// LED Control Variables
int cycles = 0;
uint8_t brightness = 100;


void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 5; ++i)
    {
        strip[i].begin();
        strip[i].show();
    }
    pinMode(LED_BUILTIN, OUTPUT);

    maxqueue.saturate(LED_PER_STRIP);
}

//Read at a rate of 50kHz - period is 
void loop()
{
    int begin = millis();

    takeSamples();

    fix_fft(vReal, vImag, fft_m, 0);



    //Get the average of each band

    //TO DO: Translate frequency interval into bands for analysis
    short maxVal = 0;
    short val[NUM_STRIPS];

    //Define the bands
    val[0] = sumBand(0, 5);
    val[1] = sumBand(5, 50);
    val[2] = sumBand(50, 120);
    val[3] = sumBand(120, 200);
    val[4] = sumBand(200, 300);
    for (int i = 0; i < NUM_STRIPS; ++i)
    {
        if (val[i] > maxVal) maxVal = val[i];
    }
    maxqueue.push(max(maxVal, MIN_VU));


    // Set the lights
    for (int s = 0; s < NUM_STRIPS; ++s)
    {
        // Boundary factor - how many of total number of LED's get lit?
        int bound = weightAgainstMax(val[s], maxqueue.max(), LED_PER_STRIP + 1);

        // Light each LED in strip according to its position
        for (int l = 0; l < LED_PER_STRIP; ++l)
        {
            // 765 is total possible 1-or-2-tone colors in ColorCycle
            uint32_t pixelColor = adjustBrightness(ColorCycle[(cycles + 60 * s + 25 * l) % 765], brightness);
            if (l < bound - 1) strip[s].setPixelColor(l, pixelColor);
            else strip[s].setPixelColor(l, 0);
        }
        strip[s].show();
    }

    cycles++;
    if (cycles == 765) cycles = 0;


    int end = millis() - begin;
    //Serial.println(end);
    //Serial.println();
    delay(10);
}

// Read samples into vReal and set all vImag to 0
void takeSamples() {
    int samplesTaken = 0;
    unsigned long lastTime = micros();
    while (samplesTaken < NUM_SAMPLES)
    {
        //Only take samples every twenty microseconds
        unsigned long period = micros() - lastTime;
        if (period >= sample_period)
        {
            vReal[samplesTaken] = analogRead(AUDIO_PIN) >> 512;
            vImag[samplesTaken] = 0;
            lastTime += period;
            //Serial.println(samplesTaken);
            samplesTaken++;
        }
    }
}

// Sum all vImag bins from lower_bin until upper_bin
short sumBand(int lower_bin, int upper_bin)
{
    short sum = 0;
    for (int i = lower_bin; i < upper_bin; ++i)
    {
        sum = sum + abs(vImag[i]);
        //Serial.println("SUM FROM FN: " + String(sum));
    }
    return sum;
}

// Mean average all vImag bins from lower_bin until upper_bin
short avgBand(int lower_bin, int upper_bin)
{
    return sumBand(lower_bin, upper_bin) / (upper_bin - lower_bin);
}

// val/maxVal as a percentage, weighted against maxout
int weightAgainstMax(int val, int maxval, int maxout)
{
    return val * maxout / maxval;
}


// Adjust color channels in color from ColorCycle array against brightness (max 255)
uint32_t adjustBrightness(const uint32_t color, const uint8_t brightness)
{
    if (brightness == 255) return color;

    // Extract color channels, shift, and weight
    uint32_t red = ((color >> 16) & 0xff) * brightness / 255;
    uint32_t green = ((color >> 8) & 0xff) * brightness / 255;
    uint32_t blue = (color & 0xff) * brightness / 255;

    return (red << 16) + (green << 8) + (blue);
}

