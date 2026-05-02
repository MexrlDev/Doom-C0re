#include "core.h"
#include "hijack.h"
#include "doomgeneric.h"

// ---------- Global screen & palette ----------
u8  *screen = NULL;
u32 *curpal = NULL;

extern void *G;
extern void *D;
extern struct video_ctx v;

extern void *my_malloc(u32 size);
extern void  my_free(void *ptr, u32 size);

static int active = 0;

void I_InitGraphics(void) {
    screen = (u8*)my_malloc(320 * 200);
    curpal = (u32*)my_malloc(256 * 4);
}

void I_ShutdownGraphics(void) {
    if (screen) { my_free(screen, 320*200); screen = NULL; }
    if (curpal) { my_free(curpal, 256*4); curpal = NULL; }
}

void I_SetWindowTitle(const char *title) { (void)title; }

void *I_VideoBuffer(void) { return screen; }
void I_SetPalette(int palette_index) { }
int  I_GetPaletteIndex(int r, int g, int b) { return 0; }

void I_FinishUpdate(void) {
    u32 *fb = (u32 *)v.fbs[active];

    int scale_x = SCR_W / 320;
    int scale_y = SCR_H / 200;
    int scale = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale < 1) scale = 1;

    int src_w = 320;
    int src_h = 200;
    int dst_w = src_w * scale;
    int dst_h = src_h * scale;
    int off_x = (SCR_W - dst_w) / 2;
    int off_y = (SCR_H - dst_h) / 2;

    const u8 *src = screen;

    for (int y = 0; y < src_h; y++) {
        int base_y = off_y + y * scale;
        for (int x = 0; x < src_w; x++) {
            u8 idx = src[y * src_w + x];
            u32 color = curpal[idx];
            int base_x = off_x + x * scale;
            for (int dy = 0; dy < scale; dy++) {
                u32 *row = &fb[(base_y + dy) * SCR_W + base_x];
                for (int dx = 0; dx < scale; dx++)
                    row[dx] = color;
            }
        }
    }

    for (int y = 0; y < SCR_H; y++)
        for (int x = 0; x < SCR_W; x++)
            if (x < off_x || x >= off_x + dst_w || y < off_y || y >= off_y + dst_h)
                fb[y * SCR_W + x] = 0xFF000000;

    video_flip(&v, active);
    active ^= 1;
}
