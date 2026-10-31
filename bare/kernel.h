#ifndef KERNEL_H
#define KERNEL_H

typedef struct {
    int running;
    int reboot;
    int uptime_ticks;
    char cwd[256];
} kernel_state_t;

extern kernel_state_t g_kstate;

#endif
