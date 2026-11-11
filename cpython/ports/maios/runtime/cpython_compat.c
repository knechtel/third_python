#include <Python.h>
#include <signal.h>

/* maiOs currently has no signal delivery or fault-handler support. */
int _PySignal_Init(int install_signal_handlers)
{
    (void)install_signal_handlers;
    return 0;
}

void _PySignal_Fini(void)
{
}

int PyErr_CheckSignals(void)
{
    return 0;
}

int PyErr_SetInterruptEx(int signum)
{
    if (signum < 1 || signum >= NSIG) return -1;
    return 0;
}

int _PyErr_CheckSignalsTstate(PyThreadState *thread_state)
{
    (void)thread_state;
    return 0;
}

int _PyOS_InterruptOccurred(PyThreadState *thread_state)
{
    (void)thread_state;
    return 0;
}

PyStatus _PyFaulthandler_Init(int enable)
{
    (void)enable;
    return PyStatus_Ok();
}

void _PyFaulthandler_Fini(void)
{
}

PyObject *PyOS_FSPath(PyObject *path)
{
    PyObject *result;

    if (PyUnicode_Check(path) || PyBytes_Check(path)) {
        Py_INCREF(path);
        return path;
    }

    result = PyObject_CallMethod(path, "__fspath__", NULL);
    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Format(PyExc_TypeError,
                         "expected str, bytes or os.PathLike object, not %.200s",
                         Py_TYPE(path)->tp_name);
        }
        return NULL;
    }
    if (!PyUnicode_Check(result) && !PyBytes_Check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "expected %.200s.__fspath__() to return str or bytes, not %.200s",
                     Py_TYPE(path)->tp_name, Py_TYPE(result)->tp_name);
        Py_DECREF(result);
        return NULL;
    }
    return result;
}
