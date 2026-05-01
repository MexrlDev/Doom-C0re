CC      = gcc
OBJCOPY = objcopy

# Same flags as your original NES payload
CFLAGS  = -Os -ffreestanding -fno-stack-protector -fno-builtin \
          -fpie -mno-red-zone -fomit-frame-pointer -fcf-protection=none \
          -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
          -Wall -Wno-unused-function \
          -Ips4 -Idoomgeneric

LDFLAGS = -T linker.ld -nostdlib -nostartfiles -static \
          -Wl,--build-id=none -Wl,--no-dynamic-linker -Wl,-z,norelro -no-pie

# ==================== Platform layer (your PS4 code) ====================
PS4_SRCS = ps4/main.c ps4/hijack.c ps4/ftp.c \
           ps4/ps4_video.c ps4/ps4_input.c ps4/ps4_audio.c ps4/ps4_system.c

# ==================== Doom engine sources ===============================
# Take all .c files from doomgeneric/ ...
DOOM_ALL = $(wildcard doomgeneric/*.c)

# ... except the platform-specific ones that we replace
SKIP := doomgeneric/doomgeneric_allegro.c \
        doomgeneric/doomgeneric_emscripten.c \
        doomgeneric/doomgeneric_linuxvt.c \
        doomgeneric/doomgeneric_sdl.c \
        doomgeneric/doomgeneric_soso.c \
        doomgeneric/doomgeneric_sosox.c \
        doomgeneric/doomgeneric_win.c \
        doomgeneric/doomgeneric_xlib.c \
        doomgeneric/i_allegromusic.c \
        doomgeneric/i_allegrosound.c \
        doomgeneric/i_cdmus.c \
        doomgeneric/i_endoom.c \
        doomgeneric/i_input.c \
        doomgeneric/i_joystick.c \
        doomgeneric/i_scale.c \
        doomgeneric/i_sdlmusic.c \
        doomgeneric/i_sdlsound.c \
        doomgeneric/i_sound.c \
        doomgeneric/i_system.c \
        doomgeneric/i_timer.c \
        doomgeneric/i_video.c \
        doomgeneric/memio.c \
        doomgeneric/mus2mid.c \
        doomgeneric/net_client.c \
        doomgeneric/net_dedicated.c \
        doomgeneric/net_gui.c \
        doomgeneric/net_io.c \
        doomgeneric/net_loop.c \
        doomgeneric/net_packet.c \
        doomgeneric/net_query.c \
        doomgeneric/net_sdl.c \
        doomgeneric/net_server.c \
        doomgeneric/sha1.c \
        doomgeneric/statdump.c \
        doomgeneric/w_checksum.c

DOOM_SRCS = $(filter-out $(SKIP), $(DOOM_ALL))

# (Optional) If you still have duplicates, add more filter-items here.

# ==================== All source files ===================================
SRCS = $(PS4_SRCS) $(DOOM_SRCS)

# Object files in the root directory (or you can keep them in a build/ subdir)
OBJS = $(SRCS:.c=.o)

TARGET = doom

all: $(TARGET).bin

# Uncomment the next line if you need to print the final file size
# @echo "Built: $@ ($$(wc -c < $@) bytes)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $@ ($$(wc -c < $@) bytes)"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin

.PHONY: all clean
