#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include <string.h>

#include "../../../../net.h"

#define HTTP_REQUEST_MAX 2048u
#define HTTP_BODY_MAX 16384u
#define TCP_DATA_MAX 1460u
#define SESSION_COUNT 16u
#define SESSION_COOKIE "maios_session"

typedef struct {
    char id[33];
    PyObject *data;
} web_session_t;

static web_session_t sessions[SESSION_COUNT];
static unsigned next_session;
static PyObject *active_session;

static int secure_session_id(char id[33])
{
    uint32_t eax=1,ebx,ecx,edx;
    __asm__ volatile("cpuid":"+a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx));
    (void)ebx;(void)edx;
    if (!(ecx & (1u<<30))) return 0;
    static const char hex[]="0123456789abcdef";
    for (unsigned i=0;i<4;i++) {
        uint32_t word=0; uint8_t ok=0;
        for (unsigned attempt=0;attempt<10&&!ok;attempt++)
            __asm__ volatile(".byte 0x0f,0xc7,0xf0; setc %1":"=a"(word),"=m"(ok));
        if (!ok) return 0;
        for (unsigned j=0;j<8;j++) id[i*8+j]=hex[(word >> ((7-j)*4)) & 15];
    }
    id[32]='\0'; return 1;
}

static int cookie_session_id(const char *headers, size_t length, char id[33])
{
    const char *line=strstr(headers,"\r\n");
    while (line && (size_t)(line-headers)<length) {
        line+=2;
        if (line[0]=='\r' && line[1]=='\n') break;
        const char *end=strstr(line,"\r\n"); if (!end) break;
        if ((size_t)(end-line)>=7 && (line[0]=='C'||line[0]=='c') &&
            (line[1]=='O'||line[1]=='o') && (line[2]=='O'||line[2]=='o') &&
            (line[3]=='K'||line[3]=='k') && (line[4]=='I'||line[4]=='i') &&
            (line[5]=='E'||line[5]=='e') && line[6]==':') {
            const char *p=line+7;
            while (p<end) {
                while (p<end && (*p==' '||*p=='\t'||*p==';')) p++;
                const char *name=p; while (p<end && *p!='=' && *p!=';') p++;
                if (p<end && *p=='=') {
                    size_t n=(size_t)(p-name); p++;
                    const char *value=p; while (p<end && *p!=';') p++;
                    if (n==sizeof(SESSION_COOKIE)-1 && !memcmp(name,SESSION_COOKIE,n) &&
                        p-value==32) {
                        int valid=1;
                        for (unsigned i=0;i<32;i++) if (!((value[i]>='0'&&value[i]<='9')||(value[i]>='a'&&value[i]<='f'))) valid=0;
                        if (valid) { memcpy(id,value,32); id[32]='\0'; return 1; }
                    }
                }
            }
        }
        line=end;
    }
    return 0;
}

static PyObject *get_session_data(const char *headers, size_t length, char id[33], int *fresh)
{
    *fresh=1;
    if (cookie_session_id(headers,length,id)) {
        for (unsigned i=0;i<SESSION_COUNT;i++) if (sessions[i].data && !strcmp(sessions[i].id,id)) {
            *fresh=0; return sessions[i].data;
        }
    }
    if (!secure_session_id(id)) return NULL;
    unsigned slot=next_session++ % SESSION_COUNT;
    Py_XDECREF(sessions[slot].data);
    sessions[slot].data=PyDict_New();
    if (!sessions[slot].data) return NULL;
    memcpy(sessions[slot].id,id,33);
    return sessions[slot].data;
}

static int send_all(const uint8_t *data, size_t length)
{
    while (length != 0) {
        uint16_t chunk = length > TCP_DATA_MAX ? TCP_DATA_MAX : (uint16_t)length;
        if (!net_tcp_send(data, chunk)) return 0;
        data += chunk;
        length -= chunk;
    }
    return 1;
}

static void send_error(unsigned status, const char *reason)
{
    char header[128];
    char digits[10];
    size_t used = 0, count = 0;
    const char *prefix = "HTTP/1.1 ";
    const char *middle = " ";
    const char *suffix = "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    while (*prefix) header[used++] = *prefix++;
    do { digits[count++] = (char)('0' + status % 10); status /= 10; } while (status);
    while (count) header[used++] = digits[--count];
    while (*middle) header[used++] = *middle++;
    while (*reason) header[used++] = *reason++;
    for (size_t i = 0; suffix[i] && used < sizeof(header); ++i) header[used++] = suffix[i];
    (void)send_all((const uint8_t *)header, used);
}

static int send_response(PyObject *response, const char *session_id, int fresh_session)
{
    const char *body;
    Py_ssize_t body_length;
    if (PyUnicode_Check(response)) {
        body = PyUnicode_AsUTF8AndSize(response, &body_length);
        if (body == NULL) return 0;
    } else if (PyBytes_Check(response)) {
        if (PyBytes_AsStringAndSize(response, (char **)&body, &body_length) < 0) return 0;
    } else {
        PyErr_SetString(PyExc_TypeError, "web handler must return str or bytes");
        return 0;
    }
    if (body_length > (Py_ssize_t)HTTP_BODY_MAX) body_length = HTTP_BODY_MAX;

    char header[256];
    size_t used = 0, count = 0;
    const char *prefix = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: ";
    const char *suffix = "\r\nConnection: close\r\n\r\n";
    while (*prefix) header[used++] = *prefix++;
    size_t length = (size_t)body_length;
    do { header[used + count++] = (char)('0' + length % 10); length /= 10; } while (length);
    for (size_t i = 0; i < count / 2; ++i) {
        char temp = header[used + i];
        header[used + i] = header[used + count - i - 1];
        header[used + count - i - 1] = temp;
    }
    used += count;
    if (fresh_session) {
        const char *cookie="\r\nSet-Cookie: " SESSION_COOKIE "=";
        for (size_t i=0;cookie[i];i++) header[used++]=cookie[i];
        for (unsigned i=0;i<32;i++) header[used++]=session_id[i];
        const char *attrs="; Path=/; HttpOnly; SameSite=Lax";
        for (size_t i=0;attrs[i];i++) header[used++]=attrs[i];
    }
    for (size_t i = 0; suffix[i]; ++i) header[used++] = suffix[i];
    return send_all((const uint8_t *)header, used) &&
           send_all((const uint8_t *)body, (size_t)body_length);
}

static PyObject *webserver_session(PyObject *self, PyObject *args)
{
    (void)self; (void)args;
    if (!active_session) {
        PyErr_SetString(PyExc_RuntimeError,"session() is only available while handling a request");
        return NULL;
    }
    Py_INCREF(active_session); return active_session;
}

static int parse_content_length(const char *headers, size_t header_length, size_t *body_length)
{
    const char *line = strstr(headers, "\r\n");
    *body_length = 0;
    while (line != NULL && (size_t)(line - headers) < header_length) {
        line += 2;
        if (line[0] == '\r' && line[1] == '\n') return 1;
        const char *end = strstr(line, "\r\n");
        if (end == NULL || (size_t)(end - headers) > header_length) return 0;
        if ((size_t)(end - line) >= 15 &&
            (line[0] == 'C' || line[0] == 'c') &&
            (line[1] == 'O' || line[1] == 'o') &&
            (line[2] == 'N' || line[2] == 'n') &&
            (line[3] == 'T' || line[3] == 't') &&
            (line[4] == 'E' || line[4] == 'e') &&
            (line[5] == 'N' || line[5] == 'n') &&
            (line[6] == 'T' || line[6] == 't') && line[7] == '-' &&
            (line[8] == 'L' || line[8] == 'l') &&
            (line[9] == 'E' || line[9] == 'e') &&
            (line[10] == 'N' || line[10] == 'n') &&
            (line[11] == 'G' || line[11] == 'g') &&
            (line[12] == 'T' || line[12] == 't') &&
            (line[13] == 'H' || line[13] == 'h') && line[14] == ':') {
            const char *value = line + 15;
            while (value < end && (*value == ' ' || *value == '\t')) ++value;
            if (value == end) return 0;
            size_t length = 0;
            while (value < end && *value >= '0' && *value <= '9') {
                unsigned digit = (unsigned)(*value++ - '0');
                if (length > (HTTP_BODY_MAX - digit) / 10) return 0;
                length = length * 10 + digit;
            }
            while (value < end && (*value == ' ' || *value == '\t')) ++value;
            if (value != end) return 0;
            *body_length = length;
        }
        line = end;
    }
    return 1;
}

static PyObject *webserver_serve(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"handler", "port", NULL};
    PyObject *handler;
    int port = 8070;
    (void)self;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i:serve", keywords, &handler, &port))
        return NULL;
    if (!PyCallable_Check(handler)) {
        PyErr_SetString(PyExc_TypeError, "handler must be callable");
        return NULL;
    }
    if (port < 1 || port > 65535) {
        PyErr_SetString(PyExc_ValueError, "port must be between 1 and 65535");
        return NULL;
    }

    PyObject *stdout = PySys_GetObject("stdout");
    if (stdout != NULL) {
        PyObject *message = PyUnicode_FromFormat("CPython HTTP server listening on port %d\n", port);
        if (message != NULL) {
            PyObject *written = PyObject_CallMethod(stdout, "write", "O", message);
            Py_XDECREF(written);
            Py_DECREF(message);
        }
    }

    for (;;) {
        int accepted=net_tcp_accept((uint16_t)port);
        if(accepted<0){net_clear_interrupt();PyErr_SetNone(PyExc_KeyboardInterrupt);return NULL;}
        if(!accepted)continue;
        char request[HTTP_REQUEST_MAX + HTTP_BODY_MAX + 1];
        size_t used = 0;
        size_t header_length = 0, expected_length = 0;
        int complete = 0, malformed = 0;
        while (used < sizeof(request) - 1 && !complete && !malformed) {
            uint8_t chunk[512];
            uint16_t received = 0;
            int result = net_tcp_receive(chunk, sizeof(chunk), &received);
            if(result==-2){net_clear_interrupt();(void)net_tcp_close();PyErr_SetNone(PyExc_KeyboardInterrupt);return NULL;}
            if (result <= 0) break;
            for (uint16_t i = 0; i < received && used < sizeof(request) - 1; ++i) {
                request[used++] = (char)chunk[i];
                if (header_length == 0 && used >= 4 && request[used - 4] == '\r' && request[used - 3] == '\n' &&
                    request[used - 2] == '\r' && request[used - 1] == '\n') {
                    header_length = used;
                    request[used] = '\0';
                    if (header_length > HTTP_REQUEST_MAX ||
                        !parse_content_length(request, header_length, &expected_length)) malformed = 1;
                    else if (used == header_length + expected_length) complete = 1;
                } else if (header_length != 0 && used == header_length + expected_length) {
                    complete = 1;
                }
            }
            if (header_length != 0 && used >= header_length + expected_length) complete = 1;
        }
        request[used] = '\0';
        if (!complete || malformed || header_length == 0 || used < header_length + expected_length) {
            send_error(400, "Bad Request");
        } else {
            char *line_end = strstr(request, "\r\n");
            char *first_space = strchr(request, ' ');
            char *second_space = first_space ? strchr(first_space + 1, ' ') : NULL;
            if (line_end == NULL || first_space == NULL || second_space == NULL ||
                first_space >= line_end || second_space >= line_end) {
                send_error(400, "Bad Request");
            } else {
                *first_space = '\0';
                *second_space = '\0';
                char session_id[33]; int fresh_session=0;
                PyObject *session=get_session_data(request,header_length,session_id,&fresh_session);
                active_session=session;
                PyObject *body = session ? PyBytes_FromStringAndSize(request + header_length, (Py_ssize_t)expected_length) : NULL;
                PyObject *response = body ? PyObject_CallFunction(handler, "ssO", request, first_space + 1, body) : NULL;
                if (response && !send_response(response,session_id,fresh_session)) {
                    if (PyErr_Occurred()) PyErr_Print();
                    send_error(500,"Internal Server Error");
                }
                active_session=NULL;
                Py_XDECREF(body);
                if (session == NULL) { if (PyErr_Occurred()) PyErr_Print(); send_error(500,"Internal Server Error"); }
                if (response == NULL) {
                    PyErr_Print();
                    send_error(500, "Internal Server Error");
                }
                Py_XDECREF(response);
            }
        }
        (void)net_tcp_close();
    }
    Py_RETURN_NONE;
}

static PyMethodDef webserver_methods[] = {
    {"serve", (PyCFunction)(void(*)(void))webserver_serve, METH_VARARGS | METH_KEYWORDS,
     "serve(handler, port=8070) -> None\nRun a sequential HTTP server using handler(method, path)."},
    {"session", webserver_session, METH_NOARGS,
     "session() -> dict\nReturn the current request's persistent session dictionary."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef webserver_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "webserver",
    .m_doc = "A small sequential HTTP server for maiOs.",
    .m_size = -1,
    .m_methods = webserver_methods,
};

PyMODINIT_FUNC PyInit_webserver(void)
{
    return PyModule_Create(&webserver_module);
}
