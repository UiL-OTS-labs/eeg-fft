
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
 * meta data about an (.edf) European Dataformat File.
 *
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

/* ********* functions to read an header ********** */

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
read_num_bytes (EdfHeader* hdr, GInputStream* stream, GError** error)
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
read_reserved (EdfHeader* hdr, GInputStream* stream, GError** error)
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
read_num_records (EdfHeader* hdr, GInputStream* stream, GError** error)
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
read_dur_records (EdfHeader* hdr, GInputStream* stream, GError** error)
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
            g_value_set_uint(
                value,
                EDF_BASE_HEADER_SIZE +
                EDF_SIGNAL_HEADER_SIZE * priv->signals->len
                );
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

    edf_header_properties[PROP_VERSION] = g_param_spec_int(
            "version",
            "Version",
            "Version of the edf file",
            0,
            99999999,
            0,
            G_PARAM_READABLE
            );

    edf_header_properties[PROP_PATIENT_INFO] = g_param_spec_string(
            "patient-identification",
            "local-patient-identification",
            "Identifies the patient",
            "",
            G_PARAM_READWRITE
            );
    
    edf_header_properties[PROP_REC_INFO] = g_param_spec_string(
            "recording-identification",
            "local-recording-identification",
            "Identifies the recording",
            "",
            G_PARAM_READWRITE
            );

    edf_header_properties[PROP_DATE_TIME] = g_param_spec_boxed(
            "date-time",
            "Date-Time",
            "The combination of date and time of the record",
            G_TYPE_DATE_TIME,
            G_PARAM_READWRITE
            );

    edf_header_properties[PROP_NUM_BYTES_HEADER] = g_param_spec_uint(
            "num-bytes-header",
            "num-bytes-header",
            "The number of bytes the header occupies in the file",
            edf_compute_header_size(0),
            edf_compute_header_size(9999),
            edf_compute_header_size(0),
            G_PARAM_READABLE
            );

    edf_header_properties[PROP_RESERVED] = g_param_spec_string(
            "reserved",
            "Reserved",
            "A reserved portion of the header identifies specific info.",
            "",
            G_PARAM_READWRITE
            );

    edf_header_properties[PROP_NUM_DATA_RECORDS] = g_param_spec_int(
            "num-data-records",
            "number-of-data-records",
            "The number of data records stored in this file per signal.",
            -1,
            G_MAXINT,
            -1,
            G_PARAM_READABLE
            );

    edf_header_properties[PROP_DURATION_RECORD] = g_param_spec_double(
            "duration-of-record",
            "duration-of-record",
            "The duration of a record in seconds.",
            0.0,
            G_MAXDOUBLE,
            1.0,
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

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

EdfHeader*
edf_header_new() {
    EdfHeader* header;
    header = g_object_new(EDF_TYPE_HEADER, NULL);
    return header;
}

void
edf_header_destroy(EdfHeader* header) {
    g_object_unref(G_OBJECT(header));
}

gboolean
edf_header_set_signals(EdfHeader* header, GPtrArray* signals)
{
    g_return_val_if_fail(EDF_IS_HEADER(header), FALSE);
    header_set_signals(header, signals);
    return TRUE;
}

gsize 
edf_header_size(EdfHeader* header) {
    g_return_val_if_fail(EDF_IS_HEADER(header), 0);
    EdfHeaderPrivate *priv = edf_header_get_instance_private(header);
    if (priv->signals)
        return EDF_BASE_HEADER_SIZE + EDF_SIGNAL_HEADER_SIZE * priv->signals->len;
    else
        return EDF_BASE_HEADER_SIZE;
}


gsize
edf_header_write_to_ostream(
        EdfHeader      *header,
        GOutputStream  *ostream,
        GError        **error
        )
{
    gchar buffer[256];
    gsize written_size, sz;
    GString* temp;
    gboolean result;

    g_return_val_if_fail(EDF_IS_HEADER(header) && G_IS_OUTPUT_STREAM(ostream), 0);
    g_return_val_if_fail(error == NULL || error != NULL, 0);

    header_update(header);

    temp = g_string_sized_new(256);
    EdfHeaderPrivate* priv = edf_header_get_instance_private(header);
    written_size = 0;

    // Write version
    memset (buffer, ' ', EDF_VERSION_SZ);
    g_string_printf(temp, "%d", priv->version);
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_VERSION_SZ)
          );

    result = g_output_stream_write_all(
            ostream, buffer, EDF_VERSION_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;

    // Write local patient info 
    memset(buffer, ' ', EDF_LOCAL_PATIENT_SZ);
    memcpy(buffer,
            priv->local_patient_identification->str,
            MIN(priv->local_patient_identification->len, EDF_LOCAL_PATIENT_SZ)
          );

    result = g_output_stream_write_all(
            ostream, buffer, EDF_LOCAL_PATIENT_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write local recording info 
    memset(buffer, ' ', EDF_LOCAL_RECORDING_SZ);
    memcpy(buffer,
            priv->local_recording_identification->str,
            MIN(priv->local_recording_identification->len, EDF_LOCAL_RECORDING_SZ)
          );

    result = g_output_stream_write_all(
            ostream, buffer, EDF_LOCAL_RECORDING_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write date
    memset(buffer, ' ', EDF_START_DATE_SZ);
    g_string_printf(temp, "%02d.%02d.%02d",
            g_date_time_get_day_of_month(priv->date_and_time),
            g_date_time_get_month(priv->date_and_time),
            g_date_time_get_year(priv->date_and_time) % 100
            );
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_START_DATE_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_START_DATE_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write time 
    memset(buffer, ' ', EDF_START_TIME_SZ);
    g_string_printf(temp, "%02d.%02d.%02d",
            g_date_time_get_hour(priv->date_and_time),
            g_date_time_get_minute(priv->date_and_time),
            g_date_time_get_second(priv->date_and_time)
            );
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_START_TIME_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_START_TIME_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write header size
    memset (buffer, ' ', EDF_NUM_BYTES_IN_HEADER_SZ);
    g_string_printf(temp, "%d", edf_header_num_bytes(header));
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_NUM_BYTES_IN_HEADER_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_NUM_BYTES_IN_HEADER_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write reserved
    memset (buffer, ' ', EDF_RESERVED_SZ);
    memcpy(buffer,
            priv->reserved->str,
            MIN(priv->reserved->len, EDF_RESERVED_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_RESERVED_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write number of data records
    memset (buffer, ' ', EDF_NUM_DATA_REC_SZ);
    g_string_printf(temp, "%d", priv->num_records);
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_NUM_DATA_REC_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_NUM_DATA_REC_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;
    
    // Write duration of data records
    memset (buffer, ' ', EDF_DURATION_OF_DATA_RECORD_SZ);
    g_string_printf(temp, "%lf", priv->duration_of_record);
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_NUM_DATA_REC_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_NUM_DATA_REC_SZ, &sz, NULL, error
            );
    written_size += sz;
    if (!result)
        goto fail;

    // Write number of signals
    memset (buffer, ' ', EDF_NUM_SIGNALS_SZ);
    g_string_printf(temp, "%d", priv->signals->len);
    memcpy(buffer,
            temp->str,
            MIN(temp->len, EDF_NUM_SIGNALS_SZ)
          );
    result = g_output_stream_write_all(
            ostream, buffer, EDF_NUM_SIGNALS_SZ, &sz, NULL, error
            );
    written_size += sz;

    if (!result)
        goto fail;

    // Write Signal related header info
    // Write labels
    for (gsize i = 0; i < priv->signals->len; i++) {
        const char* var;
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        
        memset(buffer, ' ', EDF_LABEL_SZ);
        var = edf_signal_get_label(signal);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_LABEL_SZ)
               );

        result = g_output_stream_write_all(
                ostream, buffer, EDF_LABEL_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // Write transducers
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char*var = edf_signal_get_transducer(signal);

        memset(buffer, ' ', EDF_TRANDUCER_TYPE_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_TRANDUCER_TYPE_SZ)
               );

        result = g_output_stream_write_all(
                ostream, buffer, EDF_TRANDUCER_TYPE_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // Write physical dimension
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char* var = edf_signal_get_physical_dimension(signal);

        memset(buffer, ' ', EDF_PHYSICAL_DIMENSION_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_PHYSICAL_DIMENSION_SZ)
               );

        result = g_output_stream_write_all(
                ostream, buffer, EDF_PHYSICAL_DIMENSION_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }
        
    // Write physical minimum
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gdouble doublevar = edf_signal_get_physical_min(signal);

        memset(buffer, ' ', EDF_PHYSICAL_MINIMUM_SZ);
        g_string_printf(temp, "%lf", doublevar);
        memcpy(buffer,
                temp->str,
                MIN(temp->len, EDF_PHYSICAL_MINIMUM_SZ)
              );
        result = g_output_stream_write_all(
                ostream, buffer, EDF_PHYSICAL_MINIMUM_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // Write physical maximum
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gdouble doublevar = edf_signal_get_physical_max(signal);

        memset(buffer, ' ', EDF_PHYSICAL_MAXIMUM_SZ);
        g_string_printf(temp, "%lf", doublevar);
        memcpy(buffer,
                temp->str,
                MIN(temp->len, EDF_PHYSICAL_MAXIMUM_SZ)
              );
        result = g_output_stream_write_all(
                ostream, buffer, EDF_PHYSICAL_MAXIMUM_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // Write digital minimum
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gint intvar = edf_signal_get_digital_min(signal);

        memset(buffer, ' ', EDF_DIGITAL_MINIMUM_SZ);
        g_string_printf(temp, "%d", intvar);
        memcpy(buffer,
                temp->str,
                MIN(temp->len, EDF_DIGITAL_MINIMUM_SZ)
              );
        result = g_output_stream_write_all(
                ostream, buffer, EDF_DIGITAL_MINIMUM_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // Write digital maximum
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        gint intvar = edf_signal_get_physical_max(signal);

        memset(buffer, ' ', EDF_DIGITAL_MAXIMUM_SZ);
        g_string_printf(temp, "%d", intvar);
        memcpy(buffer,
                temp->str,
                MIN(temp->len, EDF_DIGITAL_MAXIMUM_SZ)
              );
        result = g_output_stream_write_all(
                ostream, buffer, EDF_DIGITAL_MAXIMUM_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // write prefiltering
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char* var = edf_signal_get_prefiltering(signal);
        
        memset(buffer, ' ', EDF_PREFILTERING_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_PREFILTERING_SZ)
               );

        result = g_output_stream_write_all(
                ostream, buffer, EDF_PREFILTERING_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }
        
    // write num samples per records 
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        guint32 guintvar = edf_signal_get_num_samples_per_record(signal);
        
        memset(buffer, ' ', EDF_NUM_SAMPLES_PER_RECORD_SZ);
        g_string_printf(temp, "%u", guintvar);
        memcpy(buffer,
               temp->str,
               MIN(temp->len, EDF_NUM_SAMPLES_PER_RECORD_SZ)
               );

        result = g_output_stream_write_all(
                ostream, buffer, EDF_NUM_SAMPLES_PER_RECORD_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    // write signal reserved
    for (gsize i = 0; i < priv->signals->len; i++) {
        EdfSignal* signal = g_ptr_array_index(priv->signals, i);
        const char*var = edf_signal_get_reserved(signal);
        
        memset(buffer, ' ', EDF_NS_RESERVED_SZ);
        memcpy(buffer,
               var,
               MIN(strlen(var), EDF_NS_RESERVED_SZ)
               );

        result = g_output_stream_write_all(
                ostream, buffer, EDF_NS_RESERVED_SZ, &sz, NULL, error
                );
        written_size += sz;
        if (!result)
            goto fail;
    }

    g_assert(written_size == edf_header_size(header));

fail:
    g_string_free(temp, TRUE);
    return written_size;
}

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

    g_assert(nread == edf_header_size(header));

    return nread;
}

gint
edf_header_get_version(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), -1);

    priv = edf_header_get_instance_private (header);
    return priv->version;
}

const gchar*
edf_header_get_patient(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), NULL);

    priv = edf_header_get_instance_private (header);
    return priv->local_patient_identification->str;
}

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

const gchar*
edf_header_get_recording(EdfHeader* header)
{
    EdfHeaderPrivate *priv;
    g_return_val_if_fail(EDF_IS_HEADER (header), NULL);

    priv = edf_header_get_instance_private (header);
    return priv->local_recording_identification->str;
}

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
 * edf_header_num_bytes:
 * @param header :(in)
 *
 * Returns: The number of bytes in the header should be
 *          256 + 256 * number of signals
 */
gint
edf_header_num_bytes(EdfHeader* hdr)
{
    g_return_val_if_fail(EDF_IS_HEADER(hdr), -1);
    uint nbytes;
    g_object_get(hdr, "num-bytes-header", &nbytes, NULL);
    return (int)nbytes;
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

/* ************ utility functions ************** */
/* these are not coupled with EdfHeader */

gsize
edf_compute_header_size(guint num_signals)
{
    return EDF_BASE_HEADER_SIZE + num_signals * EDF_SIGNAL_HEADER_SIZE;
}
