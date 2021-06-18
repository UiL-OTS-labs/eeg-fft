
#include <gedf.h>
#include <glib.h>

static const char* test_file_256 = "../biosemi-datafiles/Newtest17-256.bdf";
static const char* test_file_2048 = "../biosemi-datafiles/Newtest17-2048.bdf";

static void
bdf_create()
{
    EdfBdfFile* bdffile = NULL;
    bdffile = edf_bdf_file_new();
    g_assert_nonnull(bdffile);
    g_assert_cmpint(
        ((GTypeInstance*)bdffile)->g_class->g_type,
        ==,
        EDF_TYPE_BDF_FILE
        );
    edf_bdf_file_destroy(bdffile);
}

static void
bdf_open_256(void)
{
    EdfBdfFile* bdffile = NULL;
    EdfFile* edffile;
    GError*     error   = NULL;

    bdffile = edf_bdf_file_new_for_path(test_file_256);
    edffile = EDF_FILE(bdffile);

    edf_file_read(edffile, &error);
    g_assert_no_error(error);

    GPtrArray *signals = edf_file_get_signals(EDF_FILE(bdffile));
    g_assert_nonnull(signals);
    g_assert_true (signals->len > 0);

    g_ptr_array_unref(signals);
    edf_bdf_file_destroy(bdffile);
}

static void
bdf_open_2048(void)
{
    EdfBdfFile* bdffile = NULL;
    GError*     error   = NULL;

    bdffile = edf_bdf_file_new_for_path(test_file_2048);
    edf_file_read(EDF_FILE(bdffile), &error);
    g_assert_no_error(error);

    GPtrArray *signals = edf_file_get_signals(EDF_FILE(bdffile));
    g_assert_nonnull(signals);
    g_assert_true (signals->len > 0);

    g_ptr_array_unref(signals);
    edf_bdf_file_destroy(bdffile);
}

void add_bdf_file_suite(void)
{
    g_test_add_func("/BdfFile/create", bdf_create);
    g_test_add_func("/BdfFile/open_256", bdf_open_256);
    g_test_add_func("/BdfFile/open_2048", bdf_open_2048);
}
