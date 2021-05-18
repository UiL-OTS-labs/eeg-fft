
#include "edf-file.h"
#include "edf-header.h"
#include "edf-signal.h"
#include <gio/gio.h>

/**
 * SECTION:edf-file
 * @short_description: an object that reads/writes data in European Data Format
 * @see_also: #EdfSignal, #EdfHeader
 * @include: gedf.h
 *
 * #EdfFile is a file that contains a #EdfHeader and a number of #EdfSignal s
 * The header describes how many and what kind of signals are embedded in this
 * file. More information about the Edf format can be obtained from:
 * <ulink url="https://www.edfplus.info/specs/edf.html">edfplus.info</ulink>
 */

typedef struct _EdfFilePrivate {
    GFile*      file;
    EdfHeader*  header;
    GPtrArray*  signals;
}EdfFilePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(EdfFile, edf_file, G_TYPE_OBJECT)

typedef enum {
    PROP_FILENAME = 1,
    PROP_HEADER,
    PROP_SIGNALS,
    PROP_NUM_SIGNALS,
    N_PROPS
} EdfFileProperties;

static void
edf_file_init(EdfFile* file)
{
    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    priv->file = g_file_new_for_path("");
    priv->header = edf_header_new();
    priv->signals = g_ptr_array_new_full(0, g_object_unref);
    edf_header_set_signals(priv->header, priv->signals);
}

static void
edf_file_dispose(GObject* object)
{
    EdfFilePrivate* priv = edf_file_get_instance_private(EDF_FILE(object));

    g_clear_object(&priv->header);
    g_clear_object(&priv->file);

    g_ptr_array_unref(priv->signals);
    priv->signals = NULL;

    G_OBJECT_CLASS(edf_file_parent_class)->dispose(object);
}

static void
edf_file_finalize(GObject* object)
{
    EdfFilePrivate* priv = edf_file_get_instance_private(EDF_FILE(object));
    (void) priv;

    G_OBJECT_CLASS(edf_file_parent_class)->finalize(object);
}

static void
edf_file_write_records_to_ostream(
        EdfFile        *file,
        GOutputStream  *ostream,
        GError        **error
        )
{
    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    gint num_records_expected;

    g_assert(priv->signals->len > 0);
    EdfSignal* signal = g_ptr_array_index(priv->signals, 0);
    num_records_expected = edf_signal_get_num_records(signal);

    /* Make sure that every signal has an equal number of records.*/
    for (gsize n = 1; n < priv->signals->len; n++) {
        gint num_records;
        signal = g_ptr_array_index(priv->signals, n);
        num_records = edf_signal_get_num_records(signal);
        if (num_records != num_records_expected) {
            g_set_error(
                    error, EDF_SIGNAL_ERROR, EDF_SIGNAL_ERROR_FAILED,
                    "The number of records in the signals is out of sync"
                    );
            return;
        }
    }

    /* Write every record to disk. */
    for (gint nrec = 0; nrec < num_records_expected; nrec++) {
        for (gsize nsig = 0; nsig < priv->signals->len; nsig++) {
            signal = g_ptr_array_index(priv->signals, nsig);
            edf_signal_write_record_to_ostream(
                    signal,
                    ostream,
                    nrec,
                    error
                    );
            if (*error)
                return;
        }
    }
}

static void
edf_file_write_to_output_stream(
        EdfFile        *file,
        GOutputStream  *ostream,
        GError        **error
        )
{
    EdfFilePrivate* priv = edf_file_get_instance_private(file);

    edf_header_write_to_ostream(priv->header, ostream, error);
    if (*error)
        return;

    if (priv->signals->len == 0)
        return;

    edf_file_write_records_to_ostream(file, ostream, error);
}

static void
edf_file_set_property(
    GObject*        object,
    guint32         propid,
    const GValue   *value,
    GParamSpec     *spec
    )
{
    EdfFile* file = EDF_FILE(object);
    EdfHeader* private = edf_file_get_instance_private(file);
    (void) private;

    switch((EdfFileProperties) propid) {
        case PROP_FILENAME:
            edf_file_set_path(file, g_value_get_string(value));
            break;
        case PROP_SIGNALS:
            edf_file_set_signals(file, g_value_get_boxed (value));
            break;
        case PROP_HEADER: // Read only
        case PROP_NUM_SIGNALS:
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, spec);
    }
}

static void
edf_file_get_property(
    GObject        *object,
    guint32         propid,
    GValue         *value,
    GParamSpec     *spec
    )
{
    EdfFile* file = EDF_FILE(object);
    EdfFilePrivate* priv = edf_file_get_instance_private(file);

    switch((EdfFileProperties) propid) {
        case PROP_FILENAME:
            g_value_take_string(value, edf_file_get_path(file));
            break;
        case PROP_HEADER:
            g_value_set_object(value, priv->header);
            break;
        case PROP_SIGNALS:
            g_value_set_boxed(value, priv->signals);
            break;
        case PROP_NUM_SIGNALS:
            g_value_set_uint(value, priv->signals->len);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propid, spec);
    }
}

static GParamSpec* edf_file_properties[N_PROPS] = {NULL,};

static void
edf_file_class_init(EdfFileClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = edf_file_set_property;
    object_class->get_property = edf_file_get_property;

    object_class->dispose  = edf_file_dispose;
    object_class->finalize = edf_file_finalize;

    /**
     * EdfFile:path:
     *
     * The path proeprty. This should be set to the name
     * of the out-/input file of this file. This should be done
     * prior to reading from or writing to the file. A in or
     * output stream is opened on the basis of this path.
     */
    edf_file_properties[PROP_FILENAME] = g_param_spec_string(
            "path",
            "file-name",
            "The name of the file to write/read to/from.",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    /**
     * EdfFile:header:
     *
     * Every file contains a header it changes on basis of the
     * number and type of signals that are embedded in the file.
     */
    edf_file_properties [PROP_HEADER] = g_param_spec_object(
            "header",
            "Header",
            "The properties contained in the header",
            EDF_TYPE_HEADER,
            G_PARAM_READABLE
            );

    /**
     * EdfFile:signals:
     *
     * Every file contains (hopefully) some signals that contain
     * the actual data of one transducer. The signals document
     * what type of data is gathered.
     */
    edf_file_properties [PROP_SIGNALS] = g_param_spec_boxed(
        "signals",
        "Signals",
        "The signals contained in the file",
        G_TYPE_PTR_ARRAY,
        G_PARAM_READWRITE
    );

    /**
     * OdfFile:num_signals:
     *
     * This property allows for looping over the number of signals.
     * It boils down to the length of the array containing the 
     * #EdfSignal's.
     */
    edf_file_properties[PROP_NUM_SIGNALS] = g_param_spec_uint(
        "num-signals",
        "Number of signals",
        "The number of signals contained in the file",
        0,
        G_MAXUINT,
        0,
        G_PARAM_READABLE
    );


    g_object_class_install_properties(
            object_class, N_PROPS, edf_file_properties
            );
}

/**
 * edf_file_new:(constructor)
 *
 * Returns a new EdfFile.
 */
EdfFile*
edf_file_new()
{
    EdfFile* file = g_object_new(
            EDF_TYPE_FILE,
            NULL
            );

    return file;
}

/**
 * edf_file_new_for_path:(constructor)
 * @path :(in): the path for the file in utf8
 *
 * Create a new edf file 
 *
 * Returns: (transfer full): a freshly created file
 */
EdfFile*
edf_file_new_for_path(const gchar* path)
{
    g_return_val_if_fail(path != NULL, NULL);

    EdfFile* file = g_object_new(
            EDF_TYPE_FILE,
            "path", path,
            NULL
            );

    return file;
}


/**
 * edf_file_destroy:destructor
 * @file:(in): the EdfFile to destroy
 *
 * Decrements the ref count on the file and the file
 * will be destroyed when the count drops to zero.
 * This is equal to calling g_object_unref(G_OBJECT(file));
 */
void
edf_file_destroy(EdfFile* file)
{
    g_object_unref(file);
}

/**
 * edf_file_get_path:
 * @file : (in) : the EdfFile whose name we would like to know
 *
 * Returns :(transfer full): the path of where to open/store
 *                           the contents. Returns NULL when not applicable
 */
char*
edf_file_get_path(EdfFile* file)
{
    g_return_val_if_fail(EDF_IS_FILE(file), NULL);
    EdfFilePrivate* priv =  edf_file_get_instance_private(file);
    
    return g_file_get_path(priv->file);
}

/**
 * edf_file_set_path:
 * @file:(in): The file whose name we want to alter
 * @path:(in): The file path to a name where we can open
 *             a file for reading or writing
 */
void
edf_file_set_path(EdfFile* file, const char* path)
{
    g_return_if_fail(file != NULL);
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(path != NULL);
    
    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    GFile* dup = g_file_new_for_path(path);
    g_return_if_fail(dup);
    g_object_unref(priv->file);
    priv->file = dup;
}


/**
 * edf_file_read:
 * @self the EdfFile
 * @error:(out): If an error occurs it is returned here.
 *
 * Opens the file for reading. The property fn should be
 * set to a path of a valid edf file. Otherwise havoc will
 * occur.
 */
gsize
edf_file_read(EdfFile* file, GError** error)
{
    gsize num_bytes_tot = 0, nread;
    guint num_signals;
    gint num_records;
    EdfFilePrivate *priv;

    g_return_val_if_fail(EDF_IS_FILE(file), 0);
    g_return_val_if_fail(error != NULL && *error == NULL, 0);

    priv = edf_file_get_instance_private(file);

    GFileInputStream *ifstream = g_file_read(
            priv->file,
            NULL,
            error
            );
    if(*error)
        goto fail;
    GInputStream *istream = G_INPUT_STREAM(ifstream);

    nread = edf_header_read_from_input_stream (
            priv->header,
            G_INPUT_STREAM(istream),
            error
            );
    num_bytes_tot += nread;
    if (*error)
        goto fail;

    g_object_get(
        priv->header,
        "num-data-records", &num_records,
        "num-signals", &num_signals,
        NULL
    );

    for (gint rec = 0; rec < num_records; rec++) {
        for (gsize signal = 0; signal < num_signals; signal++) {
            EdfSignal* sig = g_ptr_array_index(priv->signals, signal);
            nread = edf_signal_read_record_from_istream(sig, istream, error);
            num_bytes_tot += nread;
            if (*error)
                goto fail;
        }
    }
fail:
    g_object_unref(ifstream);
    return num_bytes_tot;
}

/**
 * edf_file_create:
 * @error:(out): An error will be returned here when the
 *               file cannot be written or it exists.
 *
 * Write all the signals and header to disc. This method
 * will fail if the file already exists or another reason
 * for being unable to read the file.
 */
void
edf_file_create(EdfFile* file, GError**error)
{
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(error != NULL && *error == NULL);

    EdfFilePrivate* priv = edf_file_get_instance_private(file);

    GFileOutputStream* outstream = g_file_create(
            priv->file,
            G_FILE_CREATE_NONE,
            NULL,
            error
            );
    if (!outstream) {
        g_assert(*error);
        return;
    }

    edf_file_write_to_output_stream(file, G_OUTPUT_STREAM(outstream), error);
    g_object_unref(outstream);
}

/**
 * edf_file_replace:
 * @error:(out): An error will be returned here when the
 *               file cannot be written
 *
 * The edf file will be written to the currently specified path.
 * If the file exists, it will be overwritten.
 */
void
edf_file_replace(EdfFile* file, GError**error)
{
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(error != NULL && *error == NULL);

    EdfFilePrivate* priv = edf_file_get_instance_private(file);

    GFileOutputStream* outstream = g_file_replace(
            priv->file,
            NULL,
            TRUE,
            G_FILE_CREATE_NONE,
            NULL,
            error
            );
    if (!outstream) {
        g_assert(*error);
        return;
    }

    edf_file_write_to_output_stream(file, G_OUTPUT_STREAM(outstream), error);
    g_object_unref(outstream);
}

/**
 * edf_file_add_signal:
 * @self:The #EdfFile to which you would like to add a signal
 * @signal:(in)(transfer full): The #EdfSignal you would like to add
 *
 * The file will add the signal to its collection of signals, the
 * reference count of the signal is incremented. So you can release
 * your reference to the signals.
 */
void
edf_file_add_signal(EdfFile* file, EdfSignal* signal)
{
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfFilePrivate* priv = edf_file_get_instance_private(file);

    g_ptr_array_add(priv->signals, g_object_ref(signal));
}

/**
 * edf_file_set_signals:
 * @self:(in) : the #EdfFile to which to set the signals
 * @signals:(in)(element-type EdfSignal)(transfer full):
 */
void
edf_file_set_signals(EdfFile* file, GPtrArray* signals) {
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(signals != NULL);

    g_ptr_array_ref(signals);

    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    if (priv->signals)
        g_ptr_array_unref(priv->signals);

    edf_header_set_signals(priv->header, signals);

    priv->signals = signals;    
}

/**
 * edf_file_get_signals:
 * @file:: The from which to obtain the signals.
 *
 * Returns:(transfer none)(element-type EdfSignal*)
 */
GPtrArray*
edf_file_get_signals(EdfFile* file)
{
    g_return_val_if_fail(EDF_IS_FILE(file), NULL);
    EdfFilePrivate* priv =  edf_file_get_instance_private(file);
    return priv->signals;
}

/**
 * edf_file_get_num_signals:
 * @file: the #EdfFile whose signals you would like to obtains
 *
 * Returns: the number of signals or G_MAXUINT incase of error
 */
guint
edf_file_get_num_signals(EdfFile* file)
{
    g_return_val_if_fail(EDF_IS_FILE(file), -1);
    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    return priv->signals->len;
}

/**
 * edf_file_header:
 * @file : (in)
 *
 * Obtains the header of the file.
 *
 * Returns:(transfer none): The header of the file.
 */
EdfHeader*
edf_file_header(EdfFile* file)
{
    g_return_val_if_fail(EDF_IS_FILE(file), NULL);
    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    return priv->header;
}

