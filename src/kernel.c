#include "kernel.h"
#include "shell.h"
#include "vfs.h"
#include "sha256.h"
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>

kernel_state_t g_kstate;

static void trim_nl(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

static void get_home_dir(char *out, int max) {
    const char *h = getenv("HOME");
    if (!h) h = ".";
    strncpy(out, h, max - 1);
    out[max - 1] = 0;
}

static void read_current_user(char *out, int max) {
    out[0] = 0;
    char home[512], p[600];
    get_home_dir(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/user", home);
    FILE *f = fopen(p, "r");
    if (f) {
        if (fgets(out, max, f)) out[strcspn(out, "\r\n")] = 0;
        fclose(f);
    }
    if (!out[0]) {
        strncpy(out, "XiaofangOS", max - 1);
        out[max - 1] = 0;
    }
}

static void add_user_record(const char *name) {
    char home[512], p[600];
    get_home_dir(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/users", home);
    FILE *f = fopen(p, "r");
    if (f) {
        char buf[128];
        while (fgets(buf, sizeof(buf), f)) {
            trim_nl(buf);
            if (strcmp(buf, name) == 0) { fclose(f); return; }
        }
        fclose(f);
    }
    f = fopen(p, "a");
    if (f) { fprintf(f, "%s\n", name); fclose(f); }
}

static void oobe_mark_done(const char *dir) {
    char p[600];
    snprintf(p, sizeof(p), "%s/oobe", dir);
    FILE *f = fopen(p, "w");
    if (f) { fputs("true\n", f); fclose(f); }
}

static int oobe_is_done(const char *dir) {
    char p[600];
    snprintf(p, sizeof(p), "%s/oobe", dir);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    char buf[16] = {0};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    trim_nl(buf);
    return strcmp(buf, "true") == 0;
}

static void save_user(const char *dir, const char *name) {
    char p[600];
    snprintf(p, sizeof(p), "%s/user", dir);
    FILE *f = fopen(p, "w");
    if (f) { fprintf(f, "%s\n", name); fclose(f); }
}

static void save_password(const char *dir, const char *user, const char *pass) {
    char p[600], hex[65];
    snprintf(p, sizeof(p), "%s/system/%s.password", dir, user);
    FILE *f = fopen(p, "w");
    if (!f) return;
    sha256_hex(pass, hex);
    fprintf(f, "%s\n", hex);
    fclose(f);
}

static void ask(const char *label, char *out, int max, const char *def) {
    printf("%s: ", label);
    fflush(stdout);
    char buf[256] = {0};
    if (fgets(buf, sizeof(buf), stdin)) {
        trim_nl(buf);
        if (buf[0]) {
            strncpy(out, buf, max - 1);
            out[max - 1] = 0;
            return;
        }
    }
    strncpy(out, def, max - 1);
    out[max - 1] = 0;
}

static void ask_pass(const char *label, char *out, int max) {
    printf("%s: ", label);
    fflush(stdout);
    struct termios old, t;
    tcgetattr(STDIN_FILENO, &old);
    t = old;
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    char buf[256] = {0};
    if (fgets(buf, sizeof(buf), stdin)) trim_nl(buf);

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    printf("\n");

    strncpy(out, buf, max - 1);
    out[max - 1] = 0;
}

static void oobe_run(const char *dir) {
    printf("\n=== 欢迎使用 XiaofangOS ===\n");
    printf("首次启动，先做几项设置（直接回车用默认）。\n\n");

    char region[64], keyboard[64], user[64], pass[128];
    ask("地区", region, sizeof(region), "中国");
    ask("键盘布局", keyboard, sizeof(keyboard), "us");
    ask("用户名", user, sizeof(user), "XiaofangOS");
    ask_pass("密码（留空不设密码）", pass, sizeof(pass));

    char sysdir[600], homedir[600], userhome[700];
    snprintf(sysdir, sizeof(sysdir), "%s/system", dir);
    snprintf(homedir, sizeof(homedir), "%s/home", dir);
    snprintf(userhome, sizeof(userhome), "%s/%s", homedir, user);
    mkdir(sysdir, 0755);
    mkdir(homedir, 0755);
    mkdir(userhome, 0755);

    save_user(dir, user);
    add_user_record(user);
    if (pass[0]) save_password(dir, user, pass);
    oobe_mark_done(dir);

    printf("\n设置完成，欢迎你，%s。\n\n", user);
}

static void oobe_check(void) {
    const char *home = getenv("HOME");
    if (!home) home = ".";
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/.XiaofangOS", home);
    mkdir(dir, 0755);

    if (!oobe_is_done(dir)) {
        char p[600];
        snprintf(p, sizeof(p), "%s/oobe", dir);
        FILE *f = fopen(p, "r");
        if (!f) {
            f = fopen(p, "w");
            if (f) { fputs("false\n", f); fclose(f); }
        } else {
            fclose(f);
        }
        oobe_run(dir);
    }
}

void kernel_tick(void) {
    g_kstate.uptime_ticks++;
}

void kernel_panic(const char *reason) {
    printf("\n*** KERNEL PANIC: %s ***\n", reason);
    printf("系统已停止。\n");
    exit(1);
}

void kernel_boot(void) {
    printf("正在启动 %s %s...\n", KERNEL_NAME, KERNEL_VERSION);
    fflush(stdout);
    sleep(2);
    printf("启动完成\n\n");

    g_kstate.running      = 1;
    g_kstate.reboot       = 0;
    g_kstate.uptime_ticks = 0;

    vfs_init();
    oobe_check();

    /* 初始 cwd 设为当前用户目录 */
    char user[64];
    read_current_user(user, sizeof(user));
    snprintf(g_kstate.cwd, sizeof(g_kstate.cwd), "/home/%s", user);
}

void kernel_shutdown(void) {
    printf("再见。\n");
    g_kstate.running = 0;
}

int main(void) {
    while (1) {
        kernel_boot();
        while (g_kstate.running) {
            kernel_tick();
            shell_prompt();
        }
        if (!g_kstate.reboot) {
            kernel_shutdown();
            return 0;
        }
        printf("正在重启...\n\n");
        fflush(stdout);
        printf("\033[2J\033[H");
        fflush(stdout);
    }
}
