#define PY_SSIZE_T_CLEAN
#include <Python.h>

static PyObject *clock_unavailable(PyObject *self, PyObject *args)
{
    (void)self;
    (void)args;
    PyErr_SetString(PyExc_OSError, "maiOs nao possui um relogio disponivel");
    return NULL;
}

static PyMethodDef time_methods[] = {
    {"time", clock_unavailable, METH_VARARGS, "Wall-clock time is unavailable."},
    {"time_ns", clock_unavailable, METH_VARARGS, "Wall-clock time is unavailable."},
    {"localtime", clock_unavailable, METH_VARARGS, "Local time is unavailable."},
    {"gmtime", clock_unavailable, METH_VARARGS, "UTC time is unavailable."},
    {"sleep", clock_unavailable, METH_VARARGS, "Sleeping is unavailable."},
    {"monotonic", clock_unavailable, METH_VARARGS, "Monotonic time is unavailable."},
    {"monotonic_ns", clock_unavailable, METH_VARARGS, "Monotonic time is unavailable."},
    {"perf_counter", clock_unavailable, METH_VARARGS, "Performance clock is unavailable."},
    {"perf_counter_ns", clock_unavailable, METH_VARARGS, "Performance clock is unavailable."},
    {"process_time", clock_unavailable, METH_VARARGS, "Process time is unavailable."},
    {"process_time_ns", clock_unavailable, METH_VARARGS, "Process time is unavailable."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef time_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "time",
    .m_doc = "maiOs time functions; clock access is not implemented yet.",
    .m_size = -1,
    .m_methods = time_methods,
};

PyMODINIT_FUNC PyInit_time(void)
{
    return PyModule_Create(&time_module);
}
