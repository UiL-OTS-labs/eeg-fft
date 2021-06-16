
#include <glib.h>
#include <locale.h>
#include <gedf.h>


/* ************ utilities ****************** */

gboolean cmp_double_garray(GArray* a1, GArray* a2)
{
    if (a1->len != a2->len)
        return FALSE;

    for (gsize i = 0 ; i < a1->len; i++) {
        if (g_array_index(a1, double, i) != g_array_index(a2, double, i))
            return FALSE;
    }

    return TRUE;
}

/* ************ tests ****************** */

static void
signal_create(void)
{
    EdfSignal* signal = edf_signal_new();
    g_assert_nonnull(signal);
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

    g_assert_cmpstr(label, ==, label_ret);
    g_assert_cmpstr(transducer, ==, transducer_ret);
    g_assert_cmpstr(phys_dim, ==, phys_dim_ret);
    g_assert_cmpfloat(phys_min, ==, phys_min_ret);
    g_assert_cmpfloat(phys_max, ==, phys_max_ret);
    g_assert_cmpint(dig_min, ==, dig_min_ret);
    g_assert_cmpint(dig_max, ==, dig_max_ret);
    g_assert_cmpstr(prefilter, ==, prefilter_ret);
    g_assert_cmpuint(ns, ==, ns_ret);

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
    g_assert_cmpuint(edf_signal_get_num_records(signal), ==, 0);

    for (gint s = 0; s < ns_insert; s++) {
        edf_signal_append_digital(signal, s % 1024, &error);
        if (error != NULL)
            break;
    }
    g_assert_no_error(error);
    if (error) {
        g_printerr("%s: Unexpected error occurred: %s", __func__, error->message);
        g_clear_error(&error);
    }

    g_assert_cmpuint(edf_signal_get_num_records(signal), ==, num_records_expected);

    // the previous record should be full, thus this should force another record
    edf_signal_append_digital(signal, 0, &error);
    g_assert_cmpuint(edf_signal_get_num_records(signal), ==, num_records_expected + 1);

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
    g_assert_error(error, EDF_SIGNAL_ERROR, EDF_SIGNAL_ERROR_OUT_OF_RANGE);
    if (error)
        g_clear_error(&error);

    edf_signal_append_digital(signal, -1, &error);
    g_assert_error(error, EDF_SIGNAL_ERROR, EDF_SIGNAL_ERROR_OUT_OF_RANGE);
    if (error)
        g_clear_error(&error);

    g_object_unref(signal);
}

static void
signal_get_signal()
{
    const gsize size = 1024;
    GError* error = NULL;
    EdfSignal *sig = edf_signal_new_full(
        "blaat", "","",0,1024, 0, 1024, "", size
        );
    edf_signal_set_num_samples_per_record(sig, 1024);
    GArray *input  = g_array_sized_new( FALSE, FALSE, sizeof(double), size);
    for (gsize i = 0; i < size; i++) {
        gdouble d = (double)i;
        g_array_append_val(input, d);
    }

    for (gsize i = 0; i < input->len; i++) {
        gint dig_value = (gint) g_array_index(input, double, i);
        edf_signal_append_digital(sig, dig_value, &error);
    }

    GArray* output = edf_signal_get_values(sig);

    g_assert_true (cmp_double_garray(input, output));

    g_array_unref(input);
    g_array_unref(output);
    edf_signal_destroy(sig);
}


void add_signal_suite()
{
    g_test_add_func("/EdfSignal/create", signal_create);
    g_test_add_func("/EdfSignal/create-full",signal_create_full);
    g_test_add_func("/EdfSignal/append_digital",signal_append_digital);
    g_test_add_func("/EdfSignal/append_digital_range_error",
                    signal_append_digital_range_error);
    g_test_add_func("/EdfSignal/get_vals", signal_get_signal);
}
