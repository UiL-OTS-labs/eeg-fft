
#ifndef EDF_BDF_FILE_H
#define EDF_BDF_FILE_H

#include "edf-file.h"

G_BEGIN_DECLS

#define EDF_TYPE_BDF_FILE edf_bdf_file_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(EdfBdfFile, edf_bdf_file, EDF, BDF_FILE, EdfFile)

struct _EdfBdfFileClass {
    EdfFileClass parent_class;
};

G_MODULE_EXPORT EdfBdfFile*
edf_bdf_file_new();

G_MODULE_EXPORT void
edf_bdf_file_destroy(EdfBdfFile* file);

G_MODULE_EXPORT EdfBdfFile*
edf_bdf_file_new_for_path(const char* path);

G_END_DECLS

#endif
