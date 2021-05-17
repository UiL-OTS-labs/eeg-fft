
import importedf
import unittest
import os.path
import numpy as np
import scipy as sp

from gi.repository import Edf, GLib

class TestEdfFile (unittest.TestCase):

    temp_template = "test-python-libgedfXXXXXX"

    def setUp(self):
        self.tempdir = GLib.dir_make_tmp(self.temp_template)

    def test_create(self):
        outfile = Edf.File()
        signal1 = Edf.Signal()
        signal2 = Edf.Signal()

        outfile.add_signal(signal1)
        outfile.add_signal(signal2)

        self.assertEqual(outfile.props.num_signals, 2)

    def test_write(self):
        outfile = Edf.File(path="/tmp/pygedf.edf")
        signal1 = Edf.Signal()
        signal2 = Edf.Signal()


