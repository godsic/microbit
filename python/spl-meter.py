import microbit
from micropython import opt_level, const
import neopixel
import utime

opt_level(3)

np = neopixel.NeoPixel(microbit.pin0, 32)

br = const(5)
red = (br, 0, 0)
orange = (br, br // 2, 0)
green = (0, br, 0)
black = (0, 0, 0)

colors = [green, green, green, green, green, orange, orange, red]

soundLevel = 0
samples = 0
color = black

while True:

    soundLevel = 0
    samples = 0

    now = utime.ticks_ms()
    while utime.ticks_diff(utime.ticks_ms(), now) < 20:
        soundLevel += microbit.microphone.sound_level()
        soundLevel += microbit.microphone.sound_level()
        soundLevel += microbit.microphone.sound_level()
        soundLevel += microbit.microphone.sound_level()
        samples += 4

    soundLevel = min(soundLevel // samples, 8)

    for i in range(soundLevel):
        color = colors[i]
        np[i], np[8 + i], np[16 + i], np[24 + i] = color, color, color, color

    for i in range(soundLevel, 8):
        np[i], np[8 + i], np[16 + i], np[24 + i] = black, black, black, black

    np.show()
