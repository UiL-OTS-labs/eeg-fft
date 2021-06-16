

#include "edf-signal.h"
#include "edf-size-priv.h"
#include "edf-record-priv.h"

#include "glibconfig.h"
#include <glib.h>
#include <gmodule.h>
#include <errno.h>
#include <string.h>
#include <edf-signal-private.h>

G_DEFINE_QUARK(edf_signal_error_quark, edf_signal_error)


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
    G_OBJECT_CLASS(edf_signal_parent_class)->finalize(gobject);
}

static void
append_new_record(EdfSignal* self, GError **error)
{
    EdfSignalPrivate *priv = edf_signal_get_instance_private(self);
    EdfRecord* rec = edf_record_new(
        priv->num_samples_per_record, sizeof(gint16), error
    );
    if (*error) {
        g_assert(rec == NULL);
        return;
    }
    edf_signal_append_record(self, rec);
}

void
edf_signal_append_record(EdfSignal* self, EdfRecord *rec)
{
    g_return_if_fail(EDF_IS_SIGNAL(self));
    g_return_if_fail(rec != NULL);

    EdfSignalPrivate *priv = edf_signal_get_instance_private(self);

    g_ptr_array_add(priv->records, rec);
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
    PROP_SIGNAL,
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
        case PROP_SIGNAL: // Read only
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
        case PROP_SIGNAL:
            g_value_set_boxed(value, edf_signal_get_values(self));
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

    klass->append_new_record = append_new_record;

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
    
    edf_signal_properties[PROP_SIGNAL] = g_param_spec_boxed(
        "signal",
        "Signal",
        "The signal in the physical dimension as specified by the signal.",
        G_TYPE_ARRAY,
        G_PARAM_READABLE
        );

    g_object_class_install_properties(
            object_class, N_PROPERTIES, edf_signal_properties
            );
}

/**
 * edf_signal_new:(constructor)
 *
 * Create a default new Edf signal.
 */
EdfSignal*
edf_signal_new() {
    EdfSignal* signal;
    signal = g_object_new(EDF_TYPE_SIGNAL, NULL);
    return signal;
}

/**
 * edf_signal_new_full:(constructor)
 * @label :(transfer none) : A label for the signal
 * @transducer : (transfer none) : The used transducer fo the signal
 * @phys_dim : (transfer none) : The unit or physical dimension of the signal.
 * @physical_min :: the minimum physical value the signal can take.
 * @physical_max :: the maximum physical value the signal can take.
 * @digital_min :: the minimum digital value the signal can take.
 * @digital_max :: the maximum digital value the signal can take.
 * @prefiltering :(transfer none): the used prefiltering.
 * @ns :: The number of samples per record.
 *
 * Create a new Edf signal with all parameters set to the specified values.
 *
 * Returns:(transfer full): A new edf signal.
 */
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

/**
 * edf_signal_destroy:(skip)
 * @signal :: The signal to destroy
 *
 * Decrements the reference of signal and will
 * free the associated resources when the refcnt
 * drops to zero.
 */
void
edf_signal_destroy(EdfSignal* signal) {
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_object_unref(G_OBJECT(signal));
}

/**
 * edf_signal_get_label :
 *
 * Returns :(transfer none): the label of the signal.
 */
const gchar*
edf_signal_get_label(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->label->str;
}

/**
 * edf_signal_set_label:
 * @signal: The #EdfSignal whose label to update
 * @label: ascii string
 *
 * The @label will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
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

/**
 * edf_signal_get_transducer:
 *
 * Returns :(transfer none): the transducer of the signal.
 */
const gchar*
edf_signal_get_transducer(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->transducer_type->str;
}

/**
 * edf_signal_set_transducer:
 * @signal: The #EdfSignal whose transducer to update
 * @transducer: ascii string
 *
 * The @transducer will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
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

/**
 * edf_signal_get_physical_dimension:
 *
 * Returns :(transfer none): the physical dimension (unit) of the signal.
 */
const gchar*
edf_signal_get_physical_dimension(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->physical_dimension->str;
}

/**
 * edf_signal_set_physical_dimension:
 * @signal: The #EdfSignal whose transducer to update
 * @dimension: ascii string eg uV for microVolt
 *
 * The @dimension will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
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

/**
 * edf_signal_get_physical_min:
 * @signal:: the input signal
 *
 * obtain the physical minimum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the minimum a value can have in its physical dimension
 */
gdouble
edf_signal_get_physical_min(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->physical_min;
}

/**
 * edf_signal_set_physical_min:
 * @signal: the input signal
 * @min: The minimum this value should be less than the max
 *
 * set the physical minimum of the signal
 */
void
edf_signal_set_physical_min(EdfSignal* signal, gdouble min)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->physical_min = min;
}

/**
 * edf_signal_get_physical_max:
 * @signal:: the input signal
 *
 * obtain the physical maximum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the maximum a value can have in its physical dimension
 */
gdouble
edf_signal_get_physical_max(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->physical_max;
}

/**
 * edf_signal_set_physical_max:
 * @signal: the input signal
 * @max: The maximum this value should be greater than the max
 *
 * set the physical maximum of the signal
 */
void
edf_signal_set_physical_max(EdfSignal* signal, gdouble max)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->physical_max = max;
}

/**
 * edf_signal_get_digital_min:
 * @signal:: the input signal
 *
 * obtain the digital minimum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the minimum a value can have in its digital dimension
 */
gint
edf_signal_get_digital_min(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->digital_min;
}

/**
 * edf_signal_set_digital_min:
 * @signal: the input signal
 * @min: The minimum this value should be less than the max
 *
 * set the digital minimum of the signal which is typically the lowest
 * value obtained from an ADC.
 */
void
edf_signal_set_digital_min(EdfSignal* signal, gint min)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->digital_min = min;
}

/**
 * edf_signal_get_digital_max:
 * @signal:: the input signal
 *
 * obtain the digital maximum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the maximum a value can have in its digital dimension
 */
gint
edf_signal_get_digital_max(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), 0);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->digital_max;
}

/**
 * edf_signal_set_digital_max:
 * @signal: the input signal
 * @min: The maximum this value should be greater than the min
 *
 * set the digital maximum of the signal which is typically the highest
 * value obtained from an ADC.
 */
void
edf_signal_set_digital_max(EdfSignal* signal, gint max)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->digital_max = max;
}

/**
 * edf_signal_get_prefiltering :
 *
 * Returns :(transfer none): the prefiltering of the signal.
 */
const gchar*
edf_signal_get_prefiltering(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->prefiltering->str;
}

/**
 * edf_signal_set_prefiltering:
 * @signal: The #EdfSignal whose prefilter to update
 * @prefilter: ascii string eg HP:20 for an highpass 20 Hz filter
 *
 * The @prefilter will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
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

/**
 * edf_signal_append:
 * @signal:in: the signal to append a value to.
 * @value:in: the value to append the value is valid for digital_minimum < value < digital_maximum
 * @error:out: If the value is out of bounds or there isn't memory enough an error can be returned here.
 *
 * Append a digital value to the signal.
 */
void
edf_signal_append_digital(EdfSignal* signal, gint value, GError** error)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    g_return_if_fail(error != NULL && *error == NULL);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    EdfSignalClass* klass = EDF_SIGNAL_GET_CLASS(signal);
    EdfRecord* rec = NULL;
    gsize capacity, size;

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

    capacity = edf_signal_capacity(signal);
    size = edf_signal_size(signal);
    g_assert(size <= capacity);
    if (size == capacity) {
        klass->append_new_record(signal, error);
        if (*error) {
            return;
        }
    }

    rec = g_ptr_array_index(priv->records, priv->records->len - 1);
    edf_record_append(rec, value);
}

/**
 * edf_signal_get_num_records:
 * @signal:
 *
 * This field doesn't end in the header per signal, but for all signals
 * it can be used to check if all signals of an #EdfFile contain a equal
 * number of records.
 *
 * Returns: the number of records contained in this #EdfSignal
 */
guint
edf_signal_get_num_records(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), -1);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->records->len;
}

/**
 * edf_signal_get_reserved:
 * @signal:(in): the signal whose reserved info you would like to know
 *
 * Obtain the reserved information of a signal
 *
 * Returns: a pointer to the string of the signal.
 */
const gchar*
edf_signal_get_reserved(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->reserved->str;
}

/**
 * edf_signal_set_reserved:
 * @signal: The #EdfSignal whose transducer to update
 * @reserved: reserved information
 *
 * The @dimension will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
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

/**
 * edf_signal_get_num_samples_per_record:
 *
 * Returns: the number of samples in each record
 */
gint
edf_signal_get_num_samples_per_record(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), -1);
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->num_samples_per_record;
}

/**
 * edf_signal_set_num_samples_per_record:
 * @signal: the input signal
 * @num_samples: The number of samples in each record
 *
 * This sets the number of samples contained in the signal in each record
 */
void
edf_signal_set_num_samples_per_record(EdfSignal* signal, guint num_samples)
{
    g_return_if_fail(EDF_IS_SIGNAL(signal));
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    priv->num_samples_per_record = num_samples;
}

/**
 * edf_signal_get_values
 * @signal: the signal whose value you would like to read.
 *
 * Returns:(transfer full) (element-type gdouble): A list of doubles that is represents
 * the signal that is recorded.
 */
GArray*
edf_signal_get_values(EdfSignal* signal)
{
    g_return_val_if_fail(EDF_IS_SIGNAL(signal), NULL);

    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);

    size_t size = edf_signal_get_num_records(signal) *
                  edf_signal_get_num_samples_per_record(signal);

    GArray* ret = g_array_sized_new(FALSE, FALSE, sizeof(gdouble), size);
    g_return_val_if_fail(ret, NULL);

    int dig_min = edf_signal_get_digital_min(signal);
    int dig_max = edf_signal_get_digital_max(signal);
    double phys_min = edf_signal_get_physical_min(signal);
    double phys_max = edf_signal_get_physical_max(signal);
    double phys_span = phys_max - phys_min;
    int dig_span = dig_max - dig_min;
    double step = phys_span / dig_span;

    for (gsize nrec = 0; nrec < priv->records->len; nrec++) {
        EdfRecord* record = g_ptr_array_index(priv->records, nrec);
        for (gsize i = 0; i < (unsigned) priv->num_samples_per_record; i++) {
            int digital_val = edf_record_get(record, i);
            gdouble val = phys_min + step * (digital_val - dig_min);
            g_array_append_val(ret, val);
        }
    }
    return ret;
}

/**
 * edf_signal_write_record_to_ostream:(skip)
 */
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

/**
 * edf_signal_read_record_from_istream:(skip)
 */
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

gsize
edf_signal_capacity(EdfSignal* signal)
{
    EdfSignalPrivate* priv = edf_signal_get_instance_private(signal);
    return priv->records->len * priv->num_samples_per_record;
}

gsize
edf_signal_size (EdfSignal* signal)
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

