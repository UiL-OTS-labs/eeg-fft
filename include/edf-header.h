
#ifndef EDF_HEADER_H
#define EDF_HEADER_H

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define EDF_HEADER_ERROR edf_header_error_quark()

/**
 * EdfHeaderError:
 * @EDF_HEADER_ERROR_PARSE: An error while parsing the header
 *
 * An error code returned an operation on an instance of
 * EdfSignal
 */
typedef enum {
    EDF_HEADER_ERROR_PARSE,
    EDF_HEADER_INVALID_VALUE,
}EdfHeaderError;

/*
 * Type Declaration
 */
#define EDF_TYPE_HEADER edf_header_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(EdfHeader, edf_header, EDF, HEADER, GObject)

struct _EdfHeaderClass {
    GObjectClass parent_class;

    // reading a header
    gsize (*read_version) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_patient) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_recording) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_date) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_time) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_num_bytes) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_reserved) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_num_records) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_dur_records) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_num_signals) (EdfHeader* hdr, GInputStream* stream, GError** error);
    gsize (*read_signals) (EdfHeader* hdr, GInputStream* stream, GError** error);

    // Reading the signal related information from the header
    gsize (*read_label) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_transducer) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_phys_dim) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_phys_min) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_phys_max) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_dig_min) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_dig_max) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_prefiltering) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_num_samples_per_rec) (EdfHeader* hdr, GInputStream *stream, GError **error);
    gsize (*read_sig_reserved) (EdfHeader* hdr, GInputStream *stream, GError **error);

};

/**
 * edf_header_new:(constructor)
 *
 * Create a default new Edf header.
 */
G_MODULE_EXPORT EdfHeader*
edf_header_new();

/**
 * edf_header_destroy:(skip)
 * @header :: The header to destroy
 *
 * Decrements the reference of header and will
 * free the associated resources when the refcnt
 * drops to zero.
 */
G_MODULE_EXPORT void
edf_header_destroy(EdfHeader* header);

/**
 * edf_header_set_signals :
 * @hdr :(inout): the header object
 * @signals:(in)(transfer full)(element-type EdfSignal): the number of signals 0 < ns < 9999
 *
 * Set the signals that belong to this header.
 *
 * Returns :: TRUE when the operation was successful
 */
G_MODULE_EXPORT gboolean
edf_header_set_signals(EdfHeader* hdr, GPtrArray* signals);

/**
 * edf_header_write_to_ostream:
 * @hdr:(in):The header to write to the output stream
 * @ostream:(inout):The output stream to which the header should be written.
 * @error:(out):If an error occurs it will be returned here.
 *
 * Writes an header to an output stream.
 *
 * private: for internal use only
 * Returns::the number of written bytes.
 */
gsize
edf_header_write_to_ostream (
        EdfHeader         *hdr,
        GOutputStream     *ostream,
        GError           **error
        );

/**
 * edf_header_read_from_input_stream:
 * @hdr: An #EdfHeader instance
 * @inputStream :(in)(out): A GInputStream to read
 *                          the header from
 * @error: (out): An error might be returned here
 *
 * Read an header from a input stream.
 *
 * Returns: the number of bytes read.
 */
gsize
edf_header_read_from_input_stream(
        EdfHeader     *hdr,
        GInputStream  *inputStream,
        GError       **error
        );

G_MODULE_EXPORT gint
edf_header_get_version(EdfHeader* header);

G_MODULE_EXPORT const gchar*
edf_header_get_patient(EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_set_patient (EdfHeader   *header, const gchar *patient);

G_MODULE_EXPORT const gchar*
edf_header_get_recording();

G_MODULE_EXPORT gboolean
edf_header_set_recording (EdfHeader *header, const gchar *recording);

G_MODULE_EXPORT GDateTime*
edf_header_get_time (EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_set_time(EdfHeader* header, GDateTime* time);

G_MODULE_EXPORT int
edf_header_num_bytes(EdfHeader* header);

G_MODULE_EXPORT const char*
edf_header_get_reserved(EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_set_reserved(EdfHeader* header, const gchar* reserved);

G_MODULE_EXPORT gint
edf_header_get_num_records(EdfHeader* header);

G_MODULE_EXPORT gdouble
edf_header_get_record_duration(EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_set_record_duration(
    EdfHeader  *header,
    gdouble     duration
);

G_MODULE_EXPORT guint
edf_header_get_num_signals(EdfHeader* header);


/****** utility functions not directly related to EdfHeader instances ******* */

/**
 * edf_compute_header_size
 * @num_signals :: number of signals in the header
 *
 * Computes the header size given the number of signals
 * in the header.
 *
 * The size of the header is 256 bytes for the static part
 * and bytes 256 for each signal.
 *
 * So it should return 256 + 256 * ns;
 *
 * Returns : the number of bytes that the header will take.
 */
G_MODULE_EXPORT gsize
edf_compute_header_size(guint num_signals);
G_END_DECLS

// #ifndef EDF_HEADER_H
#endif
