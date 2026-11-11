#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string.h>

#include "ext4.h"

static PyTypeObject *maios_stat_result_type;

static PyStructSequence_Field maios_stat_fields[] = {
    {"st_mode", "protection bits"},
    {"st_ino", "inode number"},
    {"st_dev", "device"},
    {"st_nlink", "number of hard links"},
    {"st_uid", "user ID of owner"},
    {"st_gid", "group ID of owner"},
    {"st_size", "total size, in bytes"},
    {"st_atime", "time of last access"},
    {"st_mtime", "time of last modification"},
    {"st_ctime", "time of last status change"},
    {NULL, NULL},
};

static PyStructSequence_Desc maios_stat_desc = {
    "posix.stat_result",
    "Result from stat or lstat.",
    maios_stat_fields,
    10,
};

static PyObject *maios_stat(PyObject *self, PyObject *args)
{
    PyObject *path_bytes = NULL;
    const char *path;
    uint32_t size = 0;
    long mode = 0;
    (void)self;

    if (!PyArg_ParseTuple(args, "O&:stat", PyUnicode_FSConverter, &path_bytes)) {
        return NULL;
    }
    path = PyBytes_AS_STRING(path_bytes);
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) {
        mode = 0040000;
    } else if (ext4_read_start(path, &size)) {
        ext4_read_end();
        mode = 0100000;
    } else {
        ext4_entry_t entries[64];
        int count = ext4_list(entries, 64);
        for (int i = 0; i < count && i < 64; ++i) {
            if (strcmp(entries[i].name, path) == 0) {
                mode = entries[i].directory ? 0040000 : 0100000;
                break;
            }
        }
    }
    Py_DECREF(path_bytes);
    if (mode == 0) {
        PyErr_SetString(PyExc_FileNotFoundError, "maiOs path not found");
        return NULL;
    }

    PyObject *result = PyStructSequence_New(maios_stat_result_type);
    if (result == NULL) return NULL;
    long values[10] = {mode, 0, 0, 1, 0, 0, (long)size, 0, 0, 0};
    for (int i = 0; i < 10; ++i) {
        PyObject *value = PyLong_FromLong(values[i]);
        if (value == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyStructSequence_SET_ITEM(result, i, value);
    }
    return result;
}

static PyMethodDef maiosposix_methods[] = {
    {"stat", maios_stat, METH_VARARGS, "Return basic file status for an ext4 path."},
    {"lstat", maios_stat, METH_VARARGS, "Return basic file status for an ext4 path."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef maiosposix_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "posix",
    .m_doc = "Minimal POSIX compatibility for maiOs.",
    .m_size = -1,
    .m_methods = maiosposix_methods,
};

PyMODINIT_FUNC PyInit_posix(void)
{
    PyObject *module = PyModule_Create(&maiosposix_module);
    if (module == NULL) return NULL;
    maios_stat_result_type = PyStructSequence_NewType(&maios_stat_desc);
    if (maios_stat_result_type == NULL ||
        PyModule_AddObject(module, "stat_result", (PyObject *)maios_stat_result_type) < 0) {
        Py_XDECREF((PyObject *)maios_stat_result_type);
        Py_DECREF(module);
        return NULL;
    }
    if (PyModule_AddStringConstant(module, "name", "posix") < 0 ||
        PyModule_AddStringConstant(module, "sep", "/") < 0 ||
        PyModule_AddStringConstant(module, "pathsep", ":") < 0 ||
        PyModule_AddIntConstant(module, "O_RDONLY", 0) < 0 ||
        PyModule_AddIntConstant(module, "O_WRONLY", 1) < 0 ||
        PyModule_AddIntConstant(module, "O_RDWR", 2) < 0 ||
        PyModule_AddIntConstant(module, "O_CREAT", 0100) < 0 ||
        PyModule_AddIntConstant(module, "O_EXCL", 0200) < 0 ||
        PyModule_AddIntConstant(module, "O_TRUNC", 01000) < 0 ||
        PyModule_AddIntConstant(module, "O_APPEND", 02000) < 0) {
        Py_DECREF(module);
        return NULL;
    }
    return module;
}
