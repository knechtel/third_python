#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdint.h>
#include "../../../../ext4.h"

#define MAIOSFS_MAX_FILE_SIZE (64u * 1024u)
#define MAIOSFS_MAX_NAME 63u

static int checked_name(const char *name)
{
    size_t length = 0;

    while (name[length] != '\0') {
        if (name[length] == '/' || name[length] == '\\' || name[length] == ' ' ||
            length >= MAIOSFS_MAX_NAME) {
            return 0;
        }
        ++length;
    }
    return length != 0 && !(length == 1 && name[0] == '.') &&
           !(length == 2 && name[0] == '.' && name[1] == '.');
}

static PyObject *maiosfs_read(PyObject *self, PyObject *args)
{
    const char *name;
    uint32_t size;
    uint8_t *contents;
    PyObject *result;
    (void)self;

    if (!PyArg_ParseTuple(args, "s:read", &name)) {
        return NULL;
    }
    if (!checked_name(name)) {
        PyErr_SetString(PyExc_ValueError, "use a simple filename in the current directory");
        return NULL;
    }
    if (!ext4_read_start(name, &size)) {
        PyErr_SetString(PyExc_OSError, "file not found or unsupported");
        return NULL;
    }
    if (size > MAIOSFS_MAX_FILE_SIZE) {
        ext4_read_end();
        PyErr_SetString(PyExc_OSError, "file exceeds the maiOs 64 KiB limit");
        return NULL;
    }

    contents = PyMem_Malloc(size ? size : 1);
    if (contents == NULL) {
        ext4_read_end();
        return PyErr_NoMemory();
    }
    for (uint32_t offset = 0, block = 0; offset < size; ++block) {
        uint8_t chunk[1024];
        uint32_t count = size - offset;
        if (count > sizeof(chunk)) {
            count = sizeof(chunk);
        }
        if (!ext4_read_block(block, chunk)) {
            ext4_read_end();
            PyMem_Free(contents);
            PyErr_SetString(PyExc_OSError, "ext4 read failed");
            return NULL;
        }
        for (uint32_t i = 0; i < count; ++i) {
            contents[offset + i] = chunk[i];
        }
        offset += count;
    }
    ext4_read_end();

    result = PyBytes_FromStringAndSize((const char *)contents, (Py_ssize_t)size);
    PyMem_Free(contents);
    return result;
}

static PyObject *maiosfs_write(PyObject *self, PyObject *args)
{
    const char *name;
    Py_buffer data;
    Py_ssize_t size;
    (void)self;

    if (!PyArg_ParseTuple(args, "sy*:write", &name, &data)) {
        return NULL;
    }
    if (!checked_name(name)) {
        PyBuffer_Release(&data);
        PyErr_SetString(PyExc_ValueError, "use a simple filename in the current directory");
        return NULL;
    }
    if (data.len < 0 || (size_t)data.len > MAIOSFS_MAX_FILE_SIZE) {
        PyBuffer_Release(&data);
        PyErr_SetString(PyExc_OSError, "data exceeds the maiOs 64 KiB limit");
        return NULL;
    }
    if (!ext4_write_begin(name)) {
        PyBuffer_Release(&data);
        PyErr_SetString(PyExc_OSError, "ext4 write could not start");
        return NULL;
    }

    size = data.len;
    uint32_t blocks = ((uint32_t)size + 1023u) / 1024u;
    for (uint32_t block = 0; block < blocks; ++block) {
        size_t offset = (size_t)block * 1024u;
        size_t count = (size_t)size - offset;
        if (count > 1024u) {
            count = 1024u;
        }
        if (!ext4_write_block(block, (const uint8_t *)data.buf + offset, count)) {
            (void)ext4_write_finish((uint32_t)offset);
            PyBuffer_Release(&data);
            PyErr_SetString(PyExc_OSError, "ext4 write failed");
            return NULL;
        }
    }
    PyBuffer_Release(&data);
    if (!ext4_write_finish((uint32_t)size)) {
        PyErr_SetString(PyExc_OSError, "ext4 could not finish the write");
        return NULL;
    }
    return PyLong_FromSsize_t(size);
}

static PyObject *maiosfs_remove(PyObject *self, PyObject *args)
{
    const char *name;
    (void)self;

    if (!PyArg_ParseTuple(args, "s:remove", &name)) {
        return NULL;
    }
    if (!checked_name(name)) {
        PyErr_SetString(PyExc_ValueError, "use a simple filename in the current directory");
        return NULL;
    }
    if (!ext4_unlink(name)) {
        PyErr_SetString(PyExc_OSError, "file could not be removed");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef maiosfs_methods[] = {
    {"read", maiosfs_read, METH_VARARGS, "Read a file from the current ext4 directory."},
    {"write", maiosfs_write, METH_VARARGS, "Write bytes to a file in the current ext4 directory."},
    {"remove", maiosfs_remove, METH_VARARGS, "Remove a file from the current ext4 directory."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef maiosfs_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "maiosfs",
    .m_doc = "Access files in the maiOs ext4 filesystem.",
    .m_size = -1,
    .m_methods = maiosfs_methods,
};

PyMODINIT_FUNC PyInit_maiosfs(void)
{
    return PyModule_Create(&maiosfs_module);
}
