
#include <CUnit/CUnit.h>
#include <gedf.h>
#include <stdarg.h>
#include "edf-file.h"
#include "edf-signal.h"
#include "test-macros.h"
#include <time.h>

/* ******** global constants ********* */
static const char* SUITE_NAME = "EdfFileSuite";

// A default name to use for an edf file
const gchar* default_name = "hello-world.edf";

typedef struct {
    int         version;
    const char *loc_patient;
    const char *loc_recording;
    GDateTime  *date;
    size_t      expected_header_size;
    const char *reserved;
    int         num_records;
    double      dur_record;
    unsigned    num_signals;
} header_info;

typedef struct {
    const char *label;
    const char *transducer;
    const char *phys_dim;
    double      phys_min;
    double      phys_max;
    int         digital_min;
    int         digital_max;
    const char *prefiltering;
    unsigned    num_samples_per_record;
    const char *sig_reserved;
} signal_info;

header_info hdr_info = {
    .version                = 0,
    .loc_patient            = "mja local patient",
    .loc_recording          = "Vedel 5",
    .expected_header_size   = 256 + 2 * 256,
    .reserved               = "reserved",
    .num_records            = 60,
    .dur_record             = 1,
    .num_signals            = 2
};

signal_info sig_info[2] = {
    {
        "cz",
        "active electrode",
        "uV",
        -1000.0,
        1000.0,
        0,
        2048,
        "N:50Hz",
        2048,
        "Signal reserved1"
    },
    {
        "Fp2",
        "active electrode",
        "uV",
        -1000.0,
        1000.0,
        0,
        1024,
        "N:50Hz",
        2048,
        "Signal reserved2"
    }
};



/* ******** global variables ********* */

/* initialized in file_test_init and destroyed in file_test_finalize */
static gchar* temp_dir;

static int 
file_test_init(void)
{
    gchar template[1024] = "gedf_unit_test_XXXXXX";
    GError* error = NULL;
    temp_dir = g_dir_make_tmp(template, &error);
    if (error) {
        g_printerr("Unable to open temp dir: %s\n", error->message);
        g_error_free(error);
        return -1;
    }
    else
        g_print("Created directory: %s\n", temp_dir);
    return 0;
}

static int 
file_test_finalize(void)
{
    g_free(temp_dir);
    temp_dir = NULL;
    return 0;
}

/* ******* utility functions ************ */

double jitter(GRand* dev, double amplitude) {
    double halve_range = amplitude / 2;
    return g_rand_double_range(dev, -halve_range, halve_range);
}

static void 
fill_signal(
        EdfSignal  *sig,
        double      freq,
        double      amplitude,
        double      offset,
        unsigned    sr,
        size_t      num_samples,
        double      jitter_amplitude,
        GError    **error
        )
{
    GRand *randdev = g_rand_new();
    (void) jitter_amplitude;
    for (size_t i = 0; i < num_samples; i++) {
        int val;
        double jitval = jitter(randdev, jitter_amplitude);
        double sinoid = amplitude * sin((i * (1.0/sr)) * 2 * M_PI * freq) ;
        sinoid += jitval;
        sinoid += offset;
        val = (int) round(sinoid);
        edf_signal_append_digital(sig, val, error);
        if (*error)
            break;
    }
    g_rand_free(randdev);
}

/* ******* tests ******** */

static void
file_create(void)
{
    EdfFile* file = NULL;
    file = edf_file_new_for_path(default_name);
    gchar *dirname, *expected_name;
    gchar *output_name = NULL;
    
    dirname = g_get_current_dir();
    expected_name = g_build_filename(dirname, default_name, NULL);
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_TRUE(EDF_IS_FILE(file));

    g_object_get(G_OBJECT(file),
            "fn", &output_name,
            NULL
            );
    CU_ASSERT_STRING_EQUAL(output_name, expected_name);

    g_free(output_name);
    g_free(dirname);
    g_free(expected_name);

    edf_file_destroy(file);
}

static void
file_name(void)
{
    EdfFile* file = NULL;
    file = edf_file_new_for_path(default_name);
    const gchar* cname = "hello-world.edf";
    gchar *build_name = NULL, *name_ret = NULL;
    build_name = g_build_filename(temp_dir, cname, NULL);

    edf_file_set_path(file, build_name);
    name_ret = edf_file_get_path(file);

    CU_ASSERT_STRING_EQUAL(name_ret, build_name);

    g_free(name_ret);
    g_free(build_name);

    g_object_unref(file);
}

static void
file_open_writing(void)
{
    EdfFile* file = NULL;
    GError* error = NULL;

    gchar* tempfile = g_build_filename(temp_dir, "temp.edf", NULL);
    file  = edf_file_new_for_path(tempfile);

    edf_file_create(file, &error);
    CU_ASSERT_PTR_NULL(error);
    if(error) {
        g_printerr("Unable to open '%s':\n\t- %s\n", tempfile, error->message);
        g_error_free(error);
        error = NULL;
    }
    
    edf_file_create(file, &error);
    CU_ASSERT_PTR_NOT_NULL(error);
    if(error) {
        g_error_free(error);
        error = NULL;
        edf_file_replace(file, &error);
        CU_ASSERT_PTR_NULL(error);
        if (error)
            g_error_free(error);
    }


    g_free(tempfile);
    edf_file_destroy(file);
}

static void
file_write_with_elaborate_header_and_signals(void)
{
    EdfFile* file = NULL;
    EdfSignal* signal1 = NULL, *signal2 = NULL;
    GError* error = NULL;
    EdfHeader* header = NULL;

    // Create file
    gchar* tempfile = g_build_filename(temp_dir, default_name, NULL);
    file = edf_file_new_for_path(tempfile);

    g_object_get(G_OBJECT(file),
            "header", &header,
            NULL
            );

    g_object_set(
            G_OBJECT(header),
            "patient-identification", hdr_info.loc_patient,
            "recording-identification", hdr_info.loc_recording,
            "reserved", hdr_info.reserved,
            "duration-of-record", hdr_info.dur_record,
            NULL
            );

    // Create and fill 2 signals
    signal1 = edf_signal_new();
    signal2 = edf_signal_new();
    g_object_set(signal1,
            "label", sig_info[0].label,
            "transducer", sig_info[0].transducer,
            "physical-dimension", sig_info[0].phys_dim,
            "physical-min", sig_info[0].phys_min,
            "physical-max", sig_info[0].phys_max,
            "digital-min", sig_info[0].digital_min,
            "digital-max", sig_info[0].digital_max,
            "prefilter", sig_info[0].prefiltering,
            "ns", sig_info[0].num_samples_per_record,
            "reserved", sig_info[0].sig_reserved,
            NULL
            );
    g_object_set(signal2,
            "label", sig_info[1].label,
            "transducer", sig_info[1].transducer,
            "physical-dimension", sig_info[1].phys_dim,
            "physical-min", sig_info[1].phys_min,
            "physical-max", sig_info[1].phys_max,
            "digital-min", sig_info[1].digital_min,
            "digital-max", sig_info[1].digital_max,
            "prefilter", sig_info[1].prefiltering,
            "ns", sig_info[1].num_samples_per_record,
            "reserved", sig_info[1].sig_reserved,
            NULL
            );
    edf_file_add_signal(file, signal1);
    edf_file_add_signal(file, signal2);

    // Fill signals with generated signals
    unsigned sr = sig_info[0].num_samples_per_record;
    unsigned num_samples = sr * hdr_info.num_records;
    double hz = 10;
    double amplitude = 100;
    double offset = 400;
    double jitter_amplitude = 20;

    fill_signal(
            signal1, hz, amplitude, offset, sr, num_samples, jitter_amplitude,
            &error
            );
    if (error) {
        g_printerr("%s\n", error->message);
        goto fail;
    }
    sr = sig_info[1].num_samples_per_record;
    num_samples = sr * hdr_info.num_records;
    hz = 20;
    amplitude = 50;
    offset = 440;
    jitter_amplitude = 5;
    fill_signal(
            signal2, hz, amplitude, offset, sr, num_samples, jitter_amplitude,
            &error
            );
    if (error) {
        g_printerr("%s\n", error->message);
        goto fail;
    }
    
    edf_file_replace(file, &error);
    CU_ASSERT_PTR_NULL(error);
    if (error) {
        g_printerr("%s oops: %s", __func__, error->message);
        goto fail;
    }

fail:
    g_free(tempfile);
    g_object_unref(file);
    g_object_unref(header);
    g_object_unref(signal1);
    g_object_unref(signal2);
    if (error)
        g_error_free(error);
}

static void
file_open_reading(void)
{
    EdfFile *file = NULL;
    GError  *error = NULL;
    EdfHeader* hdr = NULL;
    gchar   *tempfile = g_build_filename(
            temp_dir,
            default_name,
            NULL
            );

    int          version;
    char        *loc_patient = NULL;
    char        *loc_recording = NULL;
    GDateTime   *date = NULL;
    unsigned int expected_header_size;
    char        *reserved = NULL;
    int          num_records;
    double       dur_record;
    unsigned int num_signals;

    file  = edf_file_new_for_path(tempfile);

    edf_file_read(file, &error);
    CU_ASSERT_PTR_NULL(error);
    if (error) {
        g_printerr("%s:%d oops: %s", __func__, __LINE__, error->message);
        goto fail;
    }

    hdr = edf_file_header(file);
    g_object_get(
        hdr,
        "version", &version,
        "patient-identification", &loc_patient,
        "recording-identification", &loc_recording,
        "date-time", &date,
        "num-bytes-header", &expected_header_size,
        "reserved", &reserved,
        "num-data-records", &num_records,
        "duration-of-record", &dur_record,
        "num-signals", &num_signals,
        NULL
    );
    CU_ASSERT_EQUAL(version, hdr_info.version);
    CU_ASSERT_EQUAL(expected_header_size, hdr_info.expected_header_size);
    CU_ASSERT_EQUAL(num_records, hdr_info.num_records);
    CU_ASSERT_EQUAL(dur_record, hdr_info.dur_record);
    CU_ASSERT_EQUAL(num_signals, hdr_info.num_signals);
    CU_ASSERT_STRING_EQUAL(loc_patient, hdr_info.loc_patient);
    CU_ASSERT_STRING_EQUAL(loc_recording, hdr_info.loc_recording);
    CU_ASSERT_STRING_EQUAL(reserved, hdr_info.reserved);

fail:
    g_object_unref(file);
    g_object_unref(hdr);
    g_free(tempfile);
    g_free(loc_patient);
    g_free(loc_recording);
    g_free(reserved);
    if (date)
        g_date_time_unref(date);
}



int add_file_suite()
{
    UNIT_SUITE_CREATE(file_test_init, file_test_finalize);

    UNIT_TEST_CREATE(file_create);
    UNIT_TEST_CREATE(file_name);
    UNIT_TEST_CREATE(file_open_writing);
    UNIT_TEST_CREATE(file_write_with_elaborate_header_and_signals);
    UNIT_TEST_CREATE(file_open_reading);
    return 0;
}
