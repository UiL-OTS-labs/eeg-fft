# eeg-fft
Read edf/bdf files and perform discrete fourier transform to identify dominant
frequencies in the signal. The utility consists of an edf/bdf reader edf.py and
a plotting program: plot-signal.py
The plotting program takes currently at least two arguments. The first is the
name of the edf file, and the remaining shall be integer values that is a valid
zero base index in the edf/bdf file.

so for example you can 
