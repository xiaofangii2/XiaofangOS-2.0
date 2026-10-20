#include "vfs.h"
#include "sys.h"

#define MAX_FILES   16
#define NAME_MAXLEN 64
#define DATA_MAXLEN 512

typedef struct { char name[NAME_MAXLEN]; char data[DATA_MAXLEN]; int used; } inode_t;
static inode_t g_inodes[MAX_FILES];

void vfs_init(void) { for (int i = 0; i < MAX_FILES; i++) g_inodes[i].used = 0; }

void vfs_list(void) {
    int n = 0;
    for (int i = 0; i < MAX_FILES; i++)
        if (g_inodes[i].used) { puts_("  "); puts_line(g_inodes[i].name); n++; }
    if (!n) puts_line("  (空)");
}
static int find(const char *name) {
    for (int i = 0; i < MAX_FILES; i++)
        if (g_inodes[i].used && scmp(g_inodes[i].name, name) == 0) return i;
    return -1;
}
int vfs_create(const char *name) {
    if (slen(name) >= NAME_MAXLEN) { puts_line("文件名过长"); return -1; }
    if (find(name) >= 0) { puts_line("已存在"); return -1; }
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_inodes[i].used) {
            g_inodes[i].used = 1;
            scpy(g_inodes[i].name, name, NAME_MAXLEN);
            g_inodes[i].data[0] = 0;
            puts_("已创建: "); puts_line(name);
            return 0;
        }
    }
    puts_line("inode 表已满");
    return -1;
}
int vfs_delete(const char *name) {
    int i = find(name);
    if (i < 0) { puts_("不存在: "); puts_line(name); return -1; }
    g_inodes[i].used = 0;
    puts_("已删除: "); puts_line(name);
    return 0;
}
int vfs_read(const char *name) {
    int i = find(name);
    if (i < 0) { puts_("不存在: "); puts_line(name); return -1; }
    puts_("--- "); puts_(name); puts_line(" ---");
    puts_line(g_inodes[i].data[0] ? g_inodes[i].data : "(空)");
    return 0;
}
