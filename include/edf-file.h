

#ifndef EDF_FILE_H
#define EDF_FILE_H

#include <glib-object.h>
#include <gmodule.h>

#include <edf-signal.h>
#include <edf-header.h>

G_BEGIN_DECLS

#define EDF_TYPE_FILE edf_file_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(EdfFile, edf_file, EDF, FILE, GObject)

struct _EdfFileClass {
    GObjectClass parent_class;
};

/**
 * edf_file_new:(constructor)
 *
 * Returns a new EdfFile.
 */
 G_MODULE_EXPORT EdfFile*
 edf_file_new();

/**
 * edf_file_new_for_path:(constructor)
 * @path :(in): the path for the file in utf8
 *
 * Create a new edf file 
 *
 * Returns: (transfer full): a freshly created file
 */
G_MODULE_EXPORT EdfFile*
edf_file_new_for_path(const char* path);

/**
 * edf_file_destroy:destructor
 * @file:(in): the EdfFile to destroy
 *
 * Decrements the ref count on the file and the file
 * will be destroyed when the count drops to zero.
 * This is equal to calling g_object_unref(G_OBJECT(file));
 */
G_MODULE_EXPORT void
edf_file_destroy(EdfFile* file);

/**
 * edf_file_get_path:
 * @file : (in) : the EdfFile whose name we would like to know
 *
 * Returns :(transfer full): the path of where to open/store
 *                           the contents.
 */
G_MODULE_EXPORT gchar* 
edf_file_get_path(EdfFile* file);

/**
 * edf_file_set_name:
 * @file:(in): The file whose name we want to alter
 * @path:(in): The file path to a name where we can open
 *             a file for reading or writing
 */
G_MODULE_EXPORT void
edf_file_set_path(EdfFile* file, const gchar* path);

/**
 * edf_file_read:
 * @self the EdfFile
 * @error:(out): If an error occurs it is returned here.
 *
 * Opens the file for reading. The property fn should be
 * set to a path of a valid edf file. Otherwise havoc will
 * occur.
 */
G_MODULE_EXPORT gsize
edf_file_read(EdfFile* self, GError** error);

/**
 * edf_file_create:
 * @error:(out): An error will be returned here when the
 *               file cannot be written or it exists.
 *
 * Write all the signals and header to disc. This method
 * will fail if the file already exists or another reason
 * for being unable to read the file.
 */
G_MODULE_EXPORT void
edf_file_create(EdfFile* self, GError** error);

/**
 * edf_file_replace:
 * @error:(out): An error will be returned here when the
 *               file cannot be written
 *
 * The edf file will be written to the currently specified path.
 * If the file exists, it will be overwritten.
 */
G_MODULE_EXPORT void
edf_file_replace(EdfFile* self, GError** error);

/**
 * edf_file_add_signal:
 * @self:The #EdfFile to which you would like to add a signal
 * @signal:(in)(transfer full): The #EdfSignal you would like to add
 *
 * The file will add the signal to its collection of signals, the
 * reference count of the signal is incremented. So you can release
 * your reference to the signals.
 */
G_MODULE_EXPORT void
edf_file_add_signal(EdfFile* self, EdfSignal* signal);

/**
 * edf_file_set_signals:
 * @self:(in) : the #EdfFile to which to set the signals
 * @signals:(in)(element-type EdfSignal)(transfer full):
 */
G_MODULE_EXPORT void
edf_file_set_signals(EdfFile* self, GPtrArray* signals);

/**
 * edf_file_get_signals:
 * @file:: The from which to obtain the signals.
 *
 * Returns:(transfer full)(element-type EdfSignal)
 */
 G_MODULE_EXPORT GPtrArray*
 edf_file_get_signals(EdfFile* file);

/**
 * edf_file_header:
 * @file : (in)
 *
 * Obtains the header of the file.
 *
 * Returns:(transfer none): The header of the file.
 */
G_MODULE_EXPORT EdfHeader*
edf_file_header(EdfFile* file);

G_END_DECLS

#endif
