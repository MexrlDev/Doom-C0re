#ifndef DOOMGENERIC_H
#define DOOMGENERIC_H

#include "core.h"          // u8, u32, s16, …

// Doom engine framebuffer and palette
extern u8  *screen;        // 320×200 indexed framebuffer
extern u32 *curpal;        // 256‑entry ARGB palette

// Doom sound output (mixed by the engine)
extern void *sndOutput;    // pointer to 16‑bit mono samples
extern int  sndSamples;    // number of sample frames available

// Platform functions – implemented by PS4 layer
void I_Init(void);
void I_Shutdown(void);
void I_Error(const char *msg);
void I_Quit(void);
void I_PrintStr(const char *str);

void I_InitGraphics(void);
void I_ShutdownGraphics(void);
void I_FinishUpdate(void);
void I_ReadKeys(void);
int  I_GetKey(void);
void I_SetWindowTitle(const char *title);

void I_InitNetwork(void);
void I_NetCmd(void);

void I_StartTic(void);
void I_StartFrame(void);
void I_UpdateSound(void);
void I_SubmitSound(void);

// Generic engine entry points
void doomgeneric_Init(void);
void doomgeneric_Tick(void);
void doomgeneric_Shutdown(void);

#endif
