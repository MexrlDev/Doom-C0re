#include "ftp.h"

/* simple helpers */
static int ftp_strlen(const char *s) {
    int n = 0; while (*s++) n++; return n;
}

static int ftp_startswith(const char *s, const char *prefix) {
    while (*prefix) { if (*s++ != *prefix++) return 0; }
    return 1;
}

static int ftp_atoi(const char *s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v;
}

static int ftp_itoa(char *buf, int val) {
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return 1; }
    if (val < 0) { buf[0] = '-'; return 1 + ftp_itoa(buf + 1, -val); }
    char tmp[16]; int i = 0;
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int len = i;
    for (int j = 0; j < len; j++) buf[j] = tmp[len - 1 - j];
    buf[len] = 0;
    return len;
}

static int is_wad_file(const char *name) {
    int len = ftp_strlen(name);
    if (len < 5) return 0;
    return (name[len-4] == '.' && name[len-3] == 'w' && name[len-2] == 'a' && name[len-1] == 'd');
}

static void ftp_log(void *G, void *sendto_fn, s32 fd, u8 *sa, const char *msg) {
    if (fd < 0 || !sendto_fn) return;
    int len = ftp_strlen(msg);
    NC(G, sendto_fn, (u64)fd, (u64)msg, (u64)len, 0, (u64)sa, 16);
}

/* minimal FTP server – accepts one file (must be .wad) and stops */
int ftp_wait_for_wad(s32 srv_fd, s32 data_listen_fd,
                     void *G, void *D,
                     void *load_mod, void *mmap,
                     void *kopen, void *kwrite, void *kclose, void *kmkdir,
                     void *getdents, void *usleep,
                     void *recvfrom, void *sendto, void *accept,
                     void *getsockname,
                     s32 log_fd, u8 *log_sa, s32 userId)
{
    (void)load_mod; (void)mmap; (void)kmkdir; (void)getdents; (void)usleep;
    (void)getsockname; (void)userId;

    void *close_fn = SYM(G, D, LIBKERNEL_HANDLE, "close");
    void *recvfrom_fn = recvfrom;   // same pointer, but we'll use it directly
    void *sendto_fn = sendto;
    void *accept_fn = accept;

    int got_wad = 0;
    char cmd_buf[512];

    ftp_log(G, sendto_fn, log_fd, log_sa, "FTP: listening on 1337\n");

    while (!got_wad) {
        s32 ctrl_fd = (s32)NC(G, accept_fn, (u64)srv_fd, 0, 0, 0, 0, 0);
        if (ctrl_fd < 0) continue;

        ftp_log(G, sendto_fn, log_fd, log_sa, "FTP: client connected\n");
        NC(G, sendto_fn, (u64)ctrl_fd, (u64)"220 PS4 Doom FTP\r\n", 18, 0, 0, 0);

        while (1) {
            s32 n = (s32)NC(G, recvfrom_fn, (u64)ctrl_fd, (u64)cmd_buf, 510, 0, 0, 0);
            if (n <= 0) break;
            cmd_buf[n] = 0;

            // match USER / PASS / TYPE / PASV / STOR / QUIT
            if (ftp_startswith(cmd_buf, "USER") || ftp_startswith(cmd_buf, "PASS"))
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)"230 OK\r\n", 7, 0, 0, 0);
            else if (ftp_startswith(cmd_buf, "TYPE"))
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)"200 OK\r\n", 8, 0, 0, 0);
            else if (ftp_startswith(cmd_buf, "PASV")) {
                // Return a fixed PASV response pointing to the data port
                char resp[80];
                int n2 = 0;
                const char *h = "227 Entering Passive Mode (127,0,0,1,5,58)\r\n"; // 5,58 = 1338
                while (*h) resp[n2++] = *h++;
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)resp, n2, 0, 0, 0);
            }
            else if (ftp_startswith(cmd_buf, "STOR")) {
                // Accept the file
                char *arg = cmd_buf + 5; // after "STOR "
                while (*arg == ' ') arg++;

                // Build destination path: FTP_DEST + filename
                char path[256];
                int pi = 0;
                const char *base = FTP_DEST;
                while (*base) path[pi++] = *base++;
                int fnlen = ftp_strlen(arg);
                for (int i = 0; i < fnlen && pi < 254; i++) path[pi++] = arg[i];
                path[pi] = 0;

                s32 fd = (s32)NC(G, SYM(G,D,LIBKERNEL_HANDLE,"sceKernelOpen"),
                                 (u64)path, 0x0601, 0x1FF, 0, 0, 0);
                if (fd < 0) {
                    NC(G, sendto_fn, (u64)ctrl_fd, (u64)"550 Cannot create file\r\n", 23, 0, 0, 0);
                    continue;
                }

                // Accept data connection
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)"150 Opening data connection\r\n", 28, 0, 0, 0);
                s32 data_fd = (s32)NC(G, accept_fn, (u64)data_listen_fd, 0, 0, 0, 0, 0);

                if (data_fd >= 0) {
                    u8 buf[FTP_BUF_SZ];
                    while (1) {
                        s32 r = (s32)NC(G, recvfrom_fn, (u64)data_fd, (u64)buf, FTP_BUF_SZ, 0, 0, 0);
                        if (r <= 0) break;
                        NC(G, SYM(G,D,LIBKERNEL_HANDLE,"sceKernelWrite"), (u64)fd, (u64)buf, (u64)r, 0, 0, 0);
                    }
                    if (close_fn) NC(G, close_fn, (u64)data_fd, 0, 0, 0, 0, 0);
                }

                NC(G, SYM(G,D,LIBKERNEL_HANDLE,"sceKernelClose"), (u64)fd, 0, 0, 0, 0, 0);
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)"226 Transfer complete\r\n", 23, 0, 0, 0);

                if (is_wad_file(arg)) {
                    got_wad = 1;
                    ftp_log(G, sendto_fn, log_fd, log_sa, "FTP: WAD received, exiting\n");
                }
                break; // disconnect after STOR
            }
            else if (ftp_startswith(cmd_buf, "QUIT")) {
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)"221 Bye\r\n", 9, 0, 0, 0);
                break;
            }
            else {
                NC(G, sendto_fn, (u64)ctrl_fd, (u64)"500 Unknown\r\n", 13, 0, 0, 0);
            }
        }

        if (close_fn) NC(G, close_fn, (u64)ctrl_fd, 0, 0, 0, 0, 0);
    }

    // Cleanup
    if (close_fn) {
        NC(G, close_fn, (u64)srv_fd, 0, 0, 0, 0, 0);
        NC(G, close_fn, (u64)data_listen_fd, 0, 0, 0, 0, 0);
    }
    return got_wad ? 1 : 0;
}
