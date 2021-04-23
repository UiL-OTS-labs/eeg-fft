
import importedf
import unittest
import os.path

from gi.repository import Edf, GLib

class TestEdfFile (unittest.TestCase):

    temp_template = "test-python-libgedfXXXXXX"

    def setUp(self):
        self.tempdir = GLib.dir_make_tmp(self.temp_template)

    def test_create(self):
        outfile = Edf.File()
        signal1 = Edf.Signal()
        signal2 = Edf.Signal()


