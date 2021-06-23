#include <glib.h>

#ifndef EEG_FFT_EDF_RECORD_PRIV_H
#define EEG_FFT_EDF_RECORD_PRIV_H

G_BEGIN_DECLS

typedef struct _record {
    guint8* bytes;          /* the bytes representing the signal */
    guint   ns;             /* the number of samples in a complete record */
    guint   sizeof_sample;  /* the number of bytes of one sample */
    guint   ns_stored;      /* the number of samples currently stored */
} EdfRecord;

EdfRecord* edf_record_new(guint ns, guint sizeof_sampeles, GError** error);

void edf_record_destroy(EdfRecord* record);

void edf_record_append(EdfRecord* record, gint value);

gsize edf_record_size(const EdfRecord* record);

gint edf_record_get(const EdfRecord* record, gsize index);

G_END_DECLS

#endif //EEG_FFT_EDF_RECORD_PRIV_H
