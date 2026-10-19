#include "kernel.h"
#include "shell.h"
#include "vfs.h"
#include "sha256.h"
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef struct {
    const char *name;
    void (*fn)(int argc, char **argv);
} cmd_t;

static void cmd_help(int argc, char **argv);
static void cmd_exit(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_uptime(int argc, char **argv);
static void cmd_ls(int argc, char **argv);
static void cmd_touch(int argc, char **argv);
static void cmd_cat(int argc, char **argv);
static void cmd_rm(int argc, char **argv);
static void cmd_version(int argc, char **argv);
static void cmd_whoami(int argc, char **argv);
static void cmd_reboot(int argc, char **argv);
static void cmd_su(int argc, char **argv);
static void cmd_useradd(int argc, char **argv);
static void cmd_users(int argc, char **argv);
static void cmd_passwd(int argc, char **argv);
static void cmd_cd(int argc, char **argv);
static void cmd_pwd(int argc, char **argv);

static cmd_t g_cmds[] = {
    {"help",    cmd_help},
    {"exit",    cmd_exit},
    {"echo",    cmd_echo},
    {"uptime",  cmd_uptime},
    {"ls",      cmd_ls},
    {"touch",   cmd_touch},
    {"cat",     cmd_cat},
    {"rm",      cmd_rm},
    {"version", cmd_version},
    {"whoami",  cmd_whoami},
    {"reboot",  cmd_reboot},
    {"su",      cmd_su},
    {"useradd", cmd_useradd},
    {"users",   cmd_users},
    {"passwd",  cmd_passwd},
    {"cd",      cmd_cd},
    {"pwd",     cmd_pwd},
};
#define CMD_COUNT (sizeof(g_cmds) / sizeof(g_cmds[0]))

static struct termios g_saved;
static int g_raw = 0;
static int g_logged_in = 0;
static char g_prompt[320];

static void trim_nl(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

static void get_home(char *out, int max) {
    const char *h = getenv("HOME");
    if (!h) h = ".";
    strncpy(out, h, max - 1);
    out[max - 1] = 0;
}

static void get_user(char *out, int max) {
    out[0] = 0;
    char home[512], p[600];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/user", home);
    FILE *f = fopen(p, "r");
    if (f) {
        if (fgets(out, max, f)) trim_nl(out);
        fclose(f);
    }
    int ok = 0;
    for (int i = 0; out[i]; i++) {
        if (out[i] != ' ' && out[i] != '\t') { ok = 1; break; }
    }
    if (!ok) {
        strncpy(out, "XiaofangOS", max - 1);
        out[max - 1] = 0;
    }
}

static void set_user(const char *name) {
    char home[512], p[600];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/user", home);
    FILE *f = fopen(p, "w");
    if (f) { fprintf(f, "%s\n", name); fclose(f); }
}

static int user_exists(const char *name) {
    char home[512], p[600];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/users", home);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    char buf[128];
    int found = 0;
    while (fgets(buf, sizeof(buf), f)) {
        trim_nl(buf);
        if (strcmp(buf, name) == 0) { found = 1; break; }
    }
    fclose(f);
    return found;
}

static int add_user(const char *name) {
    if (user_exists(name)) return 0;
    char home[512], p[600];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/users", home);
    FILE *f = fopen(p, "a");
    if (!f) return 0;
    fprintf(f, "%s\n", name);
    fclose(f);
    return 1;
}

/* 用户目录: ~/.XiaofangOS/home/<user> */
static void user_dir(const char *name, char *out, int max) {
    char home[512];
    get_home(home, sizeof(home));
    snprintf(out, max, "%s/.XiaofangOS/home/%s", home, name);
}

static void ensure_user_dir(const char *name) {
    char home[512], base[600], dir[700];
    get_home(home, sizeof(home));
    snprintf(base, sizeof(base), "%s/.XiaofangOS/home", home);
    mkdir(base, 0755);
    user_dir(name, dir, sizeof(dir));
    mkdir(dir, 0755);
}

/* 当前登录用户的目录, 用作 cwd 参考 */
static void current_dir(char *out, int max) {
    char u[64];
    get_user(u, sizeof(u));
    user_dir(u, out, max);
}

static int load_password_hash(const char *name, char out[65]) {
    char home[512], p[600];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/system/%s.password", home, name);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    if (!fgets(out, 65, f)) { fclose(f); return 0; }
    fclose(f);
    trim_nl(out);
    return out[0] != 0;
}

static int check_password(const char *name, const char *pass) {
    char stored[65];
    if (!load_password_hash(name, stored)) return 1;
    char hex[65];
    sha256_hex(pass, hex);
    return strcmp(stored, hex) == 0;
}

static void read_password(const char *label, char *out, int max) {
    printf("%s", label);
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

static void save_password(const char *name, const char *pass) {
    char home[512], p[600], hex[65];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/system/%s.password", home, name);
    FILE *f = fopen(p, "w");
    if (!f) return;
    sha256_hex(pass, hex);
    fprintf(f, "%s\n", hex);
    fclose(f);
}

static void ensure_system_dir(void) {
    char home[512], dir[600];
    get_home(home, sizeof(home));
    snprintf(dir, sizeof(dir), "%s/.XiaofangOS/system", home);
    mkdir(dir, 0755);
}

static int run_system_sh(const char *cmd, int argc, char **argv) {
    char home[512], path[600];
    get_home(home, sizeof(home));
    snprintf(path, sizeof(path), "%s/.XiaofangOS/system/%s.sh", home, cmd);
    if (access(path, F_OK) != 0) return 0;
    char line[MAX_CMD_LEN];
    int off = snprintf(line, sizeof(line), "sh '%s'", path);
    for (int i = 1; i < argc && off < (int)sizeof(line); i++)
        off += snprintf(line + off, sizeof(line) - off, " '%s'", argv[i]);
    int rc = system(line);
    (void)rc;
    return 1;
}

static void dispatch(int argc, char **argv) {
    if (argc == 0) return;
    for (size_t i = 0; i < CMD_COUNT; i++) {
        if (strcmp(argv[0], g_cmds[i].name) == 0) {
            g_cmds[i].fn(argc, argv);
            return;
        }
    }
    if (run_system_sh(argv[0], argc, argv)) return;
    printf("未知命令: %s (用 help 查看)\n", argv[0]);
}

static void raw_on(void) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &g_saved);
    t = g_saved;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    g_raw = 1;
}

static void raw_off(void) {
    if (g_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
        g_raw = 0;
    }
}

static void render(const char *buf, int len, int pos) {
    printf("\r\033[K%s", g_prompt);
    fwrite(buf, 1, len, stdout);
    int back = len - pos;
    if (back > 0) printf("\033[%dD", back);
    fflush(stdout);
}

static int read_line(char *buf, int max) {
    int len = 0, pos = 0;
    buf[0] = 0;
    while (1) {
        unsigned char c;
        if (read(STDIN_FILENO, &c, 1) != 1) return -1;
        if (c == '\r' || c == '\n') { printf("\n"); buf[len] = 0; return len; }
        if (c == 127 || c == 8) {
            if (pos > 0) {
                memmove(&buf[pos-1], &buf[pos], len - pos);
                len--; pos--; buf[len] = 0;
                render(buf, len, pos);
            }
            continue;
        }
        if (c == 27) {
            unsigned char a, b;
            if (read(STDIN_FILENO, &a, 1) != 1) return -1;
            if (read(STDIN_FILENO, &b, 1) != 1) return -1;
            if (a == '[') {
                if (b == 'D') { if (pos > 0) { pos--; printf("\033[D"); fflush(stdout); } }
                else if (b == 'C') { if (pos < len) { pos++; printf("\033[C"); fflush(stdout); } }
                else if (b == 'H') { if (pos > 0) { printf("\033[%dD", pos); fflush(stdout); pos = 0; } }
                else if (b == 'F') { int d = len - pos; if (d > 0) { printf("\033[%dC", d); fflush(stdout); pos = len; } }
                else if (b == '3') {
                    unsigned char tilde;
                    if (read(STDIN_FILENO, &tilde, 1) != 1) return -1;
                    if (tilde == '~' && pos < len) {
                        memmove(&buf[pos], &buf[pos+1], len - pos - 1);
                        len--; buf[len] = 0;
                        render(buf, len, pos);
                    }
                }
            }
            continue;
        }
        if (c < 32) continue;
        if (len >= max - 1) continue;
        memmove(&buf[pos+1], &buf[pos], len - pos);
        buf[pos] = c;
        len++; pos++; buf[len] = 0;
        render(buf, len, pos);
    }
}

static void login_once(void) {
    if (g_logged_in) return;
    char user[64];
    get_user(user, sizeof(user));
    ensure_user_dir(user);
    char stored[65];
    if (!load_password_hash(user, stored)) { g_logged_in = 1; return; }

    int tries = 0;
    while (tries < 3) {
        char pass[256], prompt[128];
        snprintf(prompt, sizeof(prompt), "%s 的密码: ", user);
        read_password(prompt, pass, sizeof(pass));
        char hex[65];
        sha256_hex(pass, hex);
        if (strcmp(stored, hex) == 0) { g_logged_in = 1; return; }
        printf("密码错误\n");
        tries++;
    }
    printf("密码错误次数过多，退出。\n");
    g_kstate.running = 0;
}

void shell_prompt(void) {
    ensure_system_dir();
    login_once();
    if (!g_kstate.running) return;

    char user[64];
    get_user(user, sizeof(user));
    snprintf(g_prompt, sizeof(g_prompt), "%s:%s/ ", user, strcmp(g_kstate.cwd, "/") == 0 ? "" : g_kstate.cwd);
    printf("%s", g_prompt);
    fflush(stdout);

    char line[MAX_CMD_LEN];
    raw_on();
    int n = read_line(line, sizeof(line));
    raw_off();

    if (n < 0) { g_kstate.running = 0; return; }
    if (n == 0) return;

    char *argv[MAX_ARGS];
    int argc = 0;
    char *tok = strtok(line, " \t");
    while (tok && argc < MAX_ARGS) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    dispatch(argc, argv);
}

static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("内置命令:\n");
    for (size_t i = 0; i < CMD_COUNT; i++) printf("  %s\n", g_cmds[i].name);
    printf("\n外部命令: 放在 ~/.XiaofangOS/system/<名字>.sh\n");
}

static void cmd_exit(int argc, char **argv) { (void)argc; (void)argv; g_kstate.running = 0; }

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) printf("%s%s", argv[i], i == argc-1 ? "\n" : " ");
    if (argc == 1) printf("\n");
}

static void cmd_uptime(int argc, char **argv) { (void)argc; (void)argv; printf("tick = %d\n", g_kstate.uptime_ticks); }

/* ls 改成列当前用户目录 */
static void cmd_ls(int argc, char **argv) {
    (void)argc; (void)argv;
    char dir[700];
    current_dir(dir, sizeof(dir));
    DIR *d = opendir(dir);
    if (!d) { printf("(空)\n"); return; }
    struct dirent *e;
    int n = 0;
    while ((e = readdir(d))) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        printf("  %s\n", e->d_name);
        n++;
    }
    closedir(d);
    if (!n) printf("(空)\n");
}

static void cmd_touch(int argc, char **argv) {
    if (argc < 2) { printf("用法: touch <file>\n"); return; }
    char dir[700], path[900];
    current_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/%s", dir, argv[1]);
    FILE *f = fopen(path, "a");
    if (f) { fclose(f); printf("已创建: %s\n", argv[1]); }
    else printf("创建失败\n");
}

static void cmd_cat(int argc, char **argv) {
    if (argc < 2) { printf("用法: cat <file>\n"); return; }
    char dir[700], path[900];
    current_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/%s", dir, argv[1]);
    FILE *f = fopen(path, "r");
    if (!f) { printf("不存在: %s\n", argv[1]); return; }
    char buf[512];
    while (fgets(buf, sizeof(buf), f)) fputs(buf, stdout);
    fclose(f);
}

static void cmd_rm(int argc, char **argv) {
    if (argc < 2) { printf("用法: rm <file>\n"); return; }
    char dir[700], path[900];
    current_dir(dir, sizeof(dir));
    snprintf(path, sizeof(path), "%s/%s", dir, argv[1]);
    if (remove(path) == 0) printf("已删除: %s\n", argv[1]);
    else printf("删除失败: %s\n", argv[1]);
}

static void cmd_version(int argc, char **argv) { (void)argc; (void)argv; printf("%s %s\n", KERNEL_NAME, KERNEL_VERSION); }

static void cmd_whoami(int argc, char **argv) {
    (void)argc; (void)argv;
    char u[64];
    get_user(u, sizeof(u));
    printf("%s\n", u);
}

static void cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    g_kstate.reboot  = 1;
    g_kstate.running = 0;
}

static void cmd_su(int argc, char **argv) {
    if (argc < 2) { printf("用法: su <用户名>\n"); return; }
    if (!user_exists(argv[1])) { printf("未知的用户: %s\n", argv[1]); return; }
    char stored[65];
    if (load_password_hash(argv[1], stored)) {
        char pass[256], hex[65];
        read_password("密码: ", pass, sizeof(pass));
        sha256_hex(pass, hex);
        if (strcmp(stored, hex) != 0) { printf("密码错误\n"); return; }
    }
    set_user(argv[1]);
    ensure_user_dir(argv[1]);
    snprintf(g_kstate.cwd, sizeof(g_kstate.cwd), "/home/%s", argv[1]);
    printf("已切换用户: %s\n", argv[1]);
}

static void cmd_useradd(int argc, char **argv) {
    if (argc < 2) { printf("用法: useradd <用户名>\n"); return; }
    if (!add_user(argv[1])) { printf("用户已存在: %s\n", argv[1]); return; }
    ensure_user_dir(argv[1]);
    printf("已创建用户: %s\n", argv[1]);
    char pass[256];
    read_password("设置密码（留空不设）: ", pass, sizeof(pass));
    if (pass[0]) {
        save_password(argv[1], pass);
        printf("密码已设置\n");
    }
}

static void cmd_users(int argc, char **argv) {
    (void)argc; (void)argv;
    char home[512], p[600];
    get_home(home, sizeof(home));
    snprintf(p, sizeof(p), "%s/.XiaofangOS/users", home);
    FILE *f = fopen(p, "r");
    if (!f) { printf("(无用户)\n"); return; }
    char buf[128];
    while (fgets(buf, sizeof(buf), f)) { trim_nl(buf); if (buf[0]) printf("  %s\n", buf); }
    fclose(f);
}

static void cmd_passwd(int argc, char **argv) {
    (void)argc; (void)argv;
    char user[64];
    get_user(user, sizeof(user));
    char old[256], fresh[256];
    read_password("当前密码: ", old, sizeof(old));
    if (!check_password(user, old)) { printf("密码错误\n"); return; }
    read_password("新密码: ", fresh, sizeof(fresh));
    if (!fresh[0]) { printf("取消\n"); return; }
    save_password(user, fresh);
    printf("密码已更新\n");
}

static void cmd_cd(int argc, char **argv) {
    if (argc < 2) {
        char u[64];
        get_user(u, sizeof(u));
        snprintf(g_kstate.cwd, sizeof(g_kstate.cwd), "/home/%s", u);
        return;
    }
    if (strcmp(argv[1], "/") == 0) {
        strcpy(g_kstate.cwd, "/");
        return;
    }
    /* 只支持绝对 /home/<user> 形式 */
    snprintf(g_kstate.cwd, sizeof(g_kstate.cwd), "%s", argv[1]);
}

static void cmd_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("%s\n", g_kstate.cwd);
}
