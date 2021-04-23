
#include <CUnit/CUnit.h>
#include <gedf.h>
#include "edf-signal.h"
#include "test-macros.h"

static const char* SUITE_NAME = "EdfSignalSuite";

static void
signal_create(void)
{
    EdfSignal* signal = edf_signal_new();
    CU_ASSERT_PTR_NOT_NULL(signal);
    edf_signal_destroy(signal);
}

static void
signal_create_full(void)
{
    const gchar* label = "Eeg";
    const gchar* label_ret = NULL;
    const gchar* transducer = "Active Electrode";
    const gchar* transducer_ret = NULL;
    const gchar* phys_dim = "uV";
    const gchar* phys_dim_ret = NULL;
    gdouble phys_min = 3.1415, phys_min_ret;
    gdouble phys_max = 2 * 3.1415, phys_max_ret;
    gint dig_min = 0, dig_min_ret;
    gint dig_max = 1023, dig_max_ret;
    const gchar* prefilter = "HP:0.1Hz";
    const gchar* prefilter_ret = NULL;
    guint ns = 100, ns_ret;

    EdfSignal* signal = edf_signal_new_full(
            label,
            transducer,
            phys_dim,
            phys_min,
            phys_max,
            dig_min,
            dig_max,
            prefilter,
            ns
            );

    g_object_get(G_OBJECT(signal),
            "physical_min", &phys_min_ret,
            "physical_max", &phys_max_ret,
            "digital_min", &dig_min_ret,
            "digital_max", &dig_max_ret,
            "ns", &ns_ret,
            NULL
            );

    label_ret = edf_signal_get_label(signal);
    transducer_ret = edf_signal_get_transducer(signal);
    phys_dim_ret = edf_signal_get_physical_dimension(signal);
    prefilter_ret = edf_signal_get_prefiltering(signal);

    CU_ASSERT_STRING_EQUAL(label, label_ret);
    CU_ASSERT_STRING_EQUAL(transducer, transducer_ret);
    CU_ASSERT_STRING_EQUAL(phys_dim, phys_dim_ret);
    CU_ASSERT_EQUAL(phys_min, phys_min_ret);
    CU_ASSERT_EQUAL(phys_max, phys_max_ret);
    CU_ASSERT_EQUAL(dig_min, dig_min_ret);
    CU_ASSERT_EQUAL(dig_max, dig_max_ret);
    CU_ASSERT_STRING_EQUAL(prefilter, prefilter_ret);
    CU_ASSERT_EQUAL(ns, ns_ret);

    edf_signal_destroy(signal);
}

static void
signal_append_digital(void)
{
    const gchar* label = "Eeg";
    const gchar* transducer = "Active Electrode";
    const gchar* phys_dim = "uV";
    gdouble phys_min = 3.1415;
    gdouble phys_max = 2 * 3.1415;
    gint dig_min = 0;
    gint dig_max = 1023;
    const gchar* prefilter = "";
    guint ns = 1000;
    const gint ns_insert = 20000;
    const unsigned num_records_expected = ns_insert / ns;
    
    GError *error = NULL;

    EdfSignal* signal = edf_signal_new_full(
            label,
            transducer,
            phys_dim,
            phys_min,
            phys_max,
            dig_min,
            dig_max,
            prefilter,
            ns
            );
    // An empty signal should hold no records.
    CU_ASSERT_EQUAL(edf_signal_get_num_records(signal), 0);

    for (gint s = 0; s < ns_insert; s++) {
        edf_signal_append_digital(signal, s % 1024, &error);
        if (error != NULL)
            break;
    }
    CU_ASSERT_PTR_NULL(error);
    if (error) {
        g_printerr("%s: Unexpected error occurred: %s", __func__, error->message);
        g_clear_error(&error);
    }

    CU_ASSERT_EQUAL(edf_signal_get_num_records(signal), num_records_expected);

    // the previous record should be full, thus this should force another record
    edf_signal_append_digital(signal, 0, &error);
    CU_ASSERT_EQUAL(edf_signal_get_num_records(signal), num_records_expected + 1);

    edf_signal_destroy(signal);
}

static void
signal_append_digital_range_error(void)
{
    const gchar* label = "Eeg";
    const gchar* transducer = "Active Electrode";
    const gchar* phys_dim = "uV";
    gdouble phys_min = 3.1415;
    gdouble phys_max = 2 * 3.1415;
    gint dig_min = 0;
    gint dig_max = 1023;
    const gchar* prefilter = "";
    guint ns = 1000;
    
    GError *error = NULL;

    EdfSignal* signal = edf_signal_new_full(
            label,
            transducer,
            phys_dim,
            phys_min,
            phys_max,
            dig_min,
            dig_max,
            prefilter,
            ns
            );
    
    edf_signal_append_digital(signal, 1024, &error);
    CU_ASSERT_PTR_NOT_NULL(error);
    if (error) {
        CU_ASSERT_EQUAL(error->domain, EDF_SIGNAL_ERROR);
        CU_ASSERT_EQUAL(error->code, EDF_SIGNAL_ERROR_OUT_OF_RANGE);
        g_clear_error(&error);
    }


    edf_signal_append_digital(signal, -1, &error);
    CU_ASSERT_PTR_NOT_NULL(error);
    if (error) {
        CU_ASSERT_EQUAL(error->domain, EDF_SIGNAL_ERROR);
        CU_ASSERT_EQUAL(error->code, EDF_SIGNAL_ERROR_OUT_OF_RANGE);
        g_clear_error(&error);
    }

    g_object_unref(signal);
}


int add_signal_suite()
{
    UNIT_SUITE_CREATE(NULL, NULL);

    UNIT_TEST_CREATE(signal_create);
    UNIT_TEST_CREATE(signal_create_full);
    UNIT_TEST_CREATE(signal_append_digital);
    UNIT_TEST_CREATE(signal_append_digital_range_error);

    return 0;
}
