/*
 * Signals are records after each other. In each
 * record there are a number of samples see
 * _EdfSignalPrivate, the num_samples_per_record (ns)
 * member.
 * This means that for each record ns bytes per sample
 * are recorded.
 */

#include "edf-signal.h"
#include "edf-record-priv.h"

#include <errno.h>

typedef union {
    gint16  i16;
    guint16 u16;
    gint32  i32;
    guint32 u32;
    guint8  array[4];
} sample_repr;

EdfRecord*
edf_record_new(guint ns, guint sizeof_sample, GError** error) {
    g_return_val_if_fail(error != NULL || *error, NULL);
    EdfRecord* new = malloc(sizeof(EdfRecord));
    if (!new) {
        int saved_errno = errno;
        if (saved_errno == ENOMEM)
            g_set_error(
                    error,
                    EDF_SIGNAL_ERROR,
                    EDF_SIGNAL_ERROR_ENOMEM,
                    "Unable to create record: %s",
                    g_strerror(saved_errno)
                    );
        else
            g_set_error_literal(
                    error,
                    EDF_SIGNAL_ERROR,
                    EDF_SIGNAL_ERROR_FAILED,
                    "Unable to allocate EdfRecord"
                    );

        return new;
    }

    new->ns_stored = 0;
    new->sizeof_sample = sizeof_sample;
    new->ns = ns;
    new->bytes = calloc(ns , sizeof_sample);
    if (!new->bytes) {
        int saved_errno = errno;
        if (saved_errno == ENOMEM)
            g_set_error(
                    error,
                    EDF_SIGNAL_ERROR,
                    EDF_SIGNAL_ERROR_ENOMEM,
                    "Unable to create record: %s",
                    g_strerror(saved_errno)
                    );
        else
            g_set_error_literal(
                    error,
                    EDF_SIGNAL_ERROR,
                    EDF_SIGNAL_ERROR_FAILED,
                    "Unable to allocate EdfRecord"
                    );

        g_free(new);
        new = NULL;
    }

    return new;
}

void
edf_record_destroy(EdfRecord* record)
{
    g_free(record->bytes);
    g_free(record);
}

gsize
edf_record_size(const EdfRecord* record)
{
    return record->ns_stored;
}

void
edf_record_append(EdfRecord* record, gint value)
{
    sample_repr repr;
    const gsize size24 = 3;

    if (record->sizeof_sample == sizeof(gint16)) { // Edf or Edf+ sample format
        repr.i16 = GINT16_TO_LE(value);
        memcpy(&record->bytes[record->sizeof_sample * record->ns_stored],
               repr.array,
               sizeof(repr.i16)
        );
    }
    else if (record->sizeof_sample == size24) { // BdfFormat signed 24bit little endian
        guint8 temp[3] = {0};
        temp [0]  = (value & 0x000000FF) >> 0;  // least significant byte
        temp [1]  = (value & 0x0000FF00) >> 8;
        // Store most significant byte omit the location for signbit.
        temp [2]  = (value & 0x007F0000) >> 16;
        // Pull the signbit from the int and store it in the previously omitted bit.
        temp [2] |= (value & 0x80000000) >> 24;
        memcpy(&record->bytes[record->sizeof_sample * record->ns_stored],
               temp,
               size24
        );
    }
    else { // unknown unhandled format.
        g_assert_not_reached();
    }
    record->ns_stored++;
}

gint
edf_record_get(const EdfRecord* record, gsize index)
{
    sample_repr repr;
    const gsize size24 = 3;
    gint value = 0;

    if (record->sizeof_sample == sizeof(gint16)) {
        memcpy(
            repr.array,
            &record->bytes[index * record->sizeof_sample],
            record->sizeof_sample
        );
        value = GINT_FROM_LE(repr.i16);
    }
    else if (record->sizeof_sample == size24) {
        guint8 temp[3] = {0};
        memcpy(&temp[0], &record->bytes[record->sizeof_sample * index], size24);
        value |= temp[0] << 0; // extract least significant byte
        value |= temp[1] << 8;
        // extract most significant byte omit sign
        value |= (temp[2] & 0x7F) << 16;
        // extract sign bit
        value |= (temp[2] & 0x80) << 24;
    }
    return value;
}

