
#include <CUnit/CUnit.h>
#include <gedf.h>
#include "test-macros.h"

static const char* SUITE_NAME = "EdfHeaderSuite";

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
    CU_ASSERT_PTR_NOT_NULL(header);
    edf_header_destroy(header);
}

static void
header_size(void)
{
    EdfHeader* hdr = edf_header_new();
    guint hdrsz;
    g_object_get(hdr, "num-bytes-header", &hdrsz, NULL);
    CU_ASSERT_EQUAL(hdrsz, bdf_header_size_validator(0));
    
    edf_header_destroy(hdr);
}

static void
header_num_signals(void)
{
    EdfHeader* hdr = edf_header_new();
    //gboolean succes = edf_header_set_num_signals(hdr, 16);
    CU_ASSERT_EQUAL(0, 1); // to remember us that this needs to be implemented or removed
    edf_header_destroy(hdr);
}

int add_header_suite()
{
    UNIT_SUITE_CREATE(NULL, NULL);

    UNIT_TEST_CREATE(header_create);
    UNIT_TEST_CREATE(header_size);
    UNIT_TEST_CREATE(header_num_signals);

    return 0;
}
