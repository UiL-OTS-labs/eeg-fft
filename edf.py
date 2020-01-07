#!/usr/bin/env python3
"""
Module for opening and reading EDF(+) files
"""

import typing
import time
import datetime
import struct

class EdfDate(datetime.datetime):

    LEN_DATE_FIELD = 8

    @classmethod
    def from_file(cls, fileobj: typing.BinaryIO):
        date = fileobj.read(cls.LEN_DATE_FIELD)
        timept = fileobj.read(cls.LEN_DATE_FIELD)
        if date[2:3].decode('ascii') != "." or            \
           date[5:6].decode('ascii') != "." or            \
           timept[2:3].decode('ascii') != "." or          \
           timept[5:6].decode('ascii') != ".":
            raise ValueError("Invalid EDF date found")
        year = date[0:2].decode('ascii')
        month= date[3:5].decode('ascii')
        day = date[6:].decode('ascii')
        hour = timept[0:2].decode('ascii')
        minute = timept[3:5].decode('ascii')
        second = timept[6:].decode('ascii')
        return EdfDate(
                int(year),
                int(month),
                int(day),
                int(hour),
                int(minute),
                int(second)
                )

class EdfHeader:
    """Represents a header of the Edfdata.

    The header describes what the content of a file is and should be regarded
    as the source of information how to parse the rest of the
    information contained in the file.
    """

    # Common length of header variables in bytes
    LEN_4BYTE = 4
    LEN_8BYTE = 8
    LEN_16BYTE = 16
    LEN_44BYTE = 44
    LEN_32BYTE = 32
    LEN_80BYTE = 80

    def __init__(self):
        """Initialize a EdfHeader with default values."""
        self.version = 0
        self.loc_patient_info = ""
        self.loc_rec_info = ""
        tpnt = time.localtime(time.time())
        self.date = EdfDate(*tpnt[:6])
        self.start_date = None
        self.start_time = None
        self.num_header_bytes = 0
        self.reserved1 = None
        self.num_data_records = 0
        self.dur_data_record = 0
        self.num_signals = 0
        self.labels = []
        self.tranducer = []
        self.physical_dimension = []
        self.physical_min = []
        self.physical_max = []
        self.digital_min = []
        self.digital_max = []
        self.prefiltering = []
        self.num_samples_per_record = []
        self.reserved2 = []

    def is_biosemi(self):
        return self.version == 255

    def from_file(self, fileobj: typing.BinaryIO):
        '''Read the header from a file that is already opened.

        @param fn A opened file from which it is assumed that the we read from
        the start. The file is also assumed to be opened for reading.
        '''
        self._read_version(fileobj)
        self._read_loc_patient_info(fileobj)
        self._read_loc_rec_info(fileobj)
        self.date = EdfDate.from_file(fileobj)
        self._read_num_header_bytes(fileobj)
        self._read_reserved1(fileobj)
        self._read_num_data_records(fileobj)
        self._read_duration_record(fileobj)
        self._read_num_signals(fileobj)
        self._read_labels(fileobj)
        self._read_transducer(fileobj)
        self._read_physical_dimension(fileobj)
        self._read_physical_min(fileobj)
        self._read_physical_max(fileobj)
        self._read_digital_min(fileobj)
        self._read_digital_max(fileobj)
        self._read_prefiltering(fileobj)
        self._read_num_samples_per_record(fileobj)
        self._read_reserved2(fileobj)
    
    @staticmethod
    def _read_label_list(
            fileobj:typing.BinaryIO,
            nbytes,
            ntot
            ) -> typing.List[str]:
        """Read ntot bytes object of nbytes and return them as a list of str"""
        strings = [None] * ntot
        for i in range(ntot):
            strings[i] = fileobj.read(nbytes).rstrip().decode('ascii')
        return strings

    @staticmethod
    def _read_int_list(
            fileobj:typing.BinaryIO,
            nbytes,
            ntot
            ) -> typing.List[int]:
        strlist = EdfHeader._read_label_list(fileobj, nbytes, ntot)
        return [int(string) for string in strlist]

    def _read_version(self, fileobj: typing.BinaryIO):
        version_bytes = fileobj.read(self.LEN_8BYTE)
        if version_bytes[0] != 255: 
            version_str = version_bytes.rstrip().decode('ascii')
            self.version = int(version)
        else: # BioSemi format
            if version_bytes[1:].decode('ascii') != "BIOSEMI":
                raise ValueError("Invalid or unknow format")
            self.version = 255

    def _read_loc_patient_info(self, fileobj: typing.BinaryIO):
        patient_info = fileobj.read(self.LEN_80BYTE).rstrip().decode('ascii')
        self.loc_patient_info = patient_info

    def _read_loc_rec_info(self, fileobj: typing.BinaryIO):
        rec_info = fileobj.read(self.LEN_80BYTE)
        self.loc_rec_info = rec_info.rstrip().decode('ascii')

    def _read_num_header_bytes(self, fileobj: typing.BinaryIO):
        num_bytes = fileobj.read(self.LEN_8BYTE).decode('ascii')
        self.num_header_bytes = int(num_bytes)

    def _read_reserved1(self, fileobj):
        self.reserved1 = fileobj.read(self.LEN_44BYTE)

    def _read_num_data_records(self, fileobj: typing.BinaryIO):
        records = fileobj.read(self.LEN_8BYTE)
        self.num_data_records = int(records)

    def _read_duration_record(self, fileobj: typing.BinaryIO):
        dur = fileobj.read(self.LEN_8BYTE)
        self.dur_data_record = float(dur.decode('ascii'))

    def _read_num_signals(self, fileobj: typing.BinaryIO):
        ns = fileobj.read(self.LEN_4BYTE).decode('ascii')
        self.num_signals = int(ns)

    def _read_labels(self, fileobj: typing.BinaryIO):
        self.labels = EdfHeader._read_label_list(
            fileobj,
            self.LEN_16BYTE,
            self.num_signals
        )

    def _read_transducer(self, fileobj : typing.BinaryIO):
        self.tranducer = EdfHeader._read_label_list(
            fileobj,
            self.LEN_80BYTE,
            self.num_signals
        )
        
    def _read_physical_dimension(self, fileobj : typing.BinaryIO):
        self.physical_dimension = EdfHeader._read_label_list(
            fileobj,
            self.LEN_8BYTE,
            self.num_signals
        )

    def _read_physical_min(self, fileobj : typing.BinaryIO):
        self.physical_min = EdfHeader._read_int_list(
            fileobj,
            self.LEN_8BYTE,
            self.num_signals
            )
    
    def _read_physical_max(self, fileobj : typing.BinaryIO):
        self.physical_max = EdfHeader._read_int_list(
            fileobj,
            self.LEN_8BYTE,
            self.num_signals
            )
    
    def _read_digital_min(self, fileobj : typing.BinaryIO):
        self.digital_min = EdfHeader._read_int_list(
            fileobj,
            self.LEN_8BYTE,
            self.num_signals
            )

    def _read_digital_max(self, fileobj : typing.BinaryIO):
        self.digital_max= EdfHeader._read_int_list(
            fileobj,
            self.LEN_8BYTE,
            self.num_signals
            )

    def _read_prefiltering(self, fileobj : typing.BinaryIO):
        self.prefiltering = EdfHeader._read_label_list(
                fileobj,
                self.LEN_80BYTE,
                self.num_signals
                )

    def _read_num_samples_per_record(self, fileobj: typing.BinaryIO):
        self.num_samples_per_record = EdfHeader._read_int_list(
                fileobj,
                self.LEN_8BYTE,
                self.num_signals
                )

    def _read_reserved2(self, fileobj : typing.BinaryIO):
        reserved = [None] * self.num_signals 
        for i in range(self.num_signals):
            reserved[i] = fileobj.read(self.LEN_32BYTE)
        self.reserved2 = reserved

    def __repr__(self):
        rep_str = (
            "EdfHeader(" +
            repr(self.version) + ", " +
            repr(self.loc_patient_info) + ", " +
            repr(self.loc_patient_info) + ", " +
            repr(self.date) + ", " +
            repr(self.reserved1) + ", " +
            repr(self.num_header_bytes) + ", " +
            repr(self.num_data_records) + ", " +
            repr(self.num_signals) + ",\n" +
            repr(self.labels) + ",\n" +
            repr(self.tranducer) + ",\n" +
            repr(self.physical_dimension) + ",\n" +
            repr(self.physical_min) + ",\n" +
            repr(self.physical_max) + ",\n" +
            repr(self.digital_max) + ",\n" +
            repr(self.digital_min) + ",\n" +
            repr(self.num_samples_per_record) + ",\n" +
            repr(self.reserved2) +"\n" +
            ")"
        )
        return rep_str

def _read_edf_samples(fileobj : typing.BinaryIO, num_samples):
    """Read values from a regular edf file"""
    for i in range(num_samples):
        yield int.from_bytes(fileobj.read(2), 'little')

def _read_bdf_samples(fileobj : typing.BinaryIO, num_samples):
    """Read values from a regular edf file"""
    for i in range(num_samples):
        yield int.from_bytes(fileobj.read(3), 'little')

class EdfFile:
    '''
    Represents a EdfFile
    '''

    def __init__(self, header : EdfHeader = EdfHeader(), samples=[[]]):
        """Initializes an empty Edf class containing a header with default
        values.
        """
        self.header = header 
        self.samples= samples

    def _read_samples(self, fileobj: typing.BinaryIO):
        """ Read the samples base upon the header information
        """
        num_records = self.header.num_data_records
        num_signals = self.header.num_signals
        output = [[] for i in range(num_signals)]
        for record in range(num_records):
            for signal in range(num_signals):
                num_samples = self.header.num_samples_per_record[signal]
                samples = None
                if not self.header.is_biosemi():
                    samples = list(_read_edf_samples(fileobj, num_samples))
                else:
                    samples = list(_read_bdf_samples(fileobj, num_samples))
                output[signal].extend(samples)
        self.samples = output

    @classmethod
    def from_fileobj(cls, fileobj: typing.BinaryIO):
        """Read a EdfFile from a file"""
        edfinstance = EdfFile()
        edfinstance.header.from_file(fileobj)
        edfinstance._read_samples(fileobj)
        return edfinstance
 
    @classmethod
    def from_file(cls, filename: str):
        """Read a EdfFile from a file"""
        fileobj = open(filename, 'rb')
        return cls.from_fileobj(fileobj)
    
    def __repr__(self) -> str:
        return "EdfFile({}, {})".format(repr(self.header), repr(self.samples))

if __name__ == "__main__":
    import argparse as ap
    import os.path as ospath
    parser = ap.ArgumentParser()
    parser.add_argument("input_file", help="File to open", type=str)
    args = parser.parse_args()

    print("Opening {}".format(args.input_file))

    filename = ospath.abspath(args.input_file)

    edffile = EdfFile.from_file(filename)
    print(edffile.header)
    print("is biosemi", edffile.header.is_biosemi())
