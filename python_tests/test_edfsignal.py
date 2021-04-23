
import unittest
import importedf

from gi.repository import GLib, Edf

class test_signal(unittest.TestCase):

    def test_create(self):

        signal = Edf.Signal()

        self.assertEqual(signal.props.label, "")
        self.assertEqual(signal.props.transducer, "")
        self.assertEqual(signal.props.physical_min, 0)
        self.assertEqual(signal.props.physical_max, 0)
        self.assertEqual(signal.props.digital_min, 0)
        self.assertEqual(signal.props.digital_max, 0)
        self.assertEqual(signal.props.prefilter, "")
        self.assertEqual(signal.props.ns, 0)
        self.assertEqual(signal.props.reserved, "")
        self.assertEqual(signal.props.sample_size, 2)

    def test_signal_props(self):
        signal = Edf.Signal(
            label = " my signal ",
            transducer = "  AgCl Electrode",
            physical_min = 30.0,
            physical_max = 40,
            digital_min = 0,
            digital_max = 1024,
            prefilter = "HP:60Hz",
            ns =  1024,
            reserved = " reserved ",
            sample_size = 2
            )

        self.assertEqual(signal.props.label, "my signal")
        self.assertEqual(signal.props.transducer, "AgCl Electrode")
        self.assertEqual(signal.props.physical_min, 30)
        self.assertEqual(signal.props.physical_max, 40)
        self.assertEqual(signal.props.digital_min, 0)
        self.assertEqual(signal.props.digital_max, 1024)
        self.assertEqual(signal.props.prefilter, "HP:60Hz")
        self.assertEqual(signal.props.ns, 1024)
        self.assertEqual(signal.props.reserved, "reserved")
        self.assertEqual(signal.props.sample_size, 2)

        

