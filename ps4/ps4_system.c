#include "core.h"
#include "doomgeneric.h"

extern void *G;
extern void *D;
extern s32 log_fd;
extern u8 log_sa[16];

/* Fake FILE type – DoomGeneric expects FILE * for I/O */
typedef int MY_FILE;

static s32 fd_table[16] = { -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1, -1, -1, -1, -1, -1, -1 };

/* Simple string length */
static int my_strlen(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

/* Error / quit / print */
void I_Error(const char *msg) {
    void *sendto = SYM(G, D, LIBKERNEL_HANDLE, "sendto");
    if (sendto && log_fd >= 0) {
        int len = my_strlen(msg);
        NC(G, sendto, (u64)log_fd, (u64)msg, (u64)len, 0, (u64)log_sa, 16);
    }
    while (1) {}
}

void I_Quit(void) { }

void I_PrintStr(const char *str) {
    void *sendto = SYM(G, D, LIBKERNEL_HANDLE, "sendto");
    if (sendto && log_fd >= 0) {
        int len = my_strlen(str);
        NC(G, sendto, (u64)log_fd, (u64)str, (u64)len, 0, (u64)log_sa, 16);
    }
}

/* File I/O */
MY_FILE *fopen(const char *path, const char *mode) {
    (void)mode;
    void *kopen = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelOpen");
    if (!kopen) return NULL;
    s32 fd = (s32)NC(G, kopen, (u64)path, 0, 0, 0, 0, 0);
    if (fd < 0) return NULL;

    for (int i = 0; i < 16; i++) {
        if (fd_table[i] == -1) {
            fd_table[i] = fd;
            return (MY_FILE *)(u64)(i + 1);
        }
    }
    void *kclose = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelClose");
    if (kclose) NC(G, kclose, (u64)fd, 0, 0, 0, 0, 0);
    return NULL;
}

int fclose(MY_FILE *stream) {
    if (!stream) return -1;
    int idx = (int)((u64)stream - 1);
    if (idx < 0 || idx >= 16 || fd_table[idx] == -1) return -1;
    void *kclose = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelClose");
    if (kclose) NC(G, kclose, (u64)fd_table[idx], 0, 0, 0, 0, 0);
    fd_table[idx] = -1;
    return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, MY_FILE *stream) {
    if (!stream) return 0;
    int idx = (int)((u64)stream - 1);
    if (idx < 0 || idx >= 16 || fd_table[idx] == -1) return 0;
    void *kread = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelRead");
    if (!kread) return 0;
    s64 total = (s64)(size * nmemb);
    s32 n = (s32)NC(G, kread, (u64)fd_table[idx], (u64)ptr, (u64)total, 0, 0, 0);
    return (n > 0) ? (size_t)(n / size) : 0;
}

int fseek(MY_FILE *stream, long offset, int whence) {
    if (!stream) return -1;
    int idx = (int)((u64)stream - 1);
    if (idx < 0 || idx >= 16 || fd_table[idx] == -1) return -1;
    void *klseek = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelLseek");
    if (!klseek) return -1;
    NC(G, klseek, (u64)fd_table[idx], (u64)offset, (u64)whence, 0, 0, 0);
    return 0;
}

void I_Init(void) { }
void I_Shutdown(void) { }
