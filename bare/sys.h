#ifndef SYS_H
#define SYS_H
long sys_write(int fd, const void *buf, unsigned long len);
long sys_read(int fd, void *buf, unsigned long len);
long sys_openat(int dirfd, const char *path, int flags, int mode);
long sys_close(int fd);
long sys_exit(int code);
long sys_mkdirat(int dirfd, const char *path, int mode);
long sys_ioctl(int fd, unsigned long req, void *arg);
long sys_getdents64(int fd, void *buf, unsigned long len);
long sys_unlinkat(int dirfd, const char *path, int flags);
struct linux_dirent64 { unsigned long d_ino; long d_off; unsigned short d_reclen; unsigned char d_type; char d_name[256]; };
void putc_(char c);
void puts_(const char *s);
void puts_line(const char *s);
unsigned long slen(const char *s);
int scmp(const char *a, const char *b);
void scpy(char *d, const char *s, int max);
int ssplit(char *line, char **argv, int max);
#endif
