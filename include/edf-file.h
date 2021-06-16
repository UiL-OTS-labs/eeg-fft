

#ifndef EDF_FILE_H
#define EDF_FILE_H

#include <glib-object.h>
#include <gmodule.h>

#include <edf-signal.h>
#include <edf-header.h>

G_BEGIN_DECLS

#define EDF_FILE_ERROR edf_file_error_quark()
G_MODULE_EXPORT GQuark edf_file_error_quark();

typedef enum {
    EdfFileErrorSignal,  // There is an mismatch between FileType and
                         // The kind of Signal.
    EdfFileErrorHeader,  // There is an mismatch between FileType and
                         // The header that is added.
} EdfFileError;

#define EDF_TYPE_FILE edf_file_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(EdfFile, edf_file, EDF, FILE, GObject)

struct _EdfFileClass {
    GObjectClass parent_class;

    /*Allows deriving instances to create their own header (eeg Bdf)*/
    EdfHeader* (*alloc_header)();

    /* adds signal and checks whether to signal is appropriate */
    gboolean (*add_signal) (EdfFile* self, EdfSignal* signal, GError** error);

    gpointer reserved[16];
};

G_MODULE_EXPORT EdfFile*
edf_file_new();

G_MODULE_EXPORT EdfFile*
edf_file_new_for_path(const char* path);

G_MODULE_EXPORT void
edf_file_destroy(EdfFile* file);

G_MODULE_EXPORT gchar* 
edf_file_get_path(EdfFile* file);

G_MODULE_EXPORT void
edf_file_set_path(EdfFile* file, const gchar* path);

G_MODULE_EXPORT gsize
edf_file_read(EdfFile* self, GError** error);

G_MODULE_EXPORT void
edf_file_create(EdfFile* self, GError** error);

G_MODULE_EXPORT void
edf_file_replace(EdfFile* self, GError** error);

G_MODULE_EXPORT void
edf_file_add_signal(EdfFile* self, EdfSignal* signal, GError** error);

G_MODULE_EXPORT void
edf_file_set_signals(EdfFile* self, GPtrArray* signals);

G_MODULE_EXPORT GPtrArray*
edf_file_get_signals(EdfFile* file);

G_MODULE_EXPORT guint
edf_file_get_num_signals(EdfFile* file);

G_MODULE_EXPORT EdfHeader*
edf_file_header(EdfFile* file);

G_END_DECLS

#endif
