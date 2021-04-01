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

MicroBit uBit;

#define SONAR_MIN_TRIG_PULSE_WIDTH_US 25   // us
#define SONAR_MIN_MEASUREMENT_CYCLE_MS 60  // ms
#define SONAR_UPDATES_PER_SECOND 10
#define SONAR_TO_SONAR_DISTANCE 0.045f  // m
#define NUMBER_OF_SONARS 2
#define SPEED_OF_SOUND 350  // m / s
#define DEG_TO_RAD (float(PI / 180.0))
#define S_TO_US 1.e6f
#define S_TO_MS 1000
#define PRINT_TO_SERIAL

static const auto MAX_STEERING_DELAY_US =
    (SONAR_TO_SONAR_DISTANCE / SPEED_OF_SOUND * S_TO_US);  // us

static const uint32_t SONAR_MEASUREMENT_CYCLE_MS =
    max(SONAR_MIN_MEASUREMENT_CYCLE_MS, S_TO_MS / SONAR_UPDATES_PER_SECOND);

static uint32_t distance[NUMBER_OF_SONARS];                // Measured distance, mm
static const int echo_pins[NUMBER_OF_SONARS] = {1, 16};    // Echo pins
static const int trigger_pins[NUMBER_OF_SONARS] = {2, 8};  // Trigger pins

static int sonar_ids[NUMBER_OF_SONARS];

static float steering_angle = 0.0f;

void RecordDistance(MicroBitEvent evt, void* id)
{
    auto i = (int*)id;
    distance[*i] = SPEED_OF_SOUND * evt.timestamp / 2 / 1000;
    return;
}

inline void TriggerSonarsWithSteering(float steeringAngle)
{
    target_disable_irq();

    auto timeDelayUs = static_cast<uint32_t>(
        round(MAX_STEERING_DELAY_US * sinf(fabs(steeringAngle) * DEG_TO_RAD)));

    uint32_t SONAR_TRIG_PULSE_WIDTH_US =
        max(SONAR_MIN_TRIG_PULSE_WIDTH_US, (NUMBER_OF_SONARS - 1) * timeDelayUs + 1);  // us

    for (auto const& tp : trigger_pins)
    {
        uBit.io.pin[tp].setDigitalValue(0);
    }
    target_wait_us(SONAR_MIN_TRIG_PULSE_WIDTH_US);

    if (steeringAngle > 0)
    {
        for (auto i = 0; i < NUMBER_OF_SONARS; i++)
        {
            auto tp = trigger_pins[i];
            uBit.io.pin[tp].setDigitalValue(1);
            if (i != NUMBER_OF_SONARS - 1) target_wait_us(timeDelayUs);
        }

        target_wait_us(SONAR_TRIG_PULSE_WIDTH_US - (NUMBER_OF_SONARS - 1) * timeDelayUs);

        for (auto i = 0; i < NUMBER_OF_SONARS; i++)
        {
            auto tp = trigger_pins[i];
            uBit.io.pin[tp].setDigitalValue(0);
            if (i != NUMBER_OF_SONARS - 1) target_wait_us(timeDelayUs);
        }
    }
    else if (steeringAngle < 0)
    {
        for (auto i = 0; i < NUMBER_OF_SONARS; i++)
        {
            auto tp = trigger_pins[(NUMBER_OF_SONARS - 1) - i];
            uBit.io.pin[tp].setDigitalValue(1);
            if (i != NUMBER_OF_SONARS - 1) target_wait_us(timeDelayUs);
        }

        target_wait_us(SONAR_TRIG_PULSE_WIDTH_US - (NUMBER_OF_SONARS - 1) * timeDelayUs);

        for (auto i = 0; i < NUMBER_OF_SONARS; i++)
        {
            auto tp = trigger_pins[(NUMBER_OF_SONARS - 1) - i];
            uBit.io.pin[tp].setDigitalValue(0);
            if (i != NUMBER_OF_SONARS - 1) target_wait_us(timeDelayUs);
        }
    }
    else
    {
        for (auto const& tp : trigger_pins)
        {
            uBit.io.pin[tp].setDigitalValue(1);
        }
        target_wait_us(SONAR_MIN_TRIG_PULSE_WIDTH_US);

        for (auto const& tp : trigger_pins)
        {
            uBit.io.pin[tp].setDigitalValue(0);
        }
    }

    target_enable_irq();
}

void ShowDistance(MicroBitEvent evt)
{
#ifdef PRINT_TO_SERIAL
    uBit.serial.printf("%d %d %d\n", uBit.systemTime(), distance[0], distance[1]);
#else
    for (auto const& d : distance)
    {
        uBit.display.scroll(d);
    }
    uBit.display.scrollAsync("mm");
#endif
}

void onButtonA(MicroBitEvent)
{
    if (steering_angle > -30.0f) steering_angle -= 5.0f;
    uBit.display.scroll(int(steering_angle));
}

void onButtonB(MicroBitEvent)
{
    if (steering_angle < 30.0f) steering_angle += 5.0f;
    uBit.display.scroll(int(steering_angle));
}

int main()
{
    uBit.init();

    uBit.messageBus.listen(DEVICE_ID_BUTTON_A, DEVICE_BUTTON_EVT_CLICK, onButtonA,
                           MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
    uBit.messageBus.listen(DEVICE_ID_BUTTON_B, DEVICE_BUTTON_EVT_CLICK, onButtonB,
                           MESSAGE_BUS_LISTENER_DROP_IF_BUSY);

    for (auto const& echo_pin : echo_pins)
    {
        uBit.io.pin[echo_pin].eventOn(MICROBIT_PIN_EVENT_ON_PULSE);
    }

    for (auto i = 0; i < NUMBER_OF_SONARS; i++)
    {
        sonar_ids[i] = i;
        uBit.messageBus.listen(MICROBIT_ID_IO_P0 + echo_pins[i], MICROBIT_PIN_EVT_PULSE_HI,
                               RecordDistance, (void*)&sonar_ids[i],
                               MESSAGE_BUS_LISTENER_REENTRANT);
    }

    uBit.messageBus.listen(MICROBIT_ID_IO_P0 + echo_pins[0], MICROBIT_PIN_EVT_PULSE_LO,
                           ShowDistance, MESSAGE_BUS_LISTENER_QUEUE_IF_BUSY);

    while (true)
    {
        TriggerSonarsWithSteering(steering_angle);
        uBit.sleep(SONAR_MEASUREMENT_CYCLE_MS);
    }
}