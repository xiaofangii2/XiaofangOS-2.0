#include "vfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILES   16
#define NAME_MAXLEN 64
#define DATA_MAXLEN 512

typedef struct {
    char name[NAME_MAXLEN];
    char data[DATA_MAXLEN];
    int  used;
} inode_t;

static inode_t g_inodes[MAX_FILES];

void vfs_init(void) {
    memset(g_inodes, 0, sizeof(g_inodes));
}

void vfs_list(void) {
    int n = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (g_inodes[i].used) {
            printf("  [%d] %s\n", i, g_inodes[i].name);
            n++;
        }
    }
    if (!n) printf("  (空)\n");
}

static int find(const char *name) {
    for (int i = 0; i < MAX_FILES; i++)
        if (g_inodes[i].used && strcmp(g_inodes[i].name, name) == 0)
            return i;
    return -1;
}

int vfs_create(const char *name) {
    if (strlen(name) >= NAME_MAXLEN) { printf("文件名过长\n"); return -1; }
    if (find(name) >= 0) { printf("已存在\n"); return -1; }
    for (int i = 0; i < MAX_FILES; i++) {
        if (!g_inodes[i].used) {
            g_inodes[i].used = 1;
            strncpy(g_inodes[i].name, name, NAME_MAXLEN - 1);
            g_inodes[i].data[0] = '\0';
            printf("已创建: %s\n", name);
            return 0;
        }
    }
    printf("inode 表已满\n");
    return -1;
}

int vfs_delete(const char *name) {
    int i = find(name);
    if (i < 0) { printf("不存在: %s\n", name); return -1; }
    g_inodes[i].used = 0;
    printf("已删除: %s\n", name);
    return 0;
}

int vfs_read(const char *name) {
    int i = find(name);
    if (i < 0) { printf("不存在: %s\n", name); return -1; }
    printf("--- %s ---\n%s\n", name, g_inodes[i].data[0] ? g_inodes[i].data : "(空)");
    return 0;
}
