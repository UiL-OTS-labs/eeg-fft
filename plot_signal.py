#!/usr/bin/env python3

# MIT License
# 
# Copyright (c) 2020 UiL OTS labs
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import edf
import matplotlib
matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt
import numpy as np
import scipy as sp
import scipy.fftpack as fft
import sys

def draw_plots(edffile:edf.EdfFile, channels):
    num_figures = 2
    num_plots = len(channels)
    fig_sig, ax1 = plt.subplots(num_plots, 1)
    fig_fourier, ax2 = plt.subplots(num_plots, 1)

    if num_plots == 1:
        ax1 = [ax1]
        ax2 = [ax2]

    for i, channel in enumerate (channels):
        sample_rate = edffile.header.num_samples_per_record[channel]
        sample_time = 1.0/sample_rate
        signal1     = np.array(edffile.samples[channel])
        n           = signal1.size
        time        = np.arange(0, n * sample_time, sample_time)
        signal_length = time[-1]

        ax1[i].plot(time, signal1)
        ax1[i].set_xlabel("time (s)")
        ax1[i].set_ylabel(
                edffile.header.labels[channel] + " (" +
                edffile.header.physical_dimension[channel] + ")"
                )

        signal1 = signal1 - np.average(signal1)
        sr = n / edffile.header.num_samples_per_record[0]

        # TODO Normalize the output of the fft
        fourier = fft.fft(signal1, n)

        freq = fft.fftfreq(n, sample_time)
        ax2[i].plot (freq[:n//2], np.abs(fourier[:n//2]))

    plt.show()

def examine_channels(channels, maxchan):
    """Determines whether the correct channel has been given"""
    for i in channels:
        if i < 0 or i >= maxchan:
            print ("Channel {} is not in the range[0, {})".format(i, maxchan),
                file=sys.stderr
                )
            return False
    return True

if __name__ == "__main__":
    import argparse as ap
    import os.path as ospath
    parser = ap.ArgumentParser()
    parser.add_argument("input_file", help="File to open", type=str)
    parser.add_argument(
            "channels",
            help="specify the channels to be examined",
            type=int,
            nargs="+"
            )
    args = parser.parse_args()

    edffile = edf.EdfFile.from_file(args.input_file)
    channels = args.channels

    ns = edffile.header.num_signals
    if not examine_channels(channels, ns):
        exit(1)

    draw_plots(edffile, channels)

