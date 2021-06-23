
#include "edf-bdf-file.h"
#include "edf-bdf-header.h"

/**
 * SECTION:edf-bdf-file
 * @short_description: An object that derives from EdfFile
 * @see_also: #EdfFile, #EdfBdfHeader
 * @include: gedf.h
 *
 * #EdfBdfFile is a file that contains a flavour of an EdfHeader, the main
 * difference is in the header that contains a
 */

G_DEFINE_TYPE(EdfBdfFile, edf_bdf_file, EDF_TYPE_FILE)

static EdfHeader*
alloc_header()
{
    return EDF_HEADER(edf_bdf_header_new());
}

/**
 * edf_bdf_init:
 * @param klass the EdfBdfFileClass
 *
 * the EdfFile should do all the magick.
 *
 * @private
 */
static void
edf_bdf_file_init(EdfBdfFile* self)
{
    // everything is handled in parent init.
    (void) self;
}

static void
edf_bdf_file_class_init(EdfBdfFileClass* klass)
{
    EdfFileClass* edf_file_class = EDF_FILE_CLASS(klass);
    edf_file_class->alloc_header = alloc_header;
}

/**
 * edf_bdf_file_new:constructor
 *
 * Create a new default EdfBdfFile instance
 *
 * Returns :(transfer full): a new #EdfBdfFile instance.
 */
EdfBdfFile*
edf_bdf_file_new()
{
    return g_object_new(EDF_TYPE_BDF_FILE, NULL);
}

/**
 * edf_bdf_file_new_for_path:constructor
 * @path: a utf8 description of the file path.
 *
 * Create a new EdfBdfFile instance the path property
 * will be updated so you can write to or read from a
 * location on disk.
 *
 * Returns :(transfer full): a new #EdfBdfFile instance.
 */
EdfBdfFile*
edf_bdf_file_new_for_path(const char* path)
{
    return g_object_new(EDF_TYPE_BDF_FILE,
                        "path", path,
                        NULL
                        );
}

void
edf_bdf_file_destroy(EdfBdfFile* file)
{
    g_object_unref(file);
}