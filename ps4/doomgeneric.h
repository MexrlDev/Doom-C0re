/* Minimal DoomGeneric for PS4 payload – no standard headers */
#include "doomgeneric.h"

/* Screen dimensions – classic Doom */
#define DOOMGENERIC_RESX  320
#define DOOMGENERIC_RESY  200

/* pixel type (32‑bit ARGB) */
typedef u32 pixel_t;

/* Global screen buffer that the engine draws into */
pixel_t* DG_ScreenBuffer = NULL;

/* Forward declarations of internal Doom engine functions */
void D_DoomMain(void);
void D_Display(void);
void D_DoomLoop(void);

/* ----------------------------------------------------------------
 *  Memory allocation
 * ---------------------------------------------------------------- */
static void* my_malloc(u32 size) {
    extern void *G, *D;
    void *mmap = SYM(G, D, LIBKERNEL_HANDLE, "mmap");
    if (!mmap) return NULL;
    void *ptr = (void*)NC(G, mmap, 0, (u64)size, 3, 0x1002, (u64)-1, 0);
    if ((s64)ptr == -1) return NULL;
    return ptr;
}

static void my_free(void *ptr, u32 size) {
    extern void *G, *D;
    void *munmap = SYM(G, D, LIBKERNEL_HANDLE, "munmap");
    if (munmap && ptr) NC(G, munmap, (u64)ptr, (u64)size, 0, 0, 0, 0);
}

/* ----------------------------------------------------------------
 *  DoomGeneric entry points
 * ---------------------------------------------------------------- */
void doomgeneric_Init(void) {
    /* Allocate the internal 320x200 ARGB buffer */
    DG_ScreenBuffer = (pixel_t*)my_malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
    if (!DG_ScreenBuffer) I_Error("Failed to allocate screen buffer");

    /* Set the engine’s global screen pointer */
    screen = (u8 *)DG_ScreenBuffer;
    D_DoomMain();
}

void doomgeneric_Tick(void) {
    /* Run one frame */
    D_Display();
}

void doomgeneric_Shutdown(void) {
    /* Free the screen buffer */
    if (DG_ScreenBuffer) {
        my_free(DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
        DG_ScreenBuffer = NULL;
    }
}
