#include "core.h"
#include "doomgeneric.h"

extern void *G;
extern void *D;
extern s32 log_fd;
extern u8 log_sa[16];
extern void *usleep_fn;

// ------------------------------------------------------------
//  Error / quit / print
// ------------------------------------------------------------
void I_Error(const char *msg) {
    // Send to UDP log and halt
    void *sendto = SYM(G, D, LIBKERNEL_HANDLE, "sendto");
    if (sendto && log_fd >= 0) {
        NC(G, sendto, (u64)log_fd, (u64)msg, (u64)str_len(msg), 0, (u64)log_sa, 16);
    }
    while (1) { /* hang */ }
}

void I_Quit(void) {
    // Will break the main loop eventually
}

void I_PrintStr(const char *str) {
    void *sendto = SYM(G, D, LIBKERNEL_HANDLE, "sendto");
    if (sendto && log_fd >= 0) {
        NC(G, sendto, (u64)log_fd, (u64)str, (u64)str_len(str), 0, (u64)log_sa, 16);
    }
}

// ------------------------------------------------------------
//  Timer (for Doom's tic rate, not used in this simple loop)
// ------------------------------------------------------------
int I_GetTime(void) {
    return 0;
}

// ------------------------------------------------------------
//  File I/O – map fopen/fclose to PS4 syscalls
// ------------------------------------------------------------
#include <stdarg.h>
#include <stddef.h>

#define MAX_FDS 16
static s32 fd_table[MAX_FDS] = { -1, -1, -1, -1, -1, -1, -1, -1,
                                  -1, -1, -1, -1, -1, -1, -1, -1 };

// override fopen/fclose/fread/fwrite/fseek
FILE *fopen(const char *path, const char *mode) {
    (void)mode; // assume read for now
    void *kopen = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelOpen");
    if (!kopen) return NULL;
    s32 fd = (s32)NC(G, kopen, (u64)path, 0, 0, 0, 0, 0);
    if (fd < 0) return NULL;
    for (int i = 0; i < MAX_FDS; i++) {
        if (fd_table[i] == -1) {
            fd_table[i] = fd;
            return (FILE *)(intptr_t)(i + 1); // return a unique index
        }
    }
    return NULL;
}

int fclose(FILE *stream) {
    int idx = (int)(intptr_t)stream - 1;
    if (idx < 0 || idx >= MAX_FDS) return -1;
    void *kclose = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelClose");
    if (!kclose) return -1;
    NC(G, kclose, (u64)fd_table[idx], 0, 0, 0, 0, 0);
    fd_table[idx] = -1;
    return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    int idx = (int)(intptr_t)stream - 1;
    if (idx < 0 || idx >= MAX_FDS || fd_table[idx] < 0) return 0;
    void *kread = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelRead");
    if (!kread) return 0;
    s64 total = size * nmemb;
    s32 n = (s32)NC(G, kread, (u64)fd_table[idx], (u64)ptr, (u64)total, 0, 0, 0);
    return (n > 0) ? (n / size) : 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    return 0; // no write needed
}

int fseek(FILE *stream, long offset, int whence) {
    int idx = (int)(intptr_t)stream - 1;
    if (idx < 0 || idx >= MAX_FDS || fd_table[idx] < 0) return -1;
    void *klseek = SYM(G, D, LIBKERNEL_HANDLE, "sceKernelLseek");
    if (!klseek) return -1;
    NC(G, klseek, (u64)fd_table[idx], (u64)offset, (u64)whence, 0, 0, 0);
    return 0;
}

// ------------------------------------------------------------
//  Main initialisation & shutdown
// ------------------------------------------------------------
void I_Init(void) {
    // already done in main.c
}

void I_Shutdown(void) {
    // cleanup audio, video, etc.
}
