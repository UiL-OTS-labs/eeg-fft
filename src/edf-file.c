
#include "edf-file.h"
#include "edf-header.h"
#include "edf-signal.h"
#include <gio/gio.h>

/**
 * SECTION:edf-file
 * @short_description: an object that reads/writes data in European Data Format
 * @see_also: #EdfSignal, #EdfHeader
 *
 * #EdfFile is a file that contains a #EdfHeader and a number of #EdfSignals
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
file_set_filename(EdfFile* file, const gchar* name)
{
    EdfFilePrivate* priv = edf_file_get_instance_private(file);
    GFile* dup = g_file_new_for_path(name);
    g_return_if_fail(dup);
    g_object_unref(priv->file);
    priv->file = dup;
}

static void
file_set_signals(EdfFile* file, GPtrArray* signals)
{
    EdfFilePrivate *priv = edf_file_get_instance_private (file);
    g_ptr_array_set_size (priv->signals, 0);
    for (gsize i = 0; i < signals->len; i++) {
        EdfSignal *sig = g_ptr_array_index(signals, i);
        g_ptr_array_add (priv->signals, sig);
    }
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
            file_set_filename(file, g_value_get_string(value));
            break;
        case PROP_SIGNALS:
            file_set_signals(file, g_value_get_boxed (value));
            break;
        case PROP_HEADER:
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
            {
                char* path = g_file_get_path(priv->file);
                g_value_set_string (value, path);
                g_free(path);
            }
            break;
        case PROP_HEADER:
            g_value_set_object(value, priv->header);
            break;
        case PROP_SIGNALS:
            g_value_set_boxed(value, priv->signals);
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

    edf_file_properties[PROP_FILENAME] = g_param_spec_string(
            "fn",
            "file-name",
            "The name of the file to write/read to/from.",
            "",
            G_PARAM_READWRITE | G_PARAM_CONSTRUCT
            );

    edf_file_properties [PROP_HEADER] = g_param_spec_object(
            "header",
            "Header",
            "The properties contained in the header",
            EDF_TYPE_HEADER,
            G_PARAM_READABLE
            );

    /**
     * signals:
     */
    edf_file_properties [PROP_SIGNALS] = g_param_spec_boxed(
        "signals",
        "Signals",
        "The signals contained in the file",
        G_TYPE_PTR_ARRAY,
        G_PARAM_READWRITE
    );

    g_object_class_install_properties(
            object_class, N_PROPS, edf_file_properties
            );
}

EdfFile*
edf_file_new()
{
    EdfFile* file = g_object_new(
            EDF_TYPE_FILE,
            NULL
            );

    return file;
}

EdfFile*
edf_file_new_for_path(const gchar* path)
{
    g_return_val_if_fail(path != NULL, NULL);

    EdfFile* file = g_object_new(
            EDF_TYPE_FILE,
            "fn", path,
            NULL
            );

    return file;
}

void
edf_file_destroy(EdfFile* file)
{
    g_object_unref(file);
}

void
edf_file_set_path(EdfFile* file, const gchar* fname)
{
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(file != NULL);
    g_object_set(
            file,
            "fn", fname,
            NULL
            );
}

gchar*
edf_file_get_path(EdfFile* file)
{
    gchar* ret = NULL;
    g_return_val_if_fail(EDF_IS_FILE(file), NULL);
    g_object_get(
            file,
            "fn", &ret,
            NULL
            );
    return ret;
}

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

void
edf_file_add_signal(EdfFile* file, EdfSignal* signal)
{
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(EDF_IS_SIGNAL(signal));

    EdfFilePrivate* priv = edf_file_get_instance_private(file);

    g_ptr_array_add(priv->signals, g_object_ref(signal));
}

EdfHeader*
edf_file_header(EdfFile* file)
{
    g_return_val_if_fail(EDF_IS_FILE(file), NULL);
    EdfHeader* hdr;
    g_object_get(file, "header", &hdr, NULL);
    return hdr;
}

void
edf_file_set_signals(EdfFile* file, GPtrArray* signals) {
    g_return_if_fail(EDF_IS_FILE(file));
    g_return_if_fail(signals);

    g_object_set(file, "signals", signals, NULL);
}

GPtrArray*
edf_file_get_sigals(EdfFile* file)
{
    g_return_val_if_fail(EDF_IS_FILE(file), NULL);
    GPtrArray * array = NULL;
    g_object_get (file, "signals", &array, NULL);
    return array;
}
