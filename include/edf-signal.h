
#ifndef EDF_SIGNAL_H
#define EDF_SIGNAL_H

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define EDF_SIGNAL_ERROR edf_signal_error_quark()

/**
 * EdfSignalError:
 * @EDF_SIGNAL_ERROR_ENOMEM: an memory allocation failed
 * @EDF_SIGNAL_ERROR_INVALID_INDEX: Trying to index a sample that is out of bounds
 * @EDF_SIGNAL_ERROR_OUT_OF_RANGE: A sample beyond the digital minimum or maximum was added.
 * @EDF_SIGNAL_ERROR_FAILED: An unspecific error occurred.
 *
 * An error code returned an operation on an instance of
 * EdfSignal
 */
typedef enum {
    EDF_SIGNAL_ERROR_ENOMEM,
    EDF_SIGNAL_ERROR_INVALID_INDEX,
    EDF_SIGNAL_ERROR_OUT_OF_RANGE,
    EDF_SIGNAL_ERROR_FAILED,
}EdfSignalError;


/*
 * Type Declaration
 */
#define EDF_TYPE_SIGNAL edf_signal_get_type()
G_MODULE_EXPORT
G_DECLARE_DERIVABLE_TYPE(EdfSignal, edf_signal, EDF, SIGNAL, GObject)

struct _EdfSignalClass {
    GObjectClass parent_class;
};

G_MODULE_EXPORT GQuark
edf_signal_error_quark(void);

/**
 * edf_signal_new:(constructor)
 *
 * Create a default new Edf signal.
 */
G_MODULE_EXPORT EdfSignal*
edf_signal_new();

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
G_MODULE_EXPORT EdfSignal*
edf_signal_new_full(
        const gchar*    label,
        const gchar*    transducer,
        const gchar*    phys_dim,
        gdouble         physical_min,
        gdouble         physical_max,
        gint            digital_min,
        gint            digital_max,
        const gchar*    prefiltering,
        guint           ns
        );

/**
 * edf_signal_destroy:(skip)
 * @signal :: The signal to destroy
 *
 * Decrements the reference of signal and will
 * free the associated resources when the refcnt
 * drops to zero.
 */
G_MODULE_EXPORT void
edf_signal_destroy(EdfSignal* signal);

/**
 * edf_signal_get_label :
 *
 * Returns :(transfer none): the label of the signal.
 */
G_MODULE_EXPORT const gchar*
edf_signal_get_label(EdfSignal* signal);

/**
 * edf_signal_set_label:
 * @signal: The #EdfSignal whose label to update
 * @label: ascii string
 *
 * The @label will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
G_MODULE_EXPORT void
edf_signal_set_label(EdfSignal* signal, const gchar* label);

/**
 * edf_signal_transducer:
 *
 * Returns :(transfer none): the transducer of the signal.
 */
G_MODULE_EXPORT const gchar*
edf_signal_get_transducer(EdfSignal* signal);

/**
 * edf_signal_set_transducer:
 * @signal: The #EdfSignal whose transducer to update
 * @transducer: ascii string
 *
 * The @transducer will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
G_MODULE_EXPORT void
edf_signal_set_transducer(EdfSignal* signal, const gchar* transducer);

/**
 * edf_signal_get_physical_dimension:
 *
 * Returns :(transfer none): the physical dimension (unit) of the signal.
 */
G_MODULE_EXPORT const gchar*
edf_signal_get_physical_dimension(EdfSignal* signal);

/**
 * edf_signal_set_physical_dimension:
 * @signal: The #EdfSignal whose transducer to update
 * @dimension: ascii string eg uV for microVolt
 *
 * The @dimension will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
G_MODULE_EXPORT void
edf_signal_set_physical_dimension(EdfSignal* signal, const gchar* dimension);

/**
 * edf_signal_get_physical_min:
 * @signal:: the input signal
 *
 * obtain the physical minimum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the minimum a value can have in its physical dimension
 */
G_MODULE_EXPORT gdouble
edf_signal_get_physical_min(EdfSignal* signal);

/**
 * edf_signal_set_physical_min:
 * @signal: the input signal
 * @min: The minimum this value should be less than the max
 *
 * set the physical minimum of the signal
 */
 G_MODULE_EXPORT void
 edf_signal_set_physical_min(EdfSignal* signal, gdouble min);

/**
 * edf_signal_get_physical_max:
 * @signal:: the input signal
 *
 * obtain the physical maximum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the maximum a value can have in its physical dimension
 */
G_MODULE_EXPORT gdouble
edf_signal_get_physical_max(EdfSignal* signal);

/**
 * edf_signal_set_physical_max:
 * @signal: the input signal
 * @max: The maximum this value should be greater than the max
 *
 * set the physical maximum of the signal
 */
G_MODULE_EXPORT void
edf_signal_set_physical_max(EdfSignal* signal, gdouble max);

/**
 * edf_signal_get_digital_min:
 * @signal:: the input signal
 *
 * obtain the digital minimum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the minimum a value can have in its digital dimension
 */
G_MODULE_EXPORT gint
edf_signal_get_digital_min(EdfSignal* signal);

/**
 * edf_signal_set_digital_min:
 * @signal: the input signal
 * @min: The minimum this value should be less than the max
 *
 * set the digital minimum of the signal which is typically the lowest
 * value obtained from an ADC.
 */
G_MODULE_EXPORT void
edf_signal_set_digital_min(EdfSignal* signal, gint min);

/**
 * edf_signal_get_digital_max:
 * @signal:: the input signal
 *
 * obtain the digital maximum of the signal. This is relevant to compute
 * the final signal value for a digital input value.
 *
 * Returns: the maximum a value can have in its digital dimension
 */
G_MODULE_EXPORT gint
edf_signal_get_digital_max(EdfSignal* signal);

/**
 * edf_signal_set_digital_max:
 * @signal: the input signal
 * @min: The maximum this value should be greater than the min
 *
 * set the digital maximum of the signal which is typically the highest
 * value obtained from an ADC.
 */
G_MODULE_EXPORT void
edf_signal_set_digital_max(EdfSignal* signal, gint min);

/**
 * edf_signal_get_prefiltering :
 *
 * Returns :(transfer none): the prefiltering of the signal.
 */
G_MODULE_EXPORT const gchar*
edf_signal_get_prefiltering(EdfSignal* signal);

/**
 * edf_signal_set_prefiltering:
 * @signal: The #EdfSignal whose prefilter to update
 * @prefilter: ascii string eg HP:20 for an highpass 20 Hz filter
 *
 * The @prefilter will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
G_MODULE_EXPORT void
edf_signal_set_prefiltering(EdfSignal* signal, const gchar* prefilter);

/**
 * edf_signal_get_num_samples_per_record:
 *
 * Returns: the number of samples in each record
 */
 G_MODULE_EXPORT gint
 edf_signal_get_num_samples_per_record(EdfSignal* signal);

/**
 * edf_signal_set_num_samples_per_record:
 * @signal: the input signal
 * @num_samples: The number of samples in each record
 *
 * This sets the number of samples contained in the signal in each record
 */
G_MODULE_EXPORT void
edf_signal_set_num_samples_per_record(EdfSignal* signal, guint num_samples);

/**
 * edf_signal_get_reserved:
 * @signal:(in): the signal whose reserved info you would like to know
 *
 * Obtain the reserved information of a signal
 *
 * Returns: a pointer to the string of the signal.
 */
G_MODULE_EXPORT const char*
edf_signal_get_reserved(EdfSignal* signal);

/**
 * edf_signal_set_reserved:
 * @signal: The #EdfSignal whose transducer to update
 * @reserved: reserved information
 *
 * The @dimension will be set to the @signal. The label will be sanitized
 * that is it will be stripped from whitespace and must fit in the
 * header as written to file.
 */
 G_MODULE_EXPORT void
 edf_signal_set_reserved(EdfSignal* signal, const gchar* reserved);

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
 G_MODULE_EXPORT guint
 edf_signal_get_num_records(EdfSignal* signal);

/**
 * edf_signal_append:
 * @signal:in: the signal to append a value to.
 * @value:in: the value to append the value is valid for digital_minimum < value < digital_maximum
 * @error:out: If the value is out of bounds or there isn't memory enough an error can be returned here.
 *
 * Append a digital value to the signal.
 */
G_MODULE_EXPORT void
edf_signal_append_digital(EdfSignal* signal, gint value, GError** error);


/**
 * edf_signal_write_record_to_output_stream:(skip)
 */
void edf_signal_write_record_to_ostream(
        EdfSignal      *signal,
        GOutputStream  *ostream,
        guint           nrec,
        GError        **error
        );

/**
 * edf_signal_read_record_from_input_stream:(skip)
 */
gsize
edf_signal_read_record_from_istream (
    EdfSignal      *signal,
    GInputStream   *ostream,
    GError        **error
);

G_END_DECLS

// #ifndef EDF_SIGNAL_H
#endif
