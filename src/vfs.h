#ifndef VFS_H
#define VFS_H
void vfs_init(void);
void vfs_list(void);
int  vfs_create(const char *name);
int  vfs_delete(const char *name);
int  vfs_read(const char *name);
#endif
