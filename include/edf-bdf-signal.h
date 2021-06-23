
#ifndef EDF_BDF_SIGNAL_H
#define EDF_BDF_SIGNAL_H

#include "edf-signal.h"

G_BEGIN_DECLS

#define EDF_TYPE_BDF_SIGNAL edf_bdf_signal_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(
    EdfBdfSignal, edf_bdf_signal, EDF, BDF_SIGNAL, EdfSignal
)

struct _EdfBdfSignalClass {
    EdfSignalClass parent_class;
};


G_END_DECLS

#endif
