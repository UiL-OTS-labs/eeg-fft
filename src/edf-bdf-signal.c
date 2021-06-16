
#include "edf-bdf-signal.h"
#include "edf-record-priv.h"
#include "edf-signal-private.h"

/**
 * SECTION:edf-bdf-signal
 * @short_description: an object that represents a biosemi signal
 * @see_also: #EdfSignal, #EdfHeader
 * @include: gedf.h
 *
 * A BDF signal is a sub-flavor of an EDF signal. It is a continuous signal
 * that with samples in a 24 bits two's complement integer format.
 */

G_DEFINE_TYPE(EdfBdfSignal, edf_bdf_signal, EDF_TYPE_SIGNAL)

static void
append_new_record(EdfSignal* self, GError** error)
{
    EdfRecord* rec = edf_record_new(
        edf_signal_get_num_samples_per_record(self), 3, error
    );
    if (*error) {
        g_assert(rec == NULL);
        return;
    }
    edf_signal_append_record(self, rec);
}

static void
edf_bdf_signal_init(EdfBdfSignal* self)
{
    // init occurs in parent class
    (void) self;
}



typedef enum {
    PROP_NULL,
    PROP_DIGITAL_MAX,
    PROP_DIGITAL_MIN,
    N_PROPERTIES
} EdfBdfSignalProperty;

static GParamSpec* obj_properties[N_PROPERTIES] = {0};

static void
set_property (
    GObject        *object,
    guint32         propid,
    const GValue   *value,
    GParamSpec     *pspec
)
{
    switch ((EdfBdfSignalProperty) propid) {
        case PROP_DIGITAL_MAX:
            edf_signal_set_digital_max(EDF_SIGNAL(object), g_value_get_int(value));
            break;
        case PROP_DIGITAL_MIN:
            edf_signal_set_digital_min(EDF_SIGNAL(object), g_value_get_int(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, pspec);
    }
}

static void
get_property (
    GObject    *object,
    guint32     propid,
    GValue     *value,
    GParamSpec *pspec
)
{
    EdfBdfSignal* self = EDF_BDF_SIGNAL(object);
    switch ((EdfBdfSignalProperty) propid) {
        case PROP_DIGITAL_MAX:
            g_value_set_int(value, edf_signal_get_digital_max(EDF_SIGNAL(self)));
            break;
        case PROP_DIGITAL_MIN:
            g_value_set_int(value, edf_signal_get_digital_min(EDF_SIGNAL(self)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, pspec);
    }
}

static void
edf_bdf_signal_class_init(EdfBdfSignalClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->get_property = get_property;
    EdfSignalClass* signal_class = EDF_SIGNAL_CLASS(klass);

    signal_class->append_new_record = append_new_record;

    obj_properties[PROP_DIGITAL_MAX] = g_param_spec_int(
        "bdf-digital-max",
        "Digital-Max",
        "The digital maximum of a BdfFile.",
        -8388608,
        8388607,
        0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT
        );

    obj_properties[PROP_DIGITAL_MIN] = g_param_spec_int(
        "bdf-digital-min",
        "Digital-Min",
        "The digital minimum of a BdfFile.",
        -8388608,
        8388607,
        0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT
    );

    g_object_class_override_property(object_class, PROP_DIGITAL_MAX, "digital-max");
    g_object_class_override_property(object_class, PROP_DIGITAL_MIN, "digital-min");

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);

}