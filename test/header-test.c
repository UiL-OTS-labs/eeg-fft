
#include <locale.h>
#include <glib.h>
#include <gedf.h>

/* taken from https://www.biosemi.com/faq/file_format.htm */
static size_t
bdf_header_size_validator(size_t num_signals)
{
    return (num_signals + 1) * 256;
}

static void
header_create(void)
{
    EdfHeader* header = edf_header_new();
    g_assert_nonnull(header);
    edf_header_destroy(header);
}

static void
header_size(void)
{
    EdfHeader* hdr = edf_header_new();
    guint hdrsz;
    g_object_get(hdr, "num-bytes-header", &hdrsz, NULL);
    g_assert_cmpuint(hdrsz, ==, bdf_header_size_validator(0));
    
    edf_header_destroy(hdr);
}

int main(int argc, char** argv)
{
    setlocale(LC_ALL, "");
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/EdfHeader/create", header_create);
    g_test_add_func("/EdfHeader/size",header_size);

    return g_test_run();
}
