
#include "edf-bdf-header.h"
#include "edf-header.h"
#include "edf-bdf-signal.h"
#include <edf-size-priv.h>


G_DEFINE_TYPE(EdfBdfHeader, edf_bdf_header, EDF_TYPE_HEADER)

static unsigned char const BDF_VERSION[EDF_VERSION_SZ + 1] = {
    255,
    'B',
    'I',
    'O',
    'S',
    'E',
    'M',
    'I',
    '\0'
};

static void
edf_bdf_header_init(EdfBdfHeader* self)
{
    (void) self;
    // TODO figure out whether we need to chain up to parent.
}

static EdfSignal*
alloc_signal()
{
    return g_object_new(EDF_TYPE_BDF_SIGNAL, NULL);
}

static size_t
read_version(EdfHeader* self, GInputStream* stream, GError **error)
{
    unsigned char temp[EDF_VERSION_SZ + 1] = {0};

    gsize nread;
    if (g_input_stream_read_all(
            stream, temp, EDF_VERSION_SZ, &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }

    if (memcmp(BDF_VERSION, temp, EDF_VERSION_SZ) != 0) {
        g_set_error(
            error, EDF_HEADER_ERROR, EDF_HEADER_ERROR_PARSE,
            "Version in header is not BDF %s", temp);
        return nread;
    }

    edf_header_set_version(self, -255);
    return nread;
}

static size_t
write_version(EdfHeader* self, GOutputStream* stream, GError **error)
{
    (void) self; // The contents of self are irrelevant.
    gsize nwritten;

    g_output_stream_write_all(
        stream, BDF_VERSION, EDF_VERSION_SZ, &nwritten, NULL, error
    );
    return nwritten;
}

static size_t
read_reserved(EdfHeader* self, GInputStream* stream, GError **error) {
    gsize nread;
    char temp[EDF_RESERVED_SZ + 1] = {0};
    char expected[EDF_RESERVED_SZ + 1] = {
        '2', '4', 'B', 'I', 'T', 0
    };
    for (size_t i = strlen(expected); i < EDF_RESERVED_SZ; ++i) {
        expected[i] = ' ';
    }
    if (
        g_input_stream_read_all(
            stream, temp, EDF_RESERVED_SZ, &nread, NULL, error
            ) != TRUE
    ) {
        return nread;
    }
    if (g_strcmp0(temp, expected) != 0) {
        g_set_error(error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
                    "Reserved in header was not BDF \"%s\"", temp
                    );
    }
    edf_header_set_reserved(self, temp);
    return nread;
}

static size_t
write_reserved(EdfHeader* self, GOutputStream *stream, GError **error)
{
    (void) self;
    gsize nwritten;
    char reserved[EDF_RESERVED_SZ + 1] = {
        '2', '4', 'B', 'I', 'T', 0
    };
    for (size_t i = strlen(reserved); i < EDF_RESERVED_SZ; ++i) {
        reserved[i] = ' ';
    }

    g_output_stream_write_all(
        stream, reserved, EDF_RESERVED_SZ, &nwritten, NULL, error
    );

    return nwritten;
}

static void
edf_bdf_header_class_init(EdfBdfHeaderClass* klass)
{
    GObjectClass   *object_class = G_OBJECT_CLASS(klass);
    EdfHeaderClass *edf_header_class = EDF_HEADER_CLASS(object_class);

    edf_header_class->alloc_signal = alloc_signal;

    edf_header_class->read_version = read_version;
    edf_header_class->write_version = write_version;

    edf_header_class->read_reserved = read_reserved;
    edf_header_class->write_reserved = write_reserved;
}

/* ********* public functions ************ */

/**
 * edf_bdf_header_new:
 *
 * @return
 */
EdfBdfHeader*
edf_bdf_header_new()
{
    return g_object_new(EDF_TYPE_BDF_HEADER, NULL);
}

void
edf_bdf_header_destroy(EdfBdfHeader* self)
{
    g_return_if_fail(EDF_IS_BDF_HEADER(self));
    g_object_unref(self);
}
