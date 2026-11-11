#include <Python.h>
#include <stdint.h>

#include "cpython_port.h"
#include "../../../../ext4.h"

#define MAIOS_CPYTHON_FILE_LIMIT (64u * 1024u)
#define MAIOS_CPYTHON_NAME_LIMIT 63u
#define MAIOS_CPYTHON_LINE_LIMIT 1024u

extern int maios_python_read_char(void);
extern void maios_python_write_char(char value);

typedef struct {
    PyObject_HEAD
    int input;
} maios_stream_t;

static int initialized;
static PyObject *maios_stream_type;
static PyObject *maios_stdin;
static PyObject *maios_stream_write(PyObject *self, PyObject *value);

static PyObject *maios_builtin_input(PyObject *self, PyObject *args)
{
    PyObject *prompt = NULL;
    PyObject *line;
    Py_ssize_t length;
    (void)self;

    if (!PyArg_ParseTuple(args, "|O:input", &prompt)) {
        return NULL;
    }
    if (prompt != NULL) {
        if (!PyUnicode_Check(prompt)) {
            PyErr_SetString(PyExc_TypeError, "prompt must be a string");
            return NULL;
        }
        PyObject *stdout_stream = PySys_GetObject("stdout");
        if (stdout_stream == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "maiOs stdout is unavailable");
            return NULL;
        }
        PyObject *written = maios_stream_write(stdout_stream, prompt);
        if (written == NULL) {
            return NULL;
        }
        Py_DECREF(written);
    }
    line = PyObject_CallMethod(maios_stdin, "readline", NULL);
    if (line == NULL) {
        return NULL;
    }
    length = PyUnicode_GetLength(line);
    if (length == 0) {
        Py_DECREF(line);
        PyErr_SetNone(PyExc_EOFError);
        return NULL;
    }
    if (PyUnicode_ReadChar(line, length - 1) == '\n') {
        PyObject *result = PyUnicode_Substring(line, 0, length - 1);
        Py_DECREF(line);
        return result;
    }
    return line;
}

static PyMethodDef maios_input_method = {
    "input", maios_builtin_input, METH_VARARGS, "Read a line from the maiOs console."};

static PyObject *maios_stream_write(PyObject *self, PyObject *value)
{
    Py_ssize_t length;
    const char *text;
    (void)self;

    text = PyUnicode_AsUTF8AndSize(value, &length);
    if (text == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < length; ++i) {
        maios_python_write_char(text[i]);
    }
    return PyLong_FromSsize_t(length);
}

static PyObject *maios_stream_flush(PyObject *self, PyObject *unused)
{
    (void)self;
    (void)unused;
    Py_RETURN_NONE;
}

static PyObject *maios_stream_isatty(PyObject *self, PyObject *unused)
{
    (void)self;
    (void)unused;
    Py_RETURN_TRUE;
}

static PyObject *maios_stream_fileno(PyObject *self, PyObject *unused)
{
    maios_stream_t *stream = (maios_stream_t *)self;
    (void)unused;
    return PyLong_FromLong(stream->input ? 0 : 1);
}

static PyObject *maios_stream_readline(PyObject *self, PyObject *args)
{
    maios_stream_t *stream = (maios_stream_t *)self;
    char line[MAIOS_CPYTHON_LINE_LIMIT];
    Py_ssize_t limit = (Py_ssize_t)sizeof(line) - 1;
    Py_ssize_t length = 0;

    if (!stream->input) {
        PyErr_SetString(PyExc_OSError, "stream is not readable");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "|n:readline", &limit)) {
        return NULL;
    }
    if (limit < 0 || limit >= (Py_ssize_t)sizeof(line)) {
        limit = (Py_ssize_t)sizeof(line) - 1;
    }

    while (length < limit) {
        int key = maios_python_read_char();
        if (key == 4 && length == 0) {
            break;
        }
        if (key == 3) {
            PyErr_SetNone(PyExc_KeyboardInterrupt);
            return NULL;
        }
        if (key == '\n' || key == '\r') {
            maios_python_write_char('\n');
            line[length++] = '\n';
            break;
        }
        if ((key == '\b' || key == 127) && length > 0) {
            --length;
            maios_python_write_char('\b');
            continue;
        }
        if (key >= 32 && key < 127) {
            line[length++] = (char)key;
            maios_python_write_char((char)key);
        }
    }
    return PyUnicode_DecodeUTF8(line, length, "replace");
}

static PyMethodDef maios_stream_methods[] = {
    {"write", maios_stream_write, METH_O, "Write text to the maiOs console."},
    {"flush", maios_stream_flush, METH_NOARGS, "Flush the maiOs console."},
    {"readline", maios_stream_readline, METH_VARARGS, "Read one line from the maiOs console."},
    {"isatty", maios_stream_isatty, METH_NOARGS, "Return whether this is a console."},
    {"fileno", maios_stream_fileno, METH_NOARGS, "Return the console stream number."},
    {NULL, NULL, 0, NULL},
};

static PyType_Slot maios_stream_slots[] = {
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_methods, maios_stream_methods},
    {0, NULL},
};

static PyType_Spec maios_stream_spec = {
    .name = "maios.ConsoleStream",
    .basicsize = sizeof(maios_stream_t),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = maios_stream_slots,
};

static int maios_set_stream(const char *name, const char *dunder_name, int input)
{
    PyObject *stream = PyObject_CallNoArgs(maios_stream_type);
    if (stream == NULL) {
        return -1;
    }
    ((maios_stream_t *)stream)->input = input;
    if (PySys_SetObject(name, stream) < 0 || PySys_SetObject(dunder_name, stream) < 0) {
        Py_DECREF(stream);
        return -1;
    }
    if (input) {
        maios_stdin = stream;
    } else {
        Py_DECREF(stream);
    }
    return 0;
}

static int maios_install_console(void)
{
    PyObject *input_function;
    maios_stream_type = PyType_FromSpec(&maios_stream_spec);
    if (maios_stream_type == NULL) {
        return -1;
    }
    if (maios_set_stream("stdin", "__stdin__", 1) < 0 ||
        maios_set_stream("stdout", "__stdout__", 0) < 0 ||
        maios_set_stream("stderr", "__stderr__", 0) < 0) {
        Py_CLEAR(maios_stdin);
        Py_CLEAR(maios_stream_type);
        return -1;
    }
    input_function = PyCFunction_New(&maios_input_method, NULL);
    if (input_function == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(PyEval_GetBuiltins(), "input", input_function) < 0) {
        Py_DECREF(input_function);
        return -1;
    }
    Py_DECREF(input_function);
    return 0;
}

static int maios_valid_name(const char *name)
{
    size_t length = 0;
    while (name[length]) {
        if (name[length] == '/' || name[length] == '\\' || name[length] == ' ' ||
            length >= MAIOS_CPYTHON_NAME_LIMIT) {
            return 0;
        }
        ++length;
    }
    return length != 0 && !(length == 1 && name[0] == '.') &&
           !(length == 2 && name[0] == '.' && name[1] == '.');
}

static PyObject *maios_main_globals(void)
{
    PyObject *main_module = PyImport_AddModule("__main__");
    if (main_module == NULL) {
        return NULL;
    }
    return PyModule_GetDict(main_module);
}

static void maios_write_text(const char *text)
{
    while (*text) maios_python_write_char(*text++);
}

static void maios_write_status(const char *stage, const PyStatus *status)
{
    const char *text;
    maios_write_text(stage);
    maios_python_write_char(':');
    maios_python_write_char(' ');
    if (status != NULL && status->func != NULL) {
        maios_write_text(status->func);
        maios_python_write_char(':');
        maios_python_write_char(' ');
    }
    if (status != NULL && status->err_msg != NULL) {
        maios_write_text(status->err_msg);
    } else {
        maios_write_text("failed without a CPython error message");
    }
    maios_python_write_char('\n');
}

int maios_cpython_initialize(void)
{
    PyConfig config;
    PyStatus status;

    if (initialized) {
        return 0;
    }

    PyConfig_InitIsolatedConfig(&config);
    config.install_signal_handlers = 0;
    config.use_environment = 0;
    config.site_import = 0;
    config.parse_argv = 0;
    config.write_bytecode = 0;

    status = PyConfig_SetBytesString(&config, &config.program_name, "python");
    if (!PyStatus_Exception(status)) {
        status = Py_InitializeFromConfig(&config);
    }
    if (PyStatus_Exception(status)) {
        maios_write_status("CPython initialization", &status);
        PyConfig_Clear(&config);
        return -1;
    }
    PyConfig_Clear(&config);

    initialized = 1;
    if (maios_install_console() < 0) {
        maios_write_text("CPython console setup failed\n");
        PyErr_Print();
        (void)Py_FinalizeEx();
        initialized = 0;
        return -1;
    }
    return 0;
}

int maios_cpython_run(const char *source)
{
    if (!initialized || source == NULL) {
        return -1;
    }
    return PyRun_SimpleStringFlags(source, NULL);
}

int maios_cpython_run_file(const char *filename)
{
    uint32_t size;
    char *source;
    PyObject *globals;
    PyObject *file_name;
    PyObject *result;

    if (!initialized || filename == NULL || !maios_valid_name(filename)) {
        return -1;
    }
    if (!ext4_read_start(filename, &size)) {
        return -1;
    }
    if (size > MAIOS_CPYTHON_FILE_LIMIT) {
        ext4_read_end();
        return -1;
    }
    source = PyMem_Malloc((size_t)size + 1);
    if (source == NULL) {
        ext4_read_end();
        return -1;
    }
    for (uint32_t offset = 0, block = 0; offset < size; ++block) {
        uint8_t chunk[1024];
        uint32_t count = size - offset;
        if (count > sizeof(chunk)) {
            count = sizeof(chunk);
        }
        if (!ext4_read_block(block, chunk)) {
            ext4_read_end();
            PyMem_Free(source);
            return -1;
        }
        for (uint32_t i = 0; i < count; ++i) {
            source[offset + i] = (char)chunk[i];
        }
        offset += count;
    }
    ext4_read_end();
    source[size] = '\0';

    globals = maios_main_globals();
    file_name = PyUnicode_DecodeFSDefault(filename);
    if (globals == NULL || file_name == NULL ||
        PyDict_SetItemString(globals, "__file__", file_name) < 0) {
        Py_XDECREF(file_name);
        PyMem_Free(source);
        PyErr_Print();
        return -1;
    }
    result = PyRun_StringFlags(source, Py_file_input, globals, globals, NULL);
    Py_DECREF(file_name);
    PyMem_Free(source);
    if (result == NULL) {
        PyErr_Print();
        return -1;
    }
    Py_DECREF(result);
    return 0;
}

int maios_cpython_repl(void)
{
    PyObject *globals;
    if (!initialized || maios_stdin == NULL) {
        return -1;
    }
    globals = maios_main_globals();
    if (globals == NULL) {
        PyErr_Print();
        return -1;
    }

    for (;;) {
        PyObject *line;
        const char *source;
        PyObject *result;
        maios_python_write_char('>');
        maios_python_write_char('>');
        maios_python_write_char('>');
        maios_python_write_char(' ');
        line = PyObject_CallMethod(maios_stdin, "readline", NULL);
        if (line == NULL) {
            PyErr_Print();
            return -1;
        }
        if (PyUnicode_GetLength(line) == 0) {
            Py_DECREF(line);
            return 0;
        }
        source = PyUnicode_AsUTF8(line);
        if (source == NULL) {
            Py_DECREF(line);
            PyErr_Print();
            return -1;
        }
        result = PyRun_StringFlags(source, Py_single_input, globals, globals, NULL);
        Py_DECREF(line);
        if (result == NULL) {
            PyErr_Print();
        } else {
            Py_DECREF(result);
        }
    }
}

int maios_cpython_finalize(void)
{
    int result;

    if (!initialized) {
        return 0;
    }

    Py_CLEAR(maios_stdin);
    Py_CLEAR(maios_stream_type);
    result = Py_FinalizeEx();
    initialized = 0;
    return result < 0 ? -1 : 0;
}
