#!/usr/bin/env python3

import numpy as np
import scipy as sp
from matplotlib import pyplot as plt

def plot_fft_timeseries(time : np.array, y : np.array):
    plt.figure()
    if len(time) != len(y) or len(time) < 2:
        raise ValueError("len(time) and len(y) must be equal and len() must"
                         "be larger then 2")
    sampletime = time[1] - time[0]
    N = len(time)
    fourier = np.fft.fft(y, N) * (1/N)
    x = np.linspace(0,1/sampletime, N)
    plt.plot(x[:N//2], np.abs(fourier[:N//2]))



if __name__ == "__main__":
    SR = 2048
    time = np.arange(0, 1, 1/SR)
    y = sp.sin(50 * time * 2 * np.pi) * .5
    y = y + sp.sin(4 * time * 2 * np.pi) * 2
    y = y + sp.sin(40 * time * 2 * np.pi)
    y = y + sp.sin(440 * time * 2 * np.pi) * 2
    y = y + np.random.randn(y.size) * 1
    plt.plot(time, y)
    plot_fft_timeseries(time, y)
    plt.show()

