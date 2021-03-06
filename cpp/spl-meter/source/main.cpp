/*
 *   Copyright (c) 2020 Mykola Dvornik
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "MicroBit.h"
#include "LevelDetectorSPL.h"
#include "neopixel.h"

MicroBit uBit;

// Adafruit NeoPixel FeatherWing - 4x8 RGB LED
#define NEOPIXEL_BYTES_PER_PIXEL 3  // GRB format
#define NEOPIXEL_PIXEL_SIZEX 8
#define NEOPIXEL_PIXEL_SIZEY 4
#define NEOPIXEL_ROW_0 (0 * NEOPIXEL_PIXEL_SIZEX * NEOPIXEL_BYTES_PER_PIXEL)
#define NEOPIXEL_ROW_1 (1 * NEOPIXEL_PIXEL_SIZEX * NEOPIXEL_BYTES_PER_PIXEL)
#define NEOPIXEL_ROW_2 (2 * NEOPIXEL_PIXEL_SIZEX * NEOPIXEL_BYTES_PER_PIXEL)
#define NEOPIXEL_ROW_3 (3 * NEOPIXEL_PIXEL_SIZEX * NEOPIXEL_BYTES_PER_PIXEL)

#define CBAR_LEVELS 8
#define NOISE_FLOOR 30.f

static MIC_DEVICE microphone = NULL;
// static StreamNormalizer* processor = NULL;
static LevelDetectorSPL* splMeter = NULL;

static const int sampleRate = 50000;         // Hz
static const int dt = 1000000 / sampleRate;  // us

static uint8_t br = 5;
static uint8_t hbr = br / 2;
static uint8_t dbr = br * 2;

static uint8_t red[] = {0, dbr, 0};
static uint8_t orange[] = {hbr, br, 0};
static uint8_t green[] = {br, 0, 0};

static uint8_t* cBar[] = {green, green, green, green, green, orange, orange, red};

static uint8_t cBarFrames[CBAR_LEVELS]
                         [NEOPIXEL_PIXEL_SIZEX * NEOPIXEL_PIXEL_SIZEY * NEOPIXEL_BYTES_PER_PIXEL];

static int ilvl;
static float lvl;

void bakeCBarFrames()
{
    for (auto j = 0; j < CBAR_LEVELS; j++)
    {
        int i;
        for (i = 0; i <= j; i++)
        {
            auto c = cBar[i];

            auto c1 = c[0];
            auto c2 = c[1];
            auto c3 = c[2];
            auto addr = NEOPIXEL_BYTES_PER_PIXEL * i;

            cBarFrames[j][NEOPIXEL_ROW_0 + addr + 0] = c1;
            cBarFrames[j][NEOPIXEL_ROW_0 + addr + 1] = c2;
            cBarFrames[j][NEOPIXEL_ROW_0 + addr + 2] = c3;

            cBarFrames[j][NEOPIXEL_ROW_1 + addr + 0] = c1;
            cBarFrames[j][NEOPIXEL_ROW_1 + addr + 1] = c2;
            cBarFrames[j][NEOPIXEL_ROW_1 + addr + 2] = c3;

            cBarFrames[j][NEOPIXEL_ROW_2 + addr + 0] = c1;
            cBarFrames[j][NEOPIXEL_ROW_2 + addr + 1] = c2;
            cBarFrames[j][NEOPIXEL_ROW_2 + addr + 2] = c3;

            cBarFrames[j][NEOPIXEL_ROW_3 + addr + 0] = c1;
            cBarFrames[j][NEOPIXEL_ROW_3 + addr + 1] = c2;
            cBarFrames[j][NEOPIXEL_ROW_3 + addr + 2] = c3;
        }

        for (; i < CBAR_LEVELS; i++)
        {
            auto addr = NEOPIXEL_BYTES_PER_PIXEL * i;
            cBarFrames[j][NEOPIXEL_ROW_0 + addr + 0] = 0;
            cBarFrames[j][NEOPIXEL_ROW_0 + addr + 1] = 0;
            cBarFrames[j][NEOPIXEL_ROW_0 + addr + 2] = 0;

            cBarFrames[j][NEOPIXEL_ROW_1 + addr + 0] = 0;
            cBarFrames[j][NEOPIXEL_ROW_1 + addr + 1] = 0;
            cBarFrames[j][NEOPIXEL_ROW_1 + addr + 2] = 0;

            cBarFrames[j][NEOPIXEL_ROW_2 + addr + 0] = 0;
            cBarFrames[j][NEOPIXEL_ROW_2 + addr + 1] = 0;
            cBarFrames[j][NEOPIXEL_ROW_2 + addr + 2] = 0;

            cBarFrames[j][NEOPIXEL_ROW_3 + addr + 0] = 0;
            cBarFrames[j][NEOPIXEL_ROW_3 + addr + 1] = 0;
            cBarFrames[j][NEOPIXEL_ROW_3 + addr + 2] = 0;
        }
    }
}

int main()
{
    uBit.init();

    uBit.adc.setSamplePeriod(dt);
    microphone = uBit.adc.getChannel(uBit.io.microphone);
    microphone->setGain(7, 0);

    splMeter = new LevelDetectorSPL(microphone->output, 105., 45., 1.0, 40., DEVICE_ID_MICROPHONE);
    splMeter->setWindowSize(LEVEL_DETECTOR_SPL_DEFAULT_WINDOW_SIZE / 2);

    uBit.io.runmic.setDigitalValue(1);
    uBit.io.runmic.setHighDrive(true);

    bakeCBarFrames();

    while (true)
    {
        lvl = splMeter->getValue() - NOISE_FLOOR;

        ilvl = min((int)(lvl / 6.25f), CBAR_LEVELS - 1);
        ilvl = max(ilvl, 0);

        neopixel_send_buffer(
            uBit.io.P1, cBarFrames[ilvl],
            NEOPIXEL_BYTES_PER_PIXEL * NEOPIXEL_PIXEL_SIZEX * NEOPIXEL_PIXEL_SIZEY);
        uBit.sleep(5);
    }
}