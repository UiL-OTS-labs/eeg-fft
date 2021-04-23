
import importedf
import unittest

from gi.repository import Edf, GLib

class TestEdfHeader(unittest.TestCase):

    def test_header_construct(self):
        """ test the default values of an header """
        dtstart = GLib.DateTime.new_now_local()
        hdr = Edf.Header()
        dtend = GLib.DateTime.new_now_local()

        self.assertEqual(hdr.props.version, 0)
        self.assertEqual(hdr.get_property("patient-identification"), "")
        self.assertEqual(hdr.get_property("recording-identification"), "")
        self.assertGreater(GLib.DateTime.difference(hdr.get_property("date-time"), dtstart), 0)
        self.assertLess(GLib.DateTime.difference(hdr.get_property("date-time"), dtend), 0)
        self.assertEqual(hdr.get_property("num_bytes_header"), Edf.compute_header_size(0));
        self.assertEqual(hdr.props.reserved, "")
        self.assertEqual(hdr.get_property("num-data-records"), -1);
        self.assertEqual(hdr.get_property("duration-of-record"), 1.0);
        self.assertEqual(hdr.get_property("num-signals"), 0)

    def test_header_construct_values(self):
        """ test the constructed values of an header """

        version = 1
        patient = "patient-blaat"
        recording = "rec_blaat"
        currenttime = GLib.DateTime.new_now_local()
        num_bytes_header = 123
        num_data_records = 10
        num_signals = 1
        dur = 2.0
        reserved = "reserved stuff"

        hdr = Edf.Header(
            patient_identification = patient,
            recording_identification = recording,
            date_time = currenttime,
            reserved = reserved,
            duration_of_record = dur
        )
        hdr.set_time(currenttime)
        
        self.assertEqual(hdr.props.version, 0)
        self.assertEqual(hdr.get_property("patient-identification"), patient)
        self.assertEqual(hdr.get_property("recording-identification"), recording)
        self.assertEqual(GLib.DateTime.difference(hdr.get_property("date-time"), currenttime), 0)
        self.assertEqual(hdr.get_property("num_bytes_header"), Edf.compute_header_size(0));
        self.assertEqual(hdr.props.reserved, reserved)
        self.assertEqual(hdr.get_property("num-data-records"), -1);
        self.assertEqual(hdr.get_property("duration-of-record"), dur)
        self.assertEqual(hdr.get_property("num-signals"), 0)


