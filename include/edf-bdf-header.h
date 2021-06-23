
#ifndef EDF_BDF_HEADER_H
#define EDF_BDF_HEADER_H

#include "edf-header.h"

G_BEGIN_DECLS

#define EDF_TYPE_BDF_HEADER edf_bdf_header_get_type()

G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(
    EdfBdfHeader, edf_bdf_header, EDF, BDF_HEADER, EdfHeader
)

struct _EdfBdfHeaderClass {
    EdfHeaderClass parent_class;
};

G_MODULE_EXPORT EdfBdfHeader*
edf_bdf_header_new();

G_MODULE_EXPORT void
edf_bdf_header_destroy(EdfBdfHeader *self);

G_END_DECLS

#endif
