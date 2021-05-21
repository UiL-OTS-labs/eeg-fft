
#include "edf-header.h"
#include "edf-signal.h"
#include "glibconfig.h"
#include <glib.h>
#include <string.h>
#include "edf-size-priv.h"
#include <stdio.h>

/**
 * EdfHeader:
 *
 * The Edfheader is a instance that holds some of the
 * meta data about an (.edf) European Data Format file.
 */

/* **** error handing **** */

G_DEFINE_QUARK(edf_header_error_quark, edf_header_error)

/* ****** utility functions ********** */

typedef struct _EdfHeaderPrivate {
    /* recording info. */
    guint32     version;
    GString    *local_patient_identification;
    GString    *local_recording_identification;
    GDateTime  *date_and_time;
    gint        num_records;
    gdouble     duration_of_record;

    GString    *reserved;

    GPtrArray  *signals;
} EdfHeaderPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(EdfHeader, edf_header, G_TYPE_OBJECT)

/* *********** helpers for the properties ************* */
static void
set_patient_identification(EdfHeader* hdr, const gchar* info)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private (hdr);
    char *copy = g_strdup (info);
    char *stripped = g_strstrip(copy);

    g_string_assign (priv->local_patient_identification, stripped);
    g_free (copy);
}

static void
set_recording_identification(EdfHeader* hdr, const gchar* info)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(hdr);
    char* copy = g_strdup(info);
    char* stripped = g_strstrip(copy);

    g_string_assign (priv->local_recording_identification, stripped);
    g_free (copy);
}

static void
set_date_ymd(EdfHeader* hdr, int year, int month, int day)
{
    gint hour, minute, second;
    GDateTime *date;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    g_return_if_fail(year >= 0 && year < 100);
    g_return_if_fail(month > 0 && month <= 12);
    g_return_if_fail(day > 0 && day <= 31);

    if (year < 85) {
        year += 2000;
    }
    else {
        year += 1900;
    }
    hour = g_date_time_get_hour(priv->date_and_time);
    minute = g_date_time_get_hour(priv->date_and_time);
    second = g_date_time_get_hour(priv->date_and_time);

    date = g_date_time_new_local(year, month, day, hour, minute, second);
    if (!date)
        g_return_if_reached();

    g_date_time_unref(priv->date_and_time);
    priv->date_and_time = date;
}

static void
set_reserved(EdfHeader* hdr, const gchar* str)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char temp[45];
    g_snprintf (temp, sizeof(temp), "%s", str);
    g_strstrip(temp);
    g_string_printf(priv->reserved, "%s", temp);
}

static void
set_time_hms(EdfHeader* hdr, int hour, int minute, int second)
{
    gint year, month, day;
    GDateTime *date;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    g_return_if_fail(hour >= 0 && hour < 24);
    g_return_if_fail(minute >= 0 && minute < 60);
    g_return_if_fail(second >= 0 && second < 60);

    g_date_time_get_ymd(priv->date_and_time, &year, &month, &day);

    date = g_date_time_new_local( year, month, day, hour, minute, second);
    if (!date)
        g_return_if_reached();

    g_date_time_unref(priv->date_and_time);
    priv->date_and_time = date;
}

/*
 * clear the signals and reset them for a new number typically
 * used when reading a header from an input stream.
 */
static void
set_num_signals(EdfHeader* hdr, gint num_signals)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(hdr);
    g_return_if_fail(num_signals >= 0 && num_signals < 9999);

    g_ptr_array_set_size(priv->signals, num_signals);
    for (gsize i = 0; i < (gsize)num_signals; i++) {
        EdfSignal* signal = edf_signal_new();
        g_ptr_array_index(priv->signals, i) = signal;
    }
}

/* ********* functions to read/write an header ********** */

static gsize
read_version (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char temp[256];
    gint val;
    gsize nread = 0;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    if (
        g_input_stream_read_all(
            stream, temp, EDF_VERSION_SZ, &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_VERSION_SZ] = '\0';

    val = g_ascii_strtoll(temp, NULL, 10);

    priv->version = val;
    return nread;
}

static gsize
write_version(EdfHeader* hdr, GOutputStream* stream, GError** error)
{
    gsize sz;
    char temp[EDF_VERSION_SZ + 1];
    char buffer[EDF_VERSION_SZ];
    EdfHeaderPrivate* priv = edf_header_get_instance_private(hdr);

    // Write version
    memset (buffer, ' ', EDF_VERSION_SZ);
    g_snprintf(temp, sizeof(temp), "%d", priv->version);
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_VERSION_SZ)
    );

    g_output_stream_write_all(
        stream, buffer, EDF_VERSION_SZ, &sz, NULL, error
    );

    return sz;
}

static gsize
read_patient (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char temp[256];
    gsize nread = 0;

    if (
        g_input_stream_read_all(
            stream, temp, EDF_LOCAL_PATIENT_SZ, &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_LOCAL_PATIENT_SZ] = '\0';
    if (!edf_header_set_patient (hdr, temp)) {
        g_set_error (error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
                     "'%s' is invalid local patient info",
                     temp
                     );
    }
    return nread;
}

static gsize
write_patient(EdfHeader* header, GOutputStream* stream, GError** error)
{
    gsize sz;
    EdfHeaderPrivate* priv = edf_header_get_instance_private(header);
    char buffer[EDF_LOCAL_PATIENT_SZ];
    memset(buffer, ' ', EDF_LOCAL_PATIENT_SZ);
    memcpy(buffer,
           priv->local_patient_identification->str,
           MIN(priv->local_patient_identification->len, EDF_LOCAL_PATIENT_SZ)
    );

    g_output_stream_write_all(
        stream, buffer, EDF_LOCAL_PATIENT_SZ, &sz, NULL, error
    );
    return sz;
}


static gsize
read_recording (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char temp[256];
    gsize nread = 0;

    if (
        g_input_stream_read_all(
            stream, temp, EDF_LOCAL_RECORDING_SZ, &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_LOCAL_RECORDING_SZ] = '\0';
    if (!edf_header_set_recording(hdr, temp)) {
        g_set_error (error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
                     "'%s' is invalid local recording info",
                     temp
        );
    }
    return nread;
}

static gsize
write_recording(EdfHeader* hdr, GOutputStream* stream, GError** error)
{
    gsize sz;
    char buffer[EDF_LOCAL_RECORDING_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    memset(buffer, ' ', EDF_LOCAL_RECORDING_SZ);
    memcpy(buffer,
           priv->local_recording_identification->str,
           MIN(priv->local_recording_identification->len, EDF_LOCAL_RECORDING_SZ)
    );

    g_output_stream_write_all(
        stream, buffer, EDF_LOCAL_RECORDING_SZ, &sz, NULL, error
    );

    return sz;
}

static gsize
read_date (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char temp[256];
    gsize nread = 0;
    int y, m, d;

    // read start date and time
    if (
        g_input_stream_read_all(
           stream, temp, EDF_START_DATE_SZ, &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_START_DATE_SZ] = '\0';
    if (
        g_ascii_isdigit(temp[0]) && g_ascii_isdigit(temp[1])  &&
        temp[2] == '.'                                             &&
        g_ascii_isdigit(temp[3]) && g_ascii_isdigit(temp[4])  &&
        temp[5] == '.'                                             &&
        g_ascii_isdigit(temp[6]) && g_ascii_isdigit(temp[7])
    ) {
        ; // everything is fine
    }
    else
        g_set_error(error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
                    "The date '%s' is in an invalid format",
                    temp
        );
    y = g_ascii_strtoll(&temp[0], NULL, 10);
    m = g_ascii_strtoll(&temp[3], NULL, 10);
    d = g_ascii_strtoll(&temp[6], NULL, 10);

    set_date_ymd(hdr, y, m, d);
    return nread;
}

static gsize
write_date(EdfHeader* hdr, GOutputStream* stream, GError **error) {
    gsize sz;
    char temp[EDF_START_DATE_SZ + 1];
    char buffer[EDF_START_DATE_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    g_snprintf(temp, sizeof(temp), "%02d.%02d.%02d",
                    g_date_time_get_day_of_month(priv->date_and_time),
                    g_date_time_get_month(priv->date_and_time),
                    g_date_time_get_year(priv->date_and_time) % 100
    );
    memset(buffer, ' ', EDF_START_DATE_SZ);
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_START_DATE_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_START_DATE_SZ, &sz, NULL, error
    );

    return sz;
}

static gsize
read_time (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char temp[256];
    gsize nread = 0;
    int h, m, s;

    // read start date and time
    if (
        g_input_stream_read_all(
                stream, temp,
                EDF_START_TIME_SZ,
                &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_START_TIME_SZ] = '\0';
    if (
        g_ascii_isdigit(temp[0]) && g_ascii_isdigit(temp[1])  &&
        temp[2] == '.'                                             &&
        g_ascii_isdigit(temp[3]) && g_ascii_isdigit(temp[4])  &&
        temp[5] == '.'                                             &&
        g_ascii_isdigit(temp[6]) && g_ascii_isdigit(temp[7])
        ) {
        ; // everything is fine
    }
    else
        g_set_error(error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
                    "The date '%s' is in an invalid format",
                    temp
        );
    h = g_ascii_strtoll(&temp[0], NULL, 10);
    m = g_ascii_strtoll(&temp[3], NULL, 10);
    s = g_ascii_strtoll(&temp[6], NULL, 10);

    set_time_hms(hdr, h, m, s);
    return nread;
}

static gsize
write_time(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    gsize sz;
    char temp[EDF_START_TIME_SZ + 1];
    char buffer[EDF_START_TIME_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    memset(buffer, ' ', EDF_START_TIME_SZ);
    g_snprintf(temp, sizeof(temp), "%02d.%02d.%02d",
                    g_date_time_get_hour(priv->date_and_time),
                    g_date_time_get_minute(priv->date_and_time),
                    g_date_time_get_second(priv->date_and_time)
    );
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_START_TIME_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_START_TIME_SZ, &sz, NULL, error
    );

    return sz;
}

static gsize
read_num_bytes(EdfHeader* hdr, GInputStream *stream, GError **error)
{
    (void) hdr; // This function doesn't access hrd, but keep function signature
                // similar to other read functions.
    char temp[256];
    gsize nread = 0;
    gint num_bytes;

    // read start date and time
    if (
        g_input_stream_read_all(
            stream, temp,
            EDF_NUM_BYTES_IN_HEADER_SZ,
            &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_NUM_BYTES_IN_HEADER_SZ] = '\0';
    num_bytes = g_ascii_strtoll(temp, NULL, 10);
    if (num_bytes % 256 != 0) {
        g_set_error(
            error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
            "the num bytes of the header is not a multiple of 256 %s", temp
            );
        return num_bytes;
    }
    return nread;
}

static gsize
write_num_bytes(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    gsize sz;
    char buffer[EDF_NUM_BYTES_IN_HEADER_SZ];
    char temp[EDF_NUM_BYTES_IN_HEADER_SZ + 1];
    // Write header size
    memset (buffer, ' ', EDF_NUM_BYTES_IN_HEADER_SZ);
    g_snprintf(temp, sizeof(temp), "%d", edf_header_get_num_bytes(hdr));
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_NUM_BYTES_IN_HEADER_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_NUM_BYTES_IN_HEADER_SZ, &sz, NULL, error
    );
    return sz;
}

static gsize
read_reserved(EdfHeader* hdr, GInputStream *stream, GError **error)
{
    char temp[256];
    gsize nread = 0;
    // read reserved
    if (
        g_input_stream_read_all (
                stream, temp,
                EDF_RESERVED_SZ,
                &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_RESERVED_SZ] = '\0';
    if (!edf_header_set_reserved(hdr, temp)) {
        g_set_error (error, edf_header_error_quark(), EDF_HEADER_ERROR_PARSE,
                     "'%s' is invalid local recording info",
                     temp
        );
    }
    return nread;
}

static gsize
write_reserved(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    gsize sz;
    char buffer[EDF_RESERVED_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    // Write reserved
    memset (buffer, ' ', EDF_RESERVED_SZ);
    memcpy(buffer,
           priv->reserved->str,
           MIN(priv->reserved->len, EDF_RESERVED_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_RESERVED_SZ, &sz, NULL, error
    );

    return sz;
}

static gsize
read_num_records(EdfHeader* hdr, GInputStream *stream, GError **error)
{
    char temp[256];
    gsize nread = 0;
    gint num_recs;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    // read start date and time
    if (
        g_input_stream_read_all(
            stream, temp,
            EDF_NUM_DATA_REC_SZ,
            &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_NUM_DATA_REC_SZ] = '\0';

    num_recs = g_ascii_strtoll(temp, NULL, 10);
    priv->num_records = num_recs;
    return nread;
}

static gsize
write_num_records(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    gsize sz;
    char temp[EDF_NUM_BYTES_IN_HEADER_SZ + 1];
    char buffer[EDF_NUM_BYTES_IN_HEADER_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    memset (buffer, ' ', EDF_NUM_BYTES_IN_HEADER_SZ);
    g_snprintf(temp, sizeof(temp), "%d", priv->num_records);
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_NUM_DATA_REC_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_NUM_DATA_REC_SZ, &sz, NULL, error
    );

    return sz;
}

static gsize
read_dur_records(EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char  temp[256];
    gsize nread = 0;
    gdouble duration;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    // read start date and time
    if (
        g_input_stream_read_all (
            stream, temp,
            EDF_DURATION_OF_DATA_RECORD_SZ,
            &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_DURATION_OF_DATA_RECORD_SZ] = '\0';

    duration = g_ascii_strtod(temp, NULL);
    priv->duration_of_record = duration;
    return nread;
}

static gsize
write_dur_records(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    gsize sz;
    char temp[EDF_DURATION_OF_DATA_RECORD_SZ + 1];
    char buffer[EDF_DURATION_OF_DATA_RECORD_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    memset (buffer, ' ', EDF_DURATION_OF_DATA_RECORD_SZ);
    g_snprintf(temp, sizeof(temp), "%lf", priv->duration_of_record);
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_NUM_DATA_REC_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_NUM_DATA_REC_SZ, &sz, NULL, error
    );
    return sz;
}

static gsize
read_num_signals (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    char  temp[256];
    gsize nread = 0;
    gint  num_signals;

    if (
        g_input_stream_read_all (
            stream, temp,
            EDF_NUM_SIGNALS_SZ,
            &nread, NULL, error
        ) != TRUE
    ) {
        return nread;
    }
    temp[EDF_NUM_SIGNALS_SZ] = '\0';
    num_signals = g_ascii_strtoll(temp, NULL, 10);
    set_num_signals(hdr, num_signals);
    return nread;
}

static gsize
write_num_signals (EdfHeader* hdr, GOutputStream* stream, GError **error)
{
    gsize sz;
    char temp[EDF_NUM_SIGNALS_SZ + 1];
    char buffer[EDF_NUM_SIGNALS_SZ];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);

    memset (buffer, ' ', EDF_NUM_SIGNALS_SZ);
    g_snprintf(temp, sizeof(temp), "%d", priv->signals->len);
    memcpy(buffer,
           temp,
           MIN(strlen(temp), EDF_NUM_SIGNALS_SZ)
    );
    g_output_stream_write_all(
        stream, buffer, EDF_NUM_SIGNALS_SZ, &sz, NULL, error
    );
    return sz;
}

static gsize
read_signals (EdfHeader* hdr, GInputStream* stream, GError** error)
{
    int nread = 0;
    EdfHeaderClass *klass = EDF_HEADER_GET_CLASS(hdr);

    nread += klass->read_label(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_transducer(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_phys_dim(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_phys_min(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_phys_max(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_dig_min(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_dig_max(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_prefiltering(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_num_samples_per_rec(hdr, stream, error);
    if (*error)
        return nread;
    nread += klass->read_sig_reserved(hdr, stream, error);
    if (*error)
        return nread;

    return nread;
}

static gsize
write_signals (EdfHeader* hdr, GOutputStream* stream, GError** error)
{
    gsize nwritten = 0;
    EdfHeaderClass *klass = EDF_HEADER_GET_CLASS(hdr);

    nwritten += klass->write_label(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_transducer(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_phys_dim(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_phys_min(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_phys_max(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_dig_min(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_dig_max(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_prefiltering(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_num_samples_per_rec(hdr, stream, error);
    if (*error)
        return nwritten;
    nwritten += klass->write_sig_reserved(hdr, stream, error);
    if (*error)
        return nwritten;

    return nwritten;
}

static gsize
read_label (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_LABEL_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_LABEL_SZ] = '\0';
        g_strstrip(temp);
        g_object_set(priv->signals->pdata[i], "label", temp, NULL);
    }
    return nread_tot;
}

static gsize
write_label(EdfHeader *hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(hdr);
    char buffer[EDF_LABEL_SZ];
    gsize sz, written_size = 0;
    for (gsize i = 0; i < priv->signals->len; i++) {
        const char* var;
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);

        memset(buffer, ' ', EDF_LABEL_SZ);
        var = edf_signal_get_label(signal);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_LABEL_SZ)
        );

        g_output_stream_write_all(
            stream, buffer, EDF_LABEL_SZ, &sz, NULL, error
        );
        written_size += sz;
        if (*error)
            break;
    }
    return written_size;
}

static gsize
read_transducer (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_TRANDUCER_TYPE_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_TRANDUCER_TYPE_SZ] = '\0';
        g_strstrip(temp);
        g_object_set(priv->signals->pdata[i], "transducer", temp, NULL);
    }
    return nread_tot;
}

static gsize
write_transducer (EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char buffer[EDF_TRANDUCER_TYPE_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char*var = edf_signal_get_transducer(signal);

        memset(buffer, ' ', EDF_TRANDUCER_TYPE_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_TRANDUCER_TYPE_SZ)
        );

        g_output_stream_write_all(
            stream, buffer, EDF_TRANDUCER_TYPE_SZ, &sz, NULL, error
        );
        written_size += sz;
        if (*error)
            break;
    }

    return written_size;
}

static gsize
read_phys_dim (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_PHYSICAL_DIMENSION_SZ,
                &nread, NULL, error
            ) != TRUE
            ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_PHYSICAL_DIMENSION_SZ] = '\0';
        g_strstrip(temp);
        g_object_set(priv->signals->pdata[i], "physical-dimension", temp, NULL);
    }
    return nread_tot;
}

static gsize
write_phys_dim (EdfHeader* hdr, GOutputStream* stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char buffer[EDF_PHYSICAL_DIMENSION_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char* var = edf_signal_get_physical_dimension(signal);

        memset(buffer, ' ', EDF_PHYSICAL_DIMENSION_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_PHYSICAL_DIMENSION_SZ)
        );

        g_output_stream_write_all(
            stream, buffer, EDF_PHYSICAL_DIMENSION_SZ, &sz, NULL, error
        );
        written_size += sz;
        if (*error)
            break;
    }

    return written_size;
}

static gsize
read_phys_min (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    gdouble val;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_PHYSICAL_MINIMUM_SZ,
                &nread, NULL, error
            ) != TRUE
            ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_PHYSICAL_MINIMUM_SZ] = '\0';
        val = g_ascii_strtod(temp, NULL);
        g_object_set(priv->signals->pdata[i], "physical-min", val, NULL);
    }
    return nread_tot;
}

static gsize
write_phys_min (EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char temp[EDF_PHYSICAL_MINIMUM_SZ + 1];
    char buffer[EDF_PHYSICAL_MINIMUM_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gdouble doublevar = edf_signal_get_physical_min(signal);

        memset(buffer, ' ', EDF_PHYSICAL_MINIMUM_SZ);
        g_snprintf(temp, sizeof(temp), "%lf", doublevar);
        memcpy(buffer,
               temp,
               MIN(strlen(temp), EDF_PHYSICAL_MINIMUM_SZ)
        );
        g_output_stream_write_all(
            stream, buffer, EDF_PHYSICAL_MINIMUM_SZ, &sz, NULL, error
        );
        written_size += sz;
        if (*error)
            break;
    }
    return written_size;
}

static gsize
read_phys_max (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    gdouble val;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_PHYSICAL_MAXIMUM_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_PHYSICAL_MAXIMUM_SZ] = '\0';
        val = g_ascii_strtod(temp, NULL);
        g_object_set(priv->signals->pdata[i], "physical-max", val, NULL);
    }
    return nread_tot;
}

static gsize
write_phys_max (EdfHeader* hdr, GOutputStream *stream, GError **error) {
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char temp[EDF_PHYSICAL_MAXIMUM_SZ + 1];
    char buffer[EDF_PHYSICAL_MAXIMUM_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gdouble doublevar = edf_signal_get_physical_max(signal);

        memset(buffer, ' ', EDF_PHYSICAL_MAXIMUM_SZ);
        g_snprintf(temp, sizeof(temp), "%lf", doublevar);
        memcpy(buffer,
               temp,
               MIN(strlen(temp), EDF_PHYSICAL_MAXIMUM_SZ)
        );
        g_output_stream_write_all(
            stream, buffer, EDF_PHYSICAL_MAXIMUM_SZ, &sz, NULL, error
        );
        written_size += sz;

        if (*error)
            break;
    }
    return written_size;
}

static gsize
read_dig_min (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    gint  val;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_DIGITAL_MINIMUM_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_DIGITAL_MINIMUM_SZ] = '\0';
        val = g_ascii_strtoll(temp, NULL, 10);
        g_object_set(priv->signals->pdata[i], "digital-min", val, NULL);
    }
    return nread_tot;
}

static gsize
write_dig_min(EdfHeader* hdr, GOutputStream *stream, GError **error) {
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char temp[EDF_DIGITAL_MINIMUM_SZ + 1];
    char buffer[EDF_DIGITAL_MINIMUM_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gint intvar = edf_signal_get_digital_min(signal);

        memset(buffer, ' ', EDF_DIGITAL_MINIMUM_SZ);
        g_snprintf(temp, sizeof(temp), "%d", intvar);
        memcpy(buffer,
               temp,
               MIN(strlen(temp), EDF_DIGITAL_MINIMUM_SZ)
        );
        g_output_stream_write_all(
            stream, buffer, EDF_DIGITAL_MINIMUM_SZ, &sz, NULL, error
        );

        written_size += sz;

        if (*error)
            break;
    }

    return written_size;
}

static gsize
read_dig_max (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    gint  val;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_DIGITAL_MAXIMUM_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_DIGITAL_MAXIMUM_SZ] = '\0';
        val = g_ascii_strtoll(temp, NULL, 10);
        g_object_set(priv->signals->pdata[i], "digital-max", val, NULL);
    }
    return nread_tot;
}

static gsize
write_dig_max(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char temp[EDF_DIGITAL_MAXIMUM_SZ + 1];
    char buffer[EDF_DIGITAL_MAXIMUM_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gint intvar = edf_signal_get_digital_max(signal);

        memset(buffer, ' ', EDF_DIGITAL_MAXIMUM_SZ);
        g_snprintf(temp, sizeof(temp), "%d", intvar);
        memcpy(buffer,
               temp,
               MIN(strlen(temp), EDF_DIGITAL_MAXIMUM_SZ)
        );
        g_output_stream_write_all(
            stream, buffer, EDF_DIGITAL_MAXIMUM_SZ, &sz, NULL, error
        );
        written_size += sz;

        if (*error)
            break;
    }

    return written_size;
}

static gsize
read_prefiltering (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_PREFILTERING_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_PREFILTERING_SZ] = '\0';
        g_strstrip(temp);
        g_object_set(priv->signals->pdata[i], "prefilter", temp, NULL);
    }
    return nread_tot;
}

static gsize
write_prefiltering(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char buffer[EDF_PREFILTERING_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char* var = edf_signal_get_prefiltering(signal);

        memset(buffer, ' ', EDF_PREFILTERING_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_PREFILTERING_SZ)
        );

        g_output_stream_write_all(
            stream, buffer, EDF_PREFILTERING_SZ, &sz, NULL, error
        );
        written_size += sz;

        if (*error)
            break;
    }

    return written_size;
}

static gsize
read_num_samples_per_rec (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    guint val;
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_NUM_SAMPLES_PER_RECORD_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_NUM_SAMPLES_PER_RECORD_SZ] = '\0';
        val = g_ascii_strtoull(temp, NULL, 10);
        g_object_set(priv->signals->pdata[i], "ns", val, NULL);
    }
    return nread_tot;
}

static gsize
write_num_samples_per_rec(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char temp[EDF_NUM_SAMPLES_PER_RECORD_SZ + 1];
    char buffer[EDF_NUM_SAMPLES_PER_RECORD_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        guint32 guintvar = edf_signal_get_num_samples_per_record(signal);

        memset(buffer, ' ', EDF_NUM_SAMPLES_PER_RECORD_SZ);
        g_snprintf(temp, sizeof(temp), "%u", guintvar);
        memcpy(buffer,
               temp,
               MIN(strlen(temp), EDF_NUM_SAMPLES_PER_RECORD_SZ)
        );

        g_output_stream_write_all(
            stream, buffer, EDF_NUM_SAMPLES_PER_RECORD_SZ, &sz, NULL, error
        );
        written_size += sz;
        if (*error)
            break;
    }

    return written_size;
}

static gsize
read_sig_reserved (EdfHeader* hdr, GInputStream *stream, GError **error)
{
    gsize nread = 0, nread_tot = 0;
    char  temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    for (gsize i = 0; i < priv->signals->len; i++) {
        if (
            g_input_stream_read_all (
                stream, temp,
                EDF_NS_RESERVED_SZ,
                &nread, NULL, error
            ) != TRUE
        ) {
            return nread_tot;
        }
        nread_tot += nread;
        temp[EDF_NS_RESERVED_SZ] = '\0';
        g_strstrip(temp);
        g_object_set(priv->signals->pdata[i], "reserved", temp, NULL);
    }
    return nread_tot;
}

static gsize
write_sig_reserved(EdfHeader* hdr, GOutputStream *stream, GError **error)
{
    EdfHeaderPrivate *priv = edf_header_get_instance_private(hdr);
    char buffer[EDF_NS_RESERVED_SZ];
    gsize sz, written_size = 0;

    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char*var = edf_signal_get_reserved(signal);

        memset(buffer, ' ', EDF_NS_RESERVED_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_NS_RESERVED_SZ)
        );

        g_output_stream_write_all(
            stream, buffer, EDF_NS_RESERVED_SZ, &sz, NULL, error
        );
        written_size += sz;
        if (*error)
            break;
    }

    return written_size;
}

static void
header_set_signals(EdfHeader* self, GPtrArray* signals)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(self);
    if (priv->signals)
        g_ptr_array_unref(priv->signals);

    priv->signals = g_ptr_array_ref(signals);
}

static void
header_update(EdfHeader* self)
{
    g_assert(EDF_IS_HEADER (self));
    EdfHeaderPrivate* priv = edf_header_get_instance_private(self);

    if (priv->signals->len > 0) {
        priv->num_records = edf_signal_get_num_records (
            g_ptr_array_index(priv->signals, 0)
        );
    }
    else {
        priv->num_records = -1;
    }
}

static void
edf_header_init(EdfHeader* self)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(self);

    priv->version = 0;
    priv->local_patient_identification = g_string_new("");
    priv->local_recording_identification = g_string_new("");
    priv->date_and_time = g_date_time_new_now_local();
    priv->num_records = -1; // only valid when recording...

    priv->reserved = g_string_new("");

    priv->signals = g_ptr_array_new();
}

static void
edf_header_dispose(GObject* gobject)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(EDF_HEADER(gobject));

    // Drop references and make sure they point to NULL.
    g_date_time_unref(priv->date_and_time);
    priv->date_and_time = NULL;

    g_ptr_array_unref(priv->signals);
    priv->signals = NULL;

    // Chain up to parent
    G_OBJECT_CLASS(edf_header_parent_class)->dispose(gobject);
}

static void
edf_header_finalize(GObject* gobject)
{
    EdfHeaderPrivate* priv = edf_header_get_instance_private(EDF_HEADER(gobject));

    // free remaining resources.
    g_string_free(priv->local_patient_identification, TRUE);
    g_string_free(priv->local_recording_identification, TRUE);
    g_string_free(priv->reserved, TRUE);
    
    // Chain up to parent
    G_OBJECT_CLASS(edf_header_parent_class)->dispose(gobject);
}

typedef enum {
    PROP_VERSION = 1,
    PROP_PATIENT_INFO,
    PROP_REC_INFO,
    PROP_DATE_TIME,
    PROP_NUM_BYTES_HEADER,
    PROP_RESERVED,
    PROP_NUM_DATA_RECORDS,
    PROP_DURATION_RECORD,
    PROP_NUM_SIGNALS,
    N_PROPERTIES
} EdfHeaderProperty;

static void
edf_header_set_property (
        GObject        *object,
        guint32         propid,
        const GValue   *value,
        GParamSpec     *spec
    )
{
    EdfHeader* self = EDF_HEADER(object);
    EdfHeaderPrivate* priv = edf_header_get_instance_private(self);

    switch ((EdfHeaderProperty) propid) {
        case PROP_PATIENT_INFO:
            set_patient_identification(self, g_value_get_string(value));
            break;
        case PROP_REC_INFO:
            set_recording_identification(self, g_value_get_string(value));
            break;
        case PROP_DATE_TIME:
            edf_header_set_time(self, g_value_get_boxed(value));
            break;
        case PROP_DURATION_RECORD:
            priv->duration_of_record = g_value_get_double(value);
            break;
        case PROP_RESERVED:
            set_reserved(self, g_value_get_string(value));
            break;
        case PROP_NUM_DATA_RECORDS:
            priv->num_records = g_value_get_int(value);
            break;
        case PROP_VERSION:          // read only
        case PROP_NUM_BYTES_HEADER: // read only
        case PROP_NUM_SIGNALS:      // read only
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, spec);
    }
}

static void
edf_header_get_property (
        GObject    *object,
        guint32     propid,
        GValue     *value,
        GParamSpec *spec
    )
{
    EdfHeader* self = EDF_HEADER(object);
    EdfHeaderPrivate* priv = edf_header_get_instance_private(self);

    switch ((EdfHeaderProperty) propid) {
        case PROP_VERSION:
            g_value_set_int(value, priv->version);
            break;
        case PROP_PATIENT_INFO:
            g_value_set_string(value, priv->local_patient_identification->str);
            break;
        case PROP_REC_INFO:
            g_value_set_string(value, priv->local_recording_identification->str);
            break;
        case PROP_DATE_TIME:
            g_value_set_boxed(value, priv->date_and_time);
            break;
        case PROP_NUM_BYTES_HEADER:
            g_value_set_uint(value, edf_header_get_num_bytes(self));
            break;
        case PROP_DURATION_RECORD:
            g_value_set_double(value, priv->duration_of_record);
            break;
        case PROP_RESERVED:
            g_value_set_string(value, priv->reserved->str);
            break;
        case PROP_NUM_DATA_RECORDS:
            g_value_set_int(value, priv->num_records);
            break;
        case PROP_NUM_SIGNALS:
            g_value_set_uint(value, priv->signals->len);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, spec);
    }
}

static GParamSpec *edf_header_properties[N_PROPERTIES] = {NULL,};

static void
edf_header_class_init(EdfHeaderClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = edf_header_set_property;
    object_class->get_property = edf_header_get_property;
    object_class->dispose = edf_header_dispose;
    object_class->finalize = edf_header_finalize;

    // Reading the fixed header from file
    klass->read_version = read_version;
    klass->read_patient = read_patient;
    klass->read_recording = read_recording;
    klass->read_date = read_date;
    klass->read_time = read_time;
    klass->read_num_bytes = read_num_bytes;
    klass->read_reserved = read_reserved;
    klass->read_num_records = read_num_records;
    klass->read_dur_records = read_dur_records;
    klass->read_num_signals = read_num_signals;
    klass->read_signals = read_signals;

    // Reading the signal related information from the header
    klass->read_label = read_label;
    klass->read_transducer = read_transducer;
    klass->read_phys_dim = read_phys_dim;
    klass->read_phys_min = read_phys_min;
    klass->read_phys_max = read_phys_max;
    klass->read_dig_min = read_dig_min;
    klass->read_dig_max = read_dig_max;
    klass->read_prefiltering = read_prefiltering;
    klass->read_num_samples_per_rec = read_num_samples_per_rec;
    klass->read_sig_reserved = read_sig_reserved;

    // Writing the fixed header from file
    klass->write_version = write_version;
    klass->write_patient = write_patient;
    klass->write_recording = write_recording;
    klass->write_date = write_date;
    klass->write_time = write_time;
    klass->write_num_bytes = write_num_bytes;
    klass->write_reserved = write_reserved;
    klass->write_num_records = write_num_records;
    klass->write_dur_records = write_dur_records;
    klass->write_num_signals = write_num_signals;
    klass->write_signals = write_signals;

    // Writing the signal related information from the header
    klass->write_label = write_label;
    klass->write_transducer = write_transducer;
    klass->write_phys_dim = write_phys_dim;
    klass->write_phys_min = write_phys_min;
    klass->write_phys_max = write_phys_max;
    klass->write_dig_min = write_dig_min;
    klass->write_dig_max = write_dig_max;
    klass->write_prefiltering = write_prefiltering;
    klass->write_num_samples_per_rec = write_num_samples_per_rec;
    klass->write_sig_reserved = write_sig_reserved;

    /**
     * EdfHeader:version:
     *
     * The header specifies a version (0 for EDF and EDF+) this
     * property can obtain that version. It is stored as string in
     * the header, but turned into an integer for programming.
     */
    edf_header_properties[PROP_VERSION] = g_param_spec_int(
            "version",
            "Version",
            "Version of the edf file",
            0,
            99999999,
            0,
            G_PARAM_READABLE
            );

    /**
     * EdfHeader:patient-identification:
     *
     * This holds some general information about the patient. Edf+ is more strict
     * on this than EDF is.
     */
    edf_header_properties[PROP_PATIENT_INFO] = g_param_spec_string(
            "patient-identification",
            "local-patient-identification",
            "Identifies the patient",
            "",
            G_PARAM_READWRITE
            );
    
    /**
     * EdfHeader:recording-identification:
     *
     * This holds some general information about the recording. Edf+ is more strict
     * on this than EDF is.
     */
    edf_header_properties[PROP_REC_INFO] = g_param_spec_string(
            "recording-identification",
            "local-recording-identification",
            "Identifies the recording",
            "",
            G_PARAM_READWRITE
            );

    /**
     * EdfHeader:date-time:
     *
     * An edf header stores the date and time separately in the header, this
     * member holds both together.
     */
    edf_header_properties[PROP_DATE_TIME] = g_param_spec_boxed(
            "date-time",
            "Date-Time",
            "The combination of date and time of the record",
            G_TYPE_DATE_TIME,
            G_PARAM_READWRITE
            );

    /**
     * EdfHeader:num-bytes-header:
     *
     * This holds the number of bytes that the header will occupy.
     * Which is 256 + (number of signals * 256).
     */
    edf_header_properties[PROP_NUM_BYTES_HEADER] = g_param_spec_uint(
            "num-bytes-header",
            "num-bytes-header",
            "The number of bytes the header occupies in the file",
            edf_compute_header_size(0),
            edf_compute_header_size(9999),
            edf_compute_header_size(0),
            G_PARAM_READABLE
            );

    /**
     * EdfHeader:reserved
     *
     * Reserved information about this header.
     */
    edf_header_properties[PROP_RESERVED] = g_param_spec_string(
            "reserved",
            "Reserved",
            "A reserved portion of the header identifies specific info.",
            "",
            G_PARAM_READWRITE
            );

    /**
     * EdfHeader:num-data_records
     *
     * The number of data records per signal. This can be -1 when recording
     * otherwise it should be >= 0.
     */
    edf_header_properties[PROP_NUM_DATA_RECORDS] = g_param_spec_int(
            "num-data-records",
            "number-of-data-records",
            "The number of data records stored in this file per signal.",
            -1,
            G_MAXINT,
            -1,
            G_PARAM_READABLE
            );

    /**
     * EdfHeader:duration-of-record:
     *
     * The duration of one record. This is typically 1.0 or smaller then 1.0 when
     * the number of bytes needed to record one record is larger than 61440.
     * gedflib allows you to pick your own records size.
     */
    edf_header_properties[PROP_DURATION_RECORD] = g_param_spec_double(
            "duration-of-record",
            "duration-of-record",
            "The duration of a record in seconds.",
            0.0,
            G_MAXDOUBLE,
            1.0,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    /**
     * EdfHeader:num_signals:
     *
     * The number of signals helt in this file.
     * this can be change via adding/removing signals.
     */
    edf_header_properties[PROP_NUM_SIGNALS] = g_param_spec_uint(
            "num-signals",
            "ns",
            "The number of signals/channels in this file",
            0,
            9999,
            0,
            G_PARAM_READABLE
            );

    g_object_class_install_properties(
            object_class, N_PROPERTIES, edf_header_properties
            );
}

/* ************* public functions ***************** */

/**
 * edf_header_new:(constructor)
 *
 * Create a default new Edf header.
 */
EdfHeader*
edf_header_new() {
    EdfHeader* header;
    header = g_object_new(EDF_TYPE_HEADER, NULL);
    return header;
}

/**
 * edf_header_destroy:(skip)
 * @header :: The header to destroy
 *
 * Decrements the reference of header and will
 * free the associated resources when the refcnt
 * drops to zero.
 */
void
edf_header_destroy(EdfHeader* header) {
    g_object_unref(G_OBJECT(header));
}

/**
 * edf_header_set_signals :
 * @hdr :(inout): the header object
 * @signals:(in)(transfer full)(element-type EdfSignal): the number of signals 0 < ns < 9999
 *
 * Set the signals that belong to this header.
 *
 * Returns :: TRUE when the operation was successful
 */
gboolean
edf_header_set_signals(EdfHeader* header, GPtrArray* signals)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);
    header_set_signals(header, signals);
    return TRUE;
}

/**
 * edf_header_write_to_ostream:
 * @hdr:(in):The header to write to the output stream
 * @ostream:(inout):The output stream to which the header should be written.
 * @error:(out):If an error occurs it will be returned here.
 *
 * Writes an header to an output stream.
 *
 * private: for internal use only
 * Returns::the number of written bytes.
 */
gsize
edf_header_write_to_ostream(
        EdfHeader      *header,
        GOutputStream  *ostream,
        GError        **error
        )
{
    gsize sz;

    g_return_val_if_fail(EDF_IS_HEADER(header) && G_IS_OUTPUT_STREAM(ostream), 0);
    g_return_val_if_fail(error == NULL || error != NULL, 0);

    EdfHeaderClass* klass = EDF_HEADER_GET_CLASS(header);
    sz = 0;

    header_update(header); // Make sure nrecords is up to date.

    sz += klass->write_version(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_patient(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_recording(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_date(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_time(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_num_bytes(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_reserved(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_num_records(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_dur_records(header, ostream, error);
    if (*error)
        return sz;

    sz += klass->write_num_signals(header, ostream, error);
    if (*error)
        return sz;

    // write the signal related info to the header.
    sz += klass->write_signals(header, ostream, error);
    if (*error)
        return sz;

    g_assert(sz == (gsize)edf_header_get_num_bytes(header));

    return sz;
}

/**
 * edf_header_read_from_input_stream:
 * @hdr: An #EdfHeader instance
 * @inputStream :(in)(out): A GInputStream to read
 *                          the header from
 * @error: (out): An error might be returned here
 *
 * Read an header from a input stream.
 *
 * Returns: the number of bytes read.
 */
gsize
edf_header_read_from_input_stream(
    EdfHeader      *header,
    GInputStream   *istream,
    GError        **error
)
{
    gsize nread = 0;
    EdfHeaderClass* klass;

    g_return_val_if_fail(EDF_IS_HEADER(header), 0);
    g_return_val_if_fail(G_IS_INPUT_STREAM(istream), 0);
    g_return_val_if_fail(error != NULL && *error == NULL, 0);

    klass = EDF_HEADER_GET_CLASS(header);

    nread += klass->read_version(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_patient(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_recording(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_date(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_time(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_num_bytes(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_reserved(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_num_records(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_dur_records(header, istream, error);
    if (*error)
        return nread;

    nread += klass->read_num_signals(header, istream, error);
    if (*error)
        return nread;

    g_assert(nread == EDF_BASE_HEADER_SIZE);

    nread += klass->read_signals(header, istream, error);
    if (*error)
        return nread;

    g_assert(nread == (gsize)edf_header_get_num_bytes(header));

    return nread;
}

/**
 * edf_header_get_version:
 * @header: the #EdfHeader instance whose version you would like to obtain.
 *
 * obtains the version of the header.
 */
gint
edf_header_get_version(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), -1);

    priv = edf_header_get_instance_private (header);
    return priv->version;
}

/**
 * edf_header_get_patient:
 * @header: the #EdfHeader instance whose local patient info you
 *          would like to obtain.
 *
 * obtain the local patient info.
 */
const gchar*
edf_header_get_patient(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), NULL);

    priv = edf_header_get_instance_private (header);
    return priv->local_patient_identification->str;
}

/**
 * edf_header_set_patient:
 * @header: the header whose patient info to update
 * @patient: the new patient info
 *
 * The length of the patient info is stored as in the file
 * This makes sure that the string is terminated in such a way
 * that it will fit the inside the header.
 */
gboolean
edf_header_set_patient(EdfHeader* header, const gchar* patient)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);
    g_return_val_if_fail(patient != NULL && g_str_is_ascii(patient), FALSE);

    char temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(header);
    g_snprintf(temp, sizeof(temp), "%s", patient);
    g_strstrip(temp);
    
    g_string_assign(priv->local_patient_identification, temp);
    return TRUE;
}

/**
 * edf_header_get_recording:
 * @header the input header
 *
 * Obtain the recording information as stored in the file
 * The length of the recording info is stored as in the file
 * This makes sure that the string is terminated in such a way
 * that it will fit the inside the header.
 */
const gchar*
edf_header_get_recording(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), NULL);

    priv = edf_header_get_instance_private (header);
    return priv->local_recording_identification->str;
}

/**
 * edf_header_set_recording:
 * @header: the header to set the recording information
 * @recording: the new recording information, must be ascii
 *
 * Sets the new recording information
 */
gboolean
edf_header_set_recording(EdfHeader* header, const gchar* recording)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);
    g_return_val_if_fail(recording != NULL && g_str_is_ascii(recording), FALSE);

    char temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(header);
    g_snprintf(temp, sizeof(temp), "%s", recording);
    g_strstrip(temp);
    
    g_string_assign(priv->local_recording_identification, temp);
    return TRUE;
}

/**
 * edf_header_get_time:
 * @header an #EdfHeader
 *
 * Returns the date and time information of the header The returned time is
 * valid in the local time zone.
 *
 * Returns : (transfer full) : the date and time of the header
 */
GDateTime*
edf_header_get_time(EdfHeader* header)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), NULL);
    EdfHeaderPrivate* priv = edf_header_get_instance_private(header);

    return g_date_time_ref(priv->date_and_time);
}

/**
 * edf_header_set_time:
 * @header an #EdfHeader
 * @time an #GDateTime which contains the time of the recording
 *
 * Sets the date and time information of the header.
 *
 * Returns : TRUE when the time was successfully updated. FALSE otherwise
 */
gboolean
edf_header_set_time(EdfHeader* header, GDateTime* time)
{
    g_return_val_if_fail (EDF_IS_HEADER (header), FALSE);
    g_return_val_if_fail (time != NULL, FALSE);

    EdfHeaderPrivate *priv = edf_header_get_instance_private (header);
    if (priv->date_and_time)
        g_date_time_unref (priv->date_and_time);

    priv->date_and_time = g_date_time_ref (time);
    return TRUE;
}

/**
 * edf_header_get_num_bytes:
 * @param header :(in)
 *
 * Returns: The number of bytes in the header should be
 *          256 + 256 * number of signals
 */
gint
edf_header_get_num_bytes(EdfHeader* hdr)
{
    g_return_val_if_fail(EDF_IS_HEADER(hdr), -1);
    unsigned num_signals = edf_header_get_num_signals(hdr);
    return num_signals * EDF_SIGNAL_HEADER_SIZE + EDF_BASE_HEADER_SIZE;
}

/**
 * edf_header_get_reserved:
 * @header: The #EdfHeader whose reserved info you would like to obtain.
 *
 * Obtain the reserved field of an EdfHeader.
 *
 * returns: an string in ascii format
 */
const gchar*
edf_header_get_reserved(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), NULL);

    priv = edf_header_get_instance_private (header);
    return priv->reserved->str;
}

/**
 * edf_header_set_reserved:
 * @header: The #EdfHeader whose reserved info you would like to set.
 * @reserved: the new ascii string for the reserved header field
 *
 * set the reserved field of an EdfHeader.
 *
 * returns: TRUE if the reserved field was successfully updated FALSE otherwise.
 */
gboolean
edf_header_set_reserved(EdfHeader* header, const gchar* reserved)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);
    g_return_val_if_fail(reserved != NULL && g_str_is_ascii(reserved), FALSE);

    char temp[256];
    EdfHeaderPrivate *priv = edf_header_get_instance_private(header);
    g_snprintf(temp, sizeof(temp), "%s", reserved);
    g_strstrip(temp);
    
    g_string_assign(priv->reserved, temp);
    return TRUE;
}

/**
 * edf_header_get_num_records:
 * @header: the #EdfHeader whose number of records you would
 *          like to know.
 *
 * Returns: -1 if no signals are added, otherwise the numbers of records
 *          of the first signal
 */
gint
edf_header_get_num_records(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), G_MININT);
    priv = edf_header_get_instance_private(header);

    header_update(header);

    return priv->num_records;
}

/**
 * edf_header_get_record_duration:
 * @header the #EdfHeader whose record duration you woud like to know.
 *
 * Returns: the duration of one record in the output.
 */
gdouble
edf_header_get_record_duration(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), -0.0);

    priv = edf_header_get_instance_private (header);
    return priv->duration_of_record;
}

/**
 * edf_header_set_record_duration:
 * @header the #EdfHeader whose record duration you woud like to adjust.
 *
 * Sets the duration of one record in the output.
 */
gboolean
edf_header_set_record_duration(EdfHeader* header, gdouble duration)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), FALSE);
    g_return_val_if_fail(duration > 0.0, FALSE);

    priv = edf_header_get_instance_private (header);
    priv->duration_of_record = duration;

    return TRUE;
}

/**
 * edf_header_get_num_signals:
 */
guint
edf_header_get_num_signals(EdfHeader* header)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), -1);
    EdfHeaderPrivate* priv = edf_header_get_instance_private(header);

    return priv->signals->len;
}

/**
 * edf_header_get_signals:
 * @header: the header whose signals to obtain
 *
 * Obtain the signals. You probably want the signals
 * from the file, they are the same, but this
 * function is mainly added for internal testing
 * purposes.
 *
 * Returns: (transfer none)(element-type EdfSignal*):
 * an pointer to the GPtrArray containing the signals.
 */
GPtrArray*
edf_header_get_signals(EdfHeader* header) {
    g_return_val_if_fail(EDF_IS_HEADER(header), NULL);

    EdfHeaderPrivate* priv = edf_header_get_instance_private(header);

    return priv->signals;
}

/**
 * edf_header_is_edf:
 * @header: the EdfHeader to test whether it is EDF
 *
 * The version number should be 0, this discriminates EDF from BDF
 * EDF+ is compatible with EDF, so an EDF+ header will also be considered
 * to be EDF.
 *
 * Returns: TRUE if the header is EDF or EDF+ false otherwise.
 */
gboolean
edf_header_is_edf(EdfHeader* header)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);
    int version = edf_header_get_version(header);

    return version == 0;
}

/**
 * edf_header_is_edfplus:
 * @header: the header to test whether it seems compatible with EDF+
 *
 * An EDF+ header is starts the reserved field with "EDF+C" or
 * "EDF+D", an EDF+ header is a valid EDF header, but a valid EDF file
 * is doesn't have to be a valid EDF+ file.
 *
 * Returns: TRUE if the header is EDF+ compatible
 */
gboolean
edf_header_is_edfplus(EdfHeader* header)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);

    if (edf_header_get_version(header) != 0)
        return FALSE;

    const char edf_continuous[] = "EDF+C";
    const char edf_discontinuous[] = "EDF+D";
    const char* reserved = edf_header_get_reserved(header);

    // TODO make even more strict on other fields.
    if (strncmp(edf_continuous, reserved, sizeof(edf_continuous)) != 0 &&
        strncmp(edf_discontinuous, reserved, sizeof(edf_discontinuous) != 0)
    ) {
        return FALSE;
    }

    return TRUE;
}

/* ************ utility functions ************** */
/* these are not coupled with EdfHeader */

/**
 * edf_compute_header_size
 * @num_signals :: number of signals in the header
 *
 * Computes the header size given the number of signals
 * in the header.
 *
 * The size of the header is 256 bytes for the static part
 * and bytes 256 for each signal.
 *
 * So it should return 256 + 256 * ns;
 *
 * Returns : the number of bytes that the header will take.
 */
gsize
edf_compute_header_size(guint num_signals)
{
    return EDF_BASE_HEADER_SIZE + num_signals * EDF_SIGNAL_HEADER_SIZE;
}

