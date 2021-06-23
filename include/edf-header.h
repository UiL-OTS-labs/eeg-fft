
#ifndef EDF_HEADER_H
#define EDF_HEADER_H

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>
#include "edf-signal.h"

G_BEGIN_DECLS

#define EDF_HEADER_ERROR edf_header_error_quark()
G_MODULE_EXPORT GQuark edf_header_error_quark();

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

    EdfSignal* (*alloc_signal)();

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

    // writing a header
    gsize (*write_version) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_patient) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_recording) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_date) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_time) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_num_bytes) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_reserved) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_num_records) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_dur_records) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_num_signals) (EdfHeader* hdr, GOutputStream* stream, GError** error);
    gsize (*write_signals) (EdfHeader* hdr, GOutputStream* stream, GError** error);

    // Write the signal related information from the header
    gsize (*write_label) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_transducer) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_phys_dim) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_phys_min) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_phys_max) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_dig_min) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_dig_max) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_prefiltering) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_num_samples_per_rec) (EdfHeader* hdr, GOutputStream *stream, GError **error);
    gsize (*write_sig_reserved) (EdfHeader* hdr, GOutputStream *stream, GError **error);

};

G_MODULE_EXPORT EdfHeader*
edf_header_new();

G_MODULE_EXPORT void
edf_header_destroy(EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_set_signals(EdfHeader* hdr, GPtrArray* signals);

gsize
edf_header_write_to_ostream (
        EdfHeader         *hdr,
        GOutputStream     *ostream,
        GError           **error
        );

gsize
edf_header_read_from_input_stream(
        EdfHeader     *hdr,
        GInputStream  *inputStream,
        GError       **error
        );

G_MODULE_EXPORT gint
edf_header_get_version(EdfHeader* header);

void
edf_header_set_version(EdfHeader* header, gint version);

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
edf_header_get_num_bytes(EdfHeader* header);

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

G_MODULE_EXPORT GPtrArray*
edf_header_get_signals(EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_is_edf(EdfHeader* header);

G_MODULE_EXPORT gboolean
edf_header_is_edfplus(EdfHeader* header);

/****** utility functions not directly related to EdfHeader instances ******* */

G_MODULE_EXPORT gsize
edf_compute_header_size(guint num_signals);
G_END_DECLS

// #ifndef EDF_HEADER_H
#endif
