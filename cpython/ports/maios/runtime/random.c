#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/random.h>

static int rdrand_available = -1;

static int has_rdrand(void)
{
    uint32_t eax, ebx, ecx, edx;
    if (rdrand_available >= 0) return rdrand_available;

    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0), "c"(0));
    if (eax < 1) {
        rdrand_available = 0;
        return 0;
    }
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1), "c"(0));
    rdrand_available = (ecx & (1u << 30)) != 0;
    return rdrand_available;
}

static int rdrand32(uint32_t *value)
{
    unsigned char ready;
    for (int attempt = 0; attempt < 10; ++attempt) {
        __asm__ volatile("rdrand %0; setc %1"
                         : "=r"(*value), "=qm"(ready));
        if (ready) return 1;
    }
    return 0;
}

ssize_t getrandom(void *buffer, size_t length, unsigned int flags)
{
    (void)flags;
    if (length == 0) return 0;
    if (buffer == NULL) { errno = EFAULT; return -1; }
    if (!has_rdrand()) { errno = ENOSYS; return -1; }

    uint8_t *out = buffer;
    size_t done = 0;
    while (done < length) {
        uint32_t value;
        if (!rdrand32(&value)) {
            if (done != 0) return (ssize_t)done;
            errno = EIO;
            return -1;
        }
        size_t chunk = length - done;
        if (chunk > sizeof(value)) chunk = sizeof(value);
        const uint8_t *bytes = (const uint8_t *)&value;
        for (size_t i = 0; i < chunk; ++i) out[done + i] = bytes[i];
        done += chunk;
    }
    return (ssize_t)done;
}
