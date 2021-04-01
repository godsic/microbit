#!/usr/bin/env python3
import serial
import time
import matplotlib.pylab as plt
import numpy as np
from parse import parse
import time

x = []
y1 = []
y2 = []

plt.matplotlib.use('gtk3agg')

with serial.Serial('/dev/ttyACM0', 115200, timeout=1, parity=serial.PARITY_NONE) as ser:

    fig = plt.figure()

    t_p = 0
    while True:

        for _ in range(10):
            line = ser.readline()
            try:
                (t, d1, d2) = parse("{:d} {:d} {:d}\n", line.decode("utf-8"))
            except:
                continue
            else:
                if d1 > 2.0 * d2 or d2 > 2.0 * d1:
                    continue
                if t > t_p:
                    x.append(t)
                    y1.append(d1)
                    y2.append(d2)
                    t_p = t
            finally:
                time.sleep(0.03)

        plt.plot(x, y1)
        plt.plot(x, y2)
        plt.draw()
        plt.pause(1./30.)
        fig.clear()