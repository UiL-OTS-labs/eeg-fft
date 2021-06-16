
#include <gedf.h>
#include <locale.h>
#include <glib.h>
#include <time.h>
#include <math.h>

/* ************ declarations ********** */

typedef struct FileFixture {
    EdfFile* file;
    gdouble  jitter;
} FileFixture;


/* ******** global constants ********* */

// A default name to use for an edf file
const gchar* default_name = "hello-world.edf";
double g_jitter = 20;

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


static gchar g_temp_dir[1024] = "";
static gchar g_temp_file[1024] = "";

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


static int
file_test_init(void)
{
    gchar template[1024] = "gedf_unit_test_XXXXXX";
    GError *error = NULL;
    char *temp_dir = g_dir_make_tmp(template, &error);

    if (error) {
        g_printerr("Unable to open temp dir: %s\n", error->message);
        g_error_free(error);
        return -1;
    } else {
        g_print("Created directory: %s\n", temp_dir);
    }
    g_snprintf(g_temp_dir, sizeof(g_temp_dir), "%s", temp_dir);
    g_free(temp_dir);
    g_snprintf(g_temp_file, sizeof(g_temp_file), "%s/%s",
               g_temp_dir, default_name);
    return 0;
}

static void
file_fixture_set_up(FileFixture* fixture, gconstpointer data)
{
    (void) data;
    fixture->file = edf_file_new_for_path(g_temp_file);
    EdfHeader* header = NULL;
    GError* error = NULL;

    g_object_get(G_OBJECT(fixture->file),
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
    EdfSignal *signal1 = edf_signal_new();
    EdfSignal *signal2 = edf_signal_new();
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
    edf_file_add_signal(fixture->file, signal1, &error);
    g_assert_no_error(error);
    edf_file_add_signal(fixture->file, signal2, &error);
    g_assert_no_error(error);

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
        error = NULL;
        g_assert_not_reached();
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
        g_error_free(error);
        error = NULL;
        g_assert_not_reached();
    }
    g_object_unref(header);
    g_object_unref(signal1);
    g_object_unref(signal2);
}

static void
file_fixture_tear_down(FileFixture* fixture, gconstpointer unused)
{
    (void) unused;
    g_clear_object(&fixture->file);
}

/* ******* tests ******** */

static void
file_create()
{
    EdfFile* file = NULL;
    file = edf_file_new_for_path(default_name);
    gchar *dirname, *expected_name;
    gchar *output_name = NULL;
    
    dirname = g_get_current_dir();
    expected_name = g_build_filename(dirname, default_name, NULL);
    g_assert_nonnull(file);
    g_assert_true(EDF_IS_FILE(file));

    g_object_get(G_OBJECT(file),
            "path", &output_name,
            NULL
            );
    g_assert_cmpstr(output_name, ==, expected_name);

    g_free(output_name);
    g_free(dirname);
    g_free(expected_name);

    edf_file_destroy(file);
}

static void
file_name()
{
    EdfFile* file = NULL;
    file = edf_file_new(default_name);

    edf_file_set_path(file, g_temp_file);
    char* name_ret = edf_file_get_path(file);

    g_assert_cmpstr(name_ret, ==, g_temp_file);

    g_free(name_ret);

    g_object_unref(file);
}

static void
file_open_writing()
{
    GError  *error = NULL;
    EdfFile *file  = edf_file_new_for_path(g_temp_file);

    edf_file_create(file, &error);
    g_assert_no_error(error);
    if (error) {
        g_printerr("Unable to open '%s':\n\t- %s\n",
                   g_temp_file, error->message);
        g_error_free(error);
        error = NULL;
    }
    
    edf_file_create(file, &error);
    g_assert_error(error, g_io_error_quark(), G_IO_ERROR_EXISTS);
    if (error) {
        g_error_free(error);
        error = NULL;
        edf_file_replace(file, &error);
        g_assert_no_error(error);
        if (error)
            g_error_free(error);
    }

    edf_file_destroy(file);
}

static void
file_write_with_elaborate_header_and_signals(
    FileFixture* fixture,
    gconstpointer unused
    )
{
    (void) unused;
    GError* error = NULL;

    edf_file_replace(fixture->file, &error);
    if (error)
        g_printerr("%s oops: %s", __func__, error->message);
}

static gboolean
check_signal_equality(EdfFile* f1, EdfFile* f2)
{
    gboolean ret = FALSE;
    GPtrArray *sigs1, *sigs2;
    sigs1 = edf_file_get_signals(f1);
    sigs2 = edf_file_get_signals(f2);

    if (sigs1->len != sigs2->len)
        goto end;

    for (size_t i = 0; i < sigs1->len; i++) {
        EdfSignal *sig1, *sig2;
        const char *label1 = NULL, *label2 = NULL;
        const char *trans1= NULL, *trans2 = NULL;
        const char *pdim1= NULL, *pdim2= NULL;
        double pmin1, pmin2;
        double pmax1, pmax2;
        int dmin1, dmin2;
        int dmax1, dmax2;
        const char* prefiltering1, *prefiltering2;
        int ns1, ns2;
        const char* res1, *res2;

        sig1 = g_ptr_array_index(sigs1, i);
        sig2 = g_ptr_array_index(sigs2, i);

        label1 = edf_signal_get_label(sig1);
        label2 = edf_signal_get_label(sig2);

        trans1 = edf_signal_get_transducer(sig1);
        trans2 = edf_signal_get_transducer(sig2);

        pdim1 = edf_signal_get_physical_dimension(sig1);
        pdim2 = edf_signal_get_physical_dimension(sig2);

        pmin1 = edf_signal_get_physical_min(sig1);
        pmin2 = edf_signal_get_physical_min(sig2);

        pmax1 = edf_signal_get_physical_max(sig1);
        pmax2 = edf_signal_get_physical_max(sig2);

        dmin1 = edf_signal_get_digital_min(sig1);
        dmin2 = edf_signal_get_digital_min(sig2);

        dmax1 = edf_signal_get_digital_max(sig1);
        dmax2 = edf_signal_get_digital_max(sig2);

        prefiltering1 = edf_signal_get_prefiltering(sig1);
        prefiltering2 = edf_signal_get_prefiltering(sig2);

        ns1 = edf_signal_get_num_samples_per_record(sig1);
        ns2 = edf_signal_get_num_samples_per_record(sig2);

        res1 = edf_signal_get_reserved(sig1);
        res2 = edf_signal_get_reserved(sig2);

        if (g_strcmp0(label1, label2) != 0)
            goto end;
        if (g_strcmp0(trans1, trans2) != 0)
            goto end;
        if (g_strcmp0(pdim1, pdim2) != 0)
            goto end;

        if (pmin1 != pmin2)
            goto end;
        if (pmax1 != pmax2)
            goto end;
        if (dmin1 != dmin2)
            goto end;
        if (dmax1 != dmax2)
            goto end;

        if(ns1 != ns2)
            goto end;

        if (g_strcmp0(prefiltering1, prefiltering2) != 0)
            goto end;
        if (g_strcmp0(res1, res2) != 0)
            goto end;
    }

    ret = TRUE;
end:
    return ret;
}

/**
 * Reads the file previously written by
 * file_write_with_elaborate_header_and_signals
 * comparsison for equality with the one
 * obtained from the fixture.
 */
static void
file_open_reading(FileFixture* fixture, gconstpointer unused)
{
    (void) unused;
    EdfFile   *file = NULL;
    GError    *error = NULL;
    EdfHeader *hdr = NULL;

    int          version;
    char        *loc_patient = NULL;
    char        *loc_recording = NULL;
    GDateTime   *date = NULL;
    unsigned int expected_header_size;
    char        *reserved = NULL;
    int          num_records;
    double       dur_record;
    unsigned int num_signals;

    file  = edf_file_new_for_path(g_temp_file);

    edf_file_read(file, &error);
    g_assert_no_error(error);

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
    g_assert_cmpint(version, ==, hdr_info.version);
    g_assert_cmpuint(expected_header_size, ==, hdr_info.expected_header_size);
    g_assert_cmpint(num_records, ==, hdr_info.num_records);
    g_assert_cmpfloat(dur_record, ==, hdr_info.dur_record);
    g_assert_cmpuint(num_signals, ==, hdr_info.num_signals);
    g_assert_cmpstr(loc_patient, ==, hdr_info.loc_patient);
    g_assert_cmpstr(loc_recording, ==, hdr_info.loc_recording);
    g_assert_cmpstr(reserved, ==, hdr_info.reserved);

    g_assert_true(check_signal_equality(file, fixture->file));

    g_object_unref(file);
    g_free(loc_patient);
    g_free(loc_recording);
    g_free(reserved);
    if (date)
        g_date_time_unref(date);
}

void file_set_signals(void)
{
    EdfFile* file;
    EdfHeader* header;
    GPtrArray* signals, *sig_file, *sig_header;

    file = edf_file_new();
    signals = g_ptr_array_new();
    g_ptr_array_set_free_func(signals, g_object_unref);
    g_object_get(
        file,
        "header", &header,
        NULL
    );

    for (unsigned i = 0; i < 3; i++) {
        EdfSignal* sig = edf_signal_new();
        g_ptr_array_add (signals, sig);
    }

    edf_file_set_signals(file, signals);

    sig_file = edf_file_get_signals(file);
    sig_header = edf_header_get_signals(header);

    g_assert_true(sig_header == sig_file);

    edf_file_destroy(file);
    edf_header_destroy(header);
    g_ptr_array_unref(signals);
}

void add_file_suite(void)
{
    g_assert_true(file_test_init() == 0);

    g_test_add_func("/EdfFile/create", file_create);
    g_test_add_func("/EdfFile/name", file_name);
    g_test_add_func("/EdfFile/writing", file_open_writing);
    g_test_add(
        "/EdfFile/file_write_with_elaborate_header_and_signals",
        FileFixture,
        NULL,
        file_fixture_set_up,
        file_write_with_elaborate_header_and_signals,
        file_fixture_tear_down
    );
    g_test_add(
        "/EdfFile/open_reading",
        FileFixture,
        NULL,
        file_fixture_set_up,
        file_open_reading,
        file_fixture_tear_down
    );
    g_test_add_func("/EdfFile/set_signals", file_set_signals);
}
