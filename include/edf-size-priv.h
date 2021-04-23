
#ifndef EDF_SIZE_PRIV_H
#define EDF_SIZE_PRIV_H

// number of bytes in the header for the fixed part
#define EDF_VERSION_SZ                  8
#define EDF_LOCAL_PATIENT_SZ            80
#define EDF_LOCAL_RECORDING_SZ          80
#define EDF_START_DATE_SZ               8
#define EDF_START_TIME_SZ               8
#define EDF_NUM_BYTES_IN_HEADER_SZ      8
#define EDF_RESERVED_SZ                 44
#define EDF_NUM_DATA_REC_SZ             8
#define EDF_DURATION_OF_DATA_RECORD_SZ  8
#define EDF_NUM_SIGNALS_SZ              4

#define EDF_BASE_HEADER_SIZE (            \
    EDF_VERSION_SZ                      + \
    EDF_LOCAL_PATIENT_SZ                + \
    EDF_LOCAL_RECORDING_SZ              + \
    EDF_START_DATE_SZ                   + \
    EDF_START_TIME_SZ                   + \
    EDF_NUM_BYTES_IN_HEADER_SZ          + \
    EDF_RESERVED_SZ                     + \
    EDF_NUM_DATA_REC_SZ                 + \
    EDF_DURATION_OF_DATA_RECORD_SZ      + \
    EDF_NUM_SIGNALS_SZ                    \
)

// number of bytes in the header for the part dependent on num signals
#define EDF_LABEL_SZ                    16
#define EDF_TRANDUCER_TYPE_SZ           80
#define EDF_PHYSICAL_DIMENSION_SZ       8
#define EDF_PHYSICAL_MINIMUM_SZ         8
#define EDF_PHYSICAL_MAXIMUM_SZ         8
#define EDF_DIGITAL_MINIMUM_SZ          8
#define EDF_DIGITAL_MAXIMUM_SZ          8
#define EDF_PREFILTERING_SZ             80
#define EDF_NUM_SAMPLES_PER_RECORD_SZ   8
#define EDF_NS_RESERVED_SZ              32

// The size of the header for each signal added.
#define EDF_SIGNAL_HEADER_SIZE (          \
        EDF_LABEL_SZ                    + \
        EDF_TRANDUCER_TYPE_SZ           + \
        EDF_PHYSICAL_DIMENSION_SZ       + \
        EDF_PHYSICAL_MINIMUM_SZ         + \
        EDF_PHYSICAL_MAXIMUM_SZ         + \
        EDF_DIGITAL_MINIMUM_SZ          + \
        EDF_DIGITAL_MAXIMUM_SZ          + \
        EDF_PREFILTERING_SZ             + \
        EDF_NUM_SAMPLES_PER_RECORD_SZ   + \
        EDF_NS_RESERVED_SZ                \
)


#endif
