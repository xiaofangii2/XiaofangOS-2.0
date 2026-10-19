#ifndef KERNEL_H
#define KERNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KERNEL_NAME    "XiaofangOS"
#define KERNEL_VERSION "2.0"
#define MAX_CMD_LEN    1024
#define MAX_ARGS       32

typedef struct {
    int    running;
    int    reboot;
    int    uptime_ticks;
    char   cwd[256];
} kernel_state_t;

extern kernel_state_t g_kstate;

void kernel_boot(void);
void kernel_shutdown(void);
void kernel_tick(void);
void kernel_panic(const char *reason);

#endif
