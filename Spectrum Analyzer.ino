#include <Adafruit_NeoPixel.h>
#include "FFT.h"
#include "SaturatingBuffer.h"
#include "LightGrid.h"

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
#define MIN_VU 50

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

//Keep track of maximum values for each strip band, each holding 100 items
SaturatingBuffer<short> maxqueue[NUM_STRIPS];

//sampling operation variables
short vReal[NUM_SAMPLES], vImag[NUM_SAMPLES];

//LED Control Variables
int cycles = 0;
uint8_t brightness = 50;


void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 5; ++i)
    {
        maxqueue[i].saturate(LED_PER_STRIP);
        strip[i].begin();
        strip[i].show();
    }
    pinMode(LED_BUILTIN, OUTPUT);
}

//Read at a rate of 50kHz - period is 
void loop()
{
    int begin = millis();

    takeSamples();

    fix_fft(vReal, vImag, fft_m, 0);

    //Serial.println(vImag[5]);

    //Get the sum of each band
    short sum[NUM_STRIPS];
    for (int i = 0; i < NUM_STRIPS; ++i)
    {
        sum[i] = sumBand(i * 63, (i + 1) * 63);
        //Serial.println("Sum: " + String(sum[i]));
        maxqueue[i].push(max(sum[i], MIN_VU));
        //Serial.println(maxqueue->max());
        //delay(1000);
    }
    //Serial.println();

    for (int s = 0; s < NUM_STRIPS; ++s)
    {
        int bound = weightAgainstMax(sum[s], maxqueue[s].max(), LED_PER_STRIP) - 1;
        for (int l = 0; l < LED_PER_STRIP; ++l)
        {
            uint32_t pixelColor = adjustBrightness(ColorCycle[(cycles + 60 * s + 25 * l) % 765], brightness);
            if (l < bound) strip[s].setPixelColor(l, pixelColor);
            else strip[s].setPixelColor(l, 0);
        }
        strip[s].show();
    }

    cycles++;
    if (cycles == 765) cycles = 0;


    int end = millis() - begin;
    Serial.println(end);
    Serial.println();

}

//strip[s].setPixelColor(l, ColorCycle[(i + 60*s + 25*l) % 765]);

//Read samples into vReal and set all vImag to 0
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

short sumBand(int lower_bin, int upper_bin)
{
    short sum = 0;
    for (int i = 0; i < upper_bin; ++i)
    {
        sum = sum + abs(vImag[i]);
        //Serial.println("SUM FROM FN: " + String(sum));
    }
    return sum;
}

int weightAgainstMax(int val, int maxval, int maxout)
{
    return val * maxout / maxval;
}

uint32_t adjustBrightness(const uint32_t color, const uint8_t brightness)
{
    if (brightness == 255) return color;

    // Extract color channels, shift, and weight
    uint32_t red = ((color >> 16) & 0xff) * brightness / 255;
    uint32_t green = ((color >> 8) & 0xff) * brightness / 255;
    uint32_t blue = (color & 0xff) * brightness / 255;

    return (red << 16) + (green << 8) + (blue);
}