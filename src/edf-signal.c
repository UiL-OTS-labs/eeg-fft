

#include "edf-signal.h"
#include "edf-size-priv.h"

#include "glibconfig.h"
#include <glib.h>
#include <gmodule.h>
#include <errno.h>
#include <string.h>

G_DEFINE_QUARK(edf_signal_error_quark, edf_signal_error)


/*
 * Signals are records after each other. In each
 * record there are a number of samples see
 * _EdfSignalPrivate, the num_samples_per_record (ns)
 * member.
 * This means that for each record ns bytes per sample
 * are recorded.
 */

typedef union {
    gint16  i16;
    guint16 u16;
    gint32  i32;
    guint32 u32;
    guint8  array[4];
}sample_repr;

typedef struct _record {
    guint8* bytes;          /* the bytes representing the signal */
    guint   ns;             /* the number of samples in a complete record */
    guint   sizeof_sample;  /* the number of bytes of one sample */
    guint   ns_stored;      /* the number of samples currently stored */
} EdfRecord;

static EdfRecord*
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

static void
edf_record_destroy(EdfRecord* record)
{
    g_free(record->bytes);
    g_free(record);
}

static gsize
edf_record_size(const EdfRecord* record)
{
    return record->ns_stored;
}

static void
edf_record_append(EdfRecord* record, gint value)
{
    sample_repr repr;
    repr.i16 = GINT16_TO_LE(value);
    memcpy(&record->bytes[record->sizeof_sample * record->ns_stored],
            repr.array,
            sizeof(repr.i16)
          );
    record->ns_stored++;
}

typedef struct _EdfSignalPrivate {
    /* recording info.*/
    GString*    label;
    GString*    transducer_type;
    GString*    physical_dimension;
    gdouble     physical_min;
    gdouble     physical_max;
    gint        digital_min;
    gint        digital_max;
    GString*    prefiltering;
    gint        num_samples_per_record;
    GString*    reserved;
    guint       sample_size;
    GPtrArray*  records;
} EdfSignalPrivate;


G_DEFINE_TYPE_WITH_PRIVATE(EdfSignal, edf_signal, G_TYPE_OBJECT)


static void
edf_signal_init(EdfSignal* self)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(self);

    priv->label = g_string_new("");
    priv->transducer_type = g_string_new("");
    priv->physical_dimension = g_string_new("");

    priv->physical_min = 0;
    priv->physical_max = 0;
    priv->digital_min = 0;
    priv->digital_max = 0;

    priv->prefiltering = g_string_new("");
    priv->num_samples_per_record = 0;

    priv->reserved = g_string_new("");

    priv->records = g_ptr_array_new_with_free_func((GDestroyNotify) edf_record_destroy);
}

static void
edf_signal_dispose(GObject* gobject)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(EDF_SIGNAL(gobject));

    // Drop references on objects
    g_ptr_array_unref(priv->records);
    priv->records = NULL;

    // Chain up to parent
    G_OBJECT_CLASS(edf_signal_parent_class)->dispose(gobject);
}

static void
edf_signal_finalize(GObject* gobject)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(EDF_SIGNAL(gobject));

    // free remaining resources.
    g_string_free(priv->label, TRUE);
    g_string_free(priv->transducer_type, TRUE);
    g_string_free(priv->physical_dimension, TRUE);
    g_string_free(priv->prefiltering, TRUE);
    g_string_free(priv->reserved, TRUE);
    
    // Chain up to parent
    G_OBJECT_CLASS(edf_signal_parent_class)->dispose(gobject);
}

typedef enum {
    PROP_LABEL = 1,
    PROP_TRANDUCER,
    PROP_PHYSICAL_DIMENSION,
    PROP_PHYSICAL_MIN,
    PROP_PHYSICAL_MAX,
    PROP_DIGITAL_MIN,
    PROP_DIGITAL_MAX,
    PROP_PREFILTERING,
    PROP_NUM_SAMPLES_PER_RECORD,
    PROP_RESERVED,
    PROP_SAMPLE_SIZE,
    PROP_NUM_RECORDS,
    N_PROPERTIES
} EdfSignalProperty;

static void
edf_signal_set_property (
        GObject        *object,
        guint32         propid,
        const GValue   *value,
        GParamSpec     *spec
    )
{
    EdfSignal* self = EDF_SIGNAL(object);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(self);

    switch ((EdfSignalProperty) propid) {
        case PROP_LABEL:
            edf_signal_set_label(self, g_value_get_string(value));
            break;
        case PROP_TRANDUCER:
            edf_signal_set_transducer(self, g_value_get_string(value));
            break;
        case PROP_PHYSICAL_DIMENSION:
            edf_signal_set_physical_dimension(self, g_value_get_string(value));
            break;
        case PROP_PHYSICAL_MIN:
            edf_signal_set_physical_min(self, g_value_get_double(value));
            break;
        case PROP_PHYSICAL_MAX:
            edf_signal_set_physical_max(self, g_value_get_double(value));
            break;
        case PROP_DIGITAL_MIN:
            edf_signal_set_digital_min(self, g_value_get_int(value));
            break;
        case PROP_DIGITAL_MAX:
            edf_signal_set_digital_max(self, g_value_get_int(value));
            break;
        case PROP_PREFILTERING:
            edf_signal_set_prefiltering(self, g_value_get_string(value));
            break;
        case PROP_NUM_SAMPLES_PER_RECORD:
            edf_signal_set_num_samples_per_record(self, g_value_get_uint(value));
            break;
        case PROP_RESERVED:
            edf_signal_set_reserved(self, g_value_get_string(value));
            break;
        case PROP_SAMPLE_SIZE:
            priv->sample_size = g_value_get_uint(value);
            break;
        case PROP_NUM_RECORDS: // Read only
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, spec);
    }
}

static void
edf_signal_get_property (
        GObject    *object,
        guint32     propid,
        GValue     *value,
        GParamSpec *spec
    )
{
    EdfSignal* self = EDF_SIGNAL(object);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(self);

    switch ((EdfSignalProperty) propid) {
        case PROP_LABEL:
            g_value_set_string(value, priv->label->str);
            break;
        case PROP_TRANDUCER:
            g_value_set_string(value, priv->transducer_type->str);
            break;
        case PROP_PHYSICAL_DIMENSION:
            g_value_set_string(value, priv->physical_dimension->str);
            break;
        case PROP_PHYSICAL_MIN:
            g_value_set_double(value, priv->physical_min);
            break;
        case PROP_PHYSICAL_MAX:
            g_value_set_double(value, priv->physical_max);
            break;
        case PROP_DIGITAL_MIN:
            g_value_set_int(value, priv->digital_min);
            break;
        case PROP_DIGITAL_MAX:
            g_value_set_int(value, priv->digital_max);
            break;
        case PROP_PREFILTERING:
            g_value_set_string(value, priv->prefiltering->str);
            break;
        case PROP_NUM_SAMPLES_PER_RECORD:
            g_value_set_uint(value, priv->num_samples_per_record);
            break;
        case PROP_RESERVED:
            g_value_set_string(value, priv->reserved->str);
            break;
        case PROP_SAMPLE_SIZE:
            g_value_set_uint(value, priv->sample_size);
            break;
        case PROP_NUM_RECORDS:
            g_value_set_int(value, edf_signal_get_num_records(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, spec);
    }
}

static GParamSpec *edf_signal_properties[N_PROPERTIES] = {NULL,};

static void
edf_signal_class_init(EdfSignalClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = edf_signal_set_property;
    object_class->get_property = edf_signal_get_property;
    object_class->dispose = edf_signal_dispose;
    object_class->finalize = edf_signal_finalize;

    edf_signal_properties[PROP_LABEL] = g_param_spec_string(
            "label",
            "Label",
            "The name of a signal e.g. EEG Fpz-Cz or Body temp",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    edf_signal_properties[PROP_TRANDUCER] = g_param_spec_string(
            "transducer",
            "transducer-type",
            "The name of the transducer or ADConverter eeg AgAgCl electrode",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    edf_signal_properties[PROP_PHYSICAL_DIMENSION] = g_param_spec_string(
            "physical-dimension",
            "unit",
            "The physical dimension of the transducer",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    edf_signal_properties[PROP_PHYSICAL_MIN] = g_param_spec_double(
            "physical-min",
            "physical-minimum",
            "The physical minimum of the transducer",
            -1e99,
            1e99,
            0,
            G_PARAM_READWRITE
            );

    edf_signal_properties[PROP_PHYSICAL_MAX] = g_param_spec_double(
            "physical-max",
            "physical-maximum",
            "The physical maximum of the transducer",
            -1e99,
            1e99,
            0,
            G_PARAM_READWRITE
            );

    edf_signal_properties [PROP_DIGITAL_MIN] = g_param_spec_int(
            "digital-min",
            "digital-minimum",
            "The digital minimum of an signal",
            G_MININT16,
            G_MAXINT16,
            0,
            G_PARAM_READWRITE
            );

    edf_signal_properties [PROP_DIGITAL_MAX] = g_param_spec_int(
            "digital-max",
            "digital-maximum",
            "The digital maximum of an signal",
            G_MININT16,
            G_MAXINT16,
            0,
            G_PARAM_READWRITE
            );

    edf_signal_properties [PROP_PREFILTERING] = g_param_spec_string(
            "prefilter",
            "prefiltering",
            "The prefilter used for this channel",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );
    
    edf_signal_properties [PROP_NUM_SAMPLES_PER_RECORD] = g_param_spec_uint(
            "ns",
            "num-samples-per-record",
            "The number of sample per record for this signal.",
            0,
            G_MAXUINT,
            0,
            G_PARAM_READWRITE
            );

    edf_signal_properties [PROP_RESERVED] = g_param_spec_string(
            "reserved",
            "Signal-Reserved",
            "Reserved info for an signal",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    edf_signal_properties[PROP_SAMPLE_SIZE] = g_param_spec_uint(
        "sample-size",
        "Sample-Size",
        "The number of bytes of a sample",
        1,
        8,
        2,
        G_PARAM_READWRITE| G_PARAM_CONSTRUCT_ONLY | G_PARAM_PRIVATE
    );

    edf_signal_properties[PROP_NUM_RECORDS] = g_param_spec_uint(
        "num-records",
        "Number of Records",
        "The number of records contained in this signal.",
        0,
        G_MAXUINT,
        0,
        G_PARAM_READABLE
        );

    g_object_class_install_properties(
            object_class, N_PROPERTIES, edf_signal_properties
            );
}

static gsize
signal_capacity(EdfSignal* signal)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->records->len * priv->num_samples_per_record;
}

static gsize
signal_size (EdfSignal* signal)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    gsize size = 0;
    if (priv->records->len == 0)
        return size;

    EdfRecord* record = g_ptr_array_index(priv->records, priv->records->len -1);

    size = (priv->records->len - 1) * priv->num_samples_per_record;
    size += edf_record_size(record);
    return size;
}


static void
signal_append_private(EdfSignal* signal, gint value, GError** error)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    gsize capacity, size;
    EdfRecord* rec = NULL;
    capacity = signal_capacity(signal);
    size = signal_size(signal);

    g_assert(size <= capacity);
    if (size == capacity) {
        rec = edf_record_new(priv->num_samples_per_record, sizeof(gint16), error);
        if (*error) {
            g_assert(rec == NULL);
            return;
        }
        g_ptr_array_add(priv->records, rec);
    }
    rec = g_ptr_array_index(priv->records, priv->records->len - 1);
    edf_record_append(rec, value);
}

EdfSignal*
edf_signal_new() {
    EdfSignal* signal;
    signal = g_object_new(EDF_TYPE_SIGNAL, NULL);
    return signal;
}

EdfSignal*
edf_signal_new_full(
        const gchar    *label,
        const gchar    *transducer,
        const gchar    *phys_dim,
        gdouble         physical_min,
        gdouble         physical_max,
        int             digital_min,
        int             digital_max,
        const gchar    *prefilter,
        guint           ns
        )
{
    EdfSignal* new = g_object_new(
            EDF_TYPE_SIGNAL,
            "label", label,
            "transducer", transducer,
            "physical_dimension", phys_dim,
            "physical_min", physical_min,
            "physical_max", physical_max,
            "digital_min", digital_min,
            "digital_max", digital_max,
            "prefilter", prefilter,
            "ns", ns,
            NULL
            );
    return new;
}

void
edf_signal_destroy(EdfSignal* signal) {
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_object_unref(G_OBJECT(signal));
}

const gchar*
edf_signal_get_label(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->label->str;
}

void
edf_signal_set_label(EdfSignal* signal, const gchar* label)
{
    EdfSignalPrivate *priv;
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(g_str_is_ascii(label));
    char temp[256];

    priv = edf_signal_get_instance_private(signal);
    g_snprintf(temp, sizeof(temp), "%s", label);
    g_strstrip(temp);
    temp[EDF_LABEL_SZ] = '\0';
    g_string_assign(priv->label, temp);
}

const gchar*
edf_signal_get_transducer(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->transducer_type->str;
}

void
edf_signal_set_transducer(EdfSignal* signal, const gchar* transducer)
{
    EdfSignalPrivate *priv;
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(g_str_is_ascii(transducer));
    char temp[256];

    priv = edf_signal_get_instance_private(signal);
    g_snprintf(temp, sizeof(temp), "%s", transducer);
    g_strstrip(temp);
    temp[EDF_TRANDUCER_TYPE_SZ] = '\0';
    g_string_assign (priv->transducer_type, temp);
}

const gchar*
edf_signal_get_physical_dimension(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->physical_dimension->str;
}

void
edf_signal_set_physical_dimension(EdfSignal* signal, const gchar* dimension)
{
    EdfSignalPrivate *priv;
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(g_str_is_ascii(dimension));
    char temp[256];

    priv = edf_signal_get_instance_private(signal);
    g_snprintf(temp, sizeof(temp), "%s", dimension);
    g_strstrip(temp);
    g_string_printf (priv->physical_dimension, "%s", temp);
}

gdouble
edf_signal_get_physical_min(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->physical_min;
}

void
edf_signal_set_physical_min(EdfSignal* signal, gdouble min)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->physical_min = min;
}

gdouble
edf_signal_get_physical_max(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->physical_max;
}

void
edf_signal_set_physical_max(EdfSignal* signal, gdouble max)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->physical_max = max;
}

gint
edf_signal_get_digital_min(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->digital_min;
}

void
edf_signal_set_digital_min(EdfSignal* signal, gint min)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->digital_min = min;
}

gint
edf_signal_get_digital_max(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->digital_max;
}

void
edf_signal_set_digital_max(EdfSignal* signal, gint max)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->digital_max = max;
}

const gchar*
edf_signal_get_prefiltering(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->prefiltering->str;
}

void
edf_signal_set_prefiltering(EdfSignal* signal, const gchar* prefiltering)
{
    EdfSignalPrivate *priv;
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(g_str_is_ascii(prefiltering));
    char temp[256];

    priv = edf_signal_get_instance_private(signal);
    g_snprintf(temp, sizeof(temp), "%s", prefiltering);
    g_strstrip(temp);
    temp[EDF_PREFILTERING_SZ] = '\0';
    g_string_assign (priv->prefiltering, temp);
}

void
edf_signal_append_digital(EdfSignal* signal, gint value, GError** error)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(error != NULL && *error == NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);

    if (value > priv->digital_max || value < priv->digital_min) {
        g_set_error(
                error,
                EDF_SIGNAL_ERROR,
                EDF_SIGNAL_ERROR_OUT_OF_RANGE,
                "The value %d is outside the range [%d, %d]",
                value, priv->digital_min, priv->digital_max
                );
        return;
    }
    signal_append_private(signal, value, error);
}

guint
edf_signal_get_num_records(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), -1);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->records->len;
}

const gchar*
edf_signal_get_reserved(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->reserved->str;
}

void
edf_signal_set_reserved(EdfSignal* signal, const gchar* reserved)
{
    EdfSignalPrivate *priv;
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(g_str_is_ascii(reserved));
    char temp[256];

    priv = edf_signal_get_instance_private(signal);
    g_snprintf(temp, sizeof(temp), "%s", reserved);
    g_strstrip(temp);
    temp[EDF_NS_RESERVED_SZ] = '\0';
    g_string_assign(priv->reserved, temp);
}

gint
edf_signal_get_num_samples_per_record(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), -1);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->num_samples_per_record;
}

void
edf_signal_set_num_samples_per_record(EdfSignal* signal, guint num_samples)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->num_samples_per_record = num_samples;
}

void
edf_signal_write_record_to_ostream(
        EdfSignal        *signal,
        GOutputStream    *ostream,
        guint             nrec,
        GError          **error
        )
{
    g_return_if_fail(EDF_IS_SIGNAL(signal) && G_IS_OUTPUT_STREAM(ostream));
    g_return_if_fail(error != NULL && *error == NULL);
    gsize bytes_written;

    EdfSignalPrivate *priv = edf_signal_get_instance_private(signal);

    g_return_if_fail(nrec < priv->records->len);

    EdfRecord* record = g_ptr_array_index(priv->records, nrec);
    g_output_stream_write_all(
            ostream,
            record->bytes,
            record->ns * record->sizeof_sample,
            &bytes_written,
            NULL,
            error
            );
    if (bytes_written != record->ns * record->sizeof_sample) {
        g_critical("Bytes written is %lu where %u was expected",
                  bytes_written, record->ns * record->sizeof_sample);
    }
}

gsize
edf_signal_read_record_from_istream(
    EdfSignal     *signal,
    GInputStream  *istream,
    GError       **error
    )
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal) && G_IS_INPUT_STREAM(istream), 0);
    g_return_val_if_fail(error && *error == NULL, 0);

    gsize numread = 0;
    EdfSignalPrivate *priv = edf_signal_get_instance_private(signal);
    gsize memchunksize = priv->num_samples_per_record * priv->sample_size;

    EdfRecord* record = edf_record_new (
        priv->num_samples_per_record,
        priv->sample_size,
        error
    );

    if (*error) {
        if (record)
            edf_record_destroy(record);
        return numread;
    }

    if (
        g_input_stream_read_all(
            istream, record->bytes, memchunksize, &numread, NULL, error
        ) != TRUE
    ) {
        goto fail;
    }
    g_ptr_array_add (priv->records, record);
    return numread;
    
fail:
    return numread;
}


