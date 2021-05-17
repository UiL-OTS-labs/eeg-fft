
#ifndef EDF_SIGNAL_H
#define EDF_SIGNAL_H

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define EDF_SIGNAL_ERROR edf_signal_error_quark()

/**
 * EdfSignalError:
 * @EDF_SIGNAL_ERROR_ENOMEM: an memory allocation failed
 * @EDF_SIGNAL_ERROR_INVALID_INDEX: Trying to index a sample that is out of bounds
 * @EDF_SIGNAL_ERROR_OUT_OF_RANGE: A sample beyond the digital minimum or maximum was added.
 * @EDF_SIGNAL_ERROR_FAILED: An unspecific error occurred.
 *
 * An error code returned an operation on an instance of
 * EdfSignal
 */
typedef enum {
    EDF_SIGNAL_ERROR_ENOMEM,
    EDF_SIGNAL_ERROR_INVALID_INDEX,
    EDF_SIGNAL_ERROR_OUT_OF_RANGE,
    EDF_SIGNAL_ERROR_FAILED,
}EdfSignalError;


/*
 * Type Declaration
 */
#define EDF_TYPE_SIGNAL edf_signal_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(EdfSignal, edf_signal, EDF, SIGNAL, GObject)

struct _EdfSignalClass {
    GObjectClass parent_class;
};

G_MODULE_EXPORT GQuark
edf_signal_error_quark(void);

G_MODULE_EXPORT EdfSignal*
edf_signal_new();

G_MODULE_EXPORT EdfSignal*
edf_signal_new_full(
        const gchar*    label,
        const gchar*    transducer,
        const gchar*    phys_dim,
        gdouble         physical_min,
        gdouble         physical_max,
        gint            digital_min,
        gint            digital_max,
        const gchar*    prefiltering,
        guint           ns
        );

G_MODULE_EXPORT void
edf_signal_destroy(EdfSignal* signal);

G_MODULE_EXPORT const gchar*
edf_signal_get_label(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_label(EdfSignal* signal, const gchar* label);

G_MODULE_EXPORT const gchar*
edf_signal_get_transducer(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_transducer(EdfSignal* signal, const gchar* transducer);

G_MODULE_EXPORT const gchar*
edf_signal_get_physical_dimension(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_physical_dimension(EdfSignal* signal, const gchar* dimension);

G_MODULE_EXPORT gdouble
edf_signal_get_physical_min(EdfSignal* signal);

 G_MODULE_EXPORT void
 edf_signal_set_physical_min(EdfSignal* signal, gdouble min);

G_MODULE_EXPORT gdouble
edf_signal_get_physical_max(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_physical_max(EdfSignal* signal, gdouble max);

G_MODULE_EXPORT gint
edf_signal_get_digital_min(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_digital_min(EdfSignal* signal, gint min);

G_MODULE_EXPORT gint
edf_signal_get_digital_max(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_digital_max(EdfSignal* signal, gint min);

G_MODULE_EXPORT const gchar*
edf_signal_get_prefiltering(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_prefiltering(EdfSignal* signal, const gchar* prefilter);

 G_MODULE_EXPORT gint
 edf_signal_get_num_samples_per_record(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_set_num_samples_per_record(EdfSignal* signal, guint num_samples);

G_MODULE_EXPORT const char*
edf_signal_get_reserved(EdfSignal* signal);

 G_MODULE_EXPORT void
 edf_signal_set_reserved(EdfSignal* signal, const gchar* reserved);

 G_MODULE_EXPORT guint
 edf_signal_get_num_records(EdfSignal* signal);

G_MODULE_EXPORT void
edf_signal_append_digital(EdfSignal* signal, gint value, GError** error);

GArray*
edf_signal_get_values(EdfSignal* signal);

void edf_signal_write_record_to_ostream(
        EdfSignal      *signal,
        GOutputStream  *ostream,
        guint           nrec,
        GError        **error
        );

gsize
edf_signal_read_record_from_istream (
    EdfSignal      *signal,
    GInputStream   *ostream,
    GError        **error
);


G_END_DECLS

// #ifndef EDF_SIGNAL_H
#endif
