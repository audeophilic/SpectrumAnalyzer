#include <Adafruit_NeoPixel.h>
#include "FFT.h"
#include "SaturatingBuffer.h"
#include "ColorCycle.h"

#define STRIP_PIN 7

#define NUM_STRIPS 1
#define LED_PER_STRIP 108

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
Adafruit_NeoPixel strip(LED_PER_STRIP, STRIP_PIN);


//TO DO: Employ an IIRF function like David implemented to deal with the avg vals?
int cycles = 0;

// LED Control Variables
uint8_t brightness = 100;


void setup()
{
    Serial.begin(9600);
    strip.begin();
    strip.show();
    pinMode(LED_BUILTIN, OUTPUT);
}

//Read at a rate of 50kHz - period is 
void loop()
{
    // Light each LED in strip according to its position
    for (int l = 0; l < LED_PER_STRIP; ++l)
    {
        // 765 is total possible 1-or-2-tone colors in ColorCycle
        uint32_t pixelColor = adjustBrightness(ColorCycle[(cycles + 4*l) % 765], brightness);
        strip.setPixelColor(l, pixelColor);
    }
    strip.show();
    if (cycles == 765) cycles = 0;
    else ++cycles;
    delay(10);
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

