#include "vfs.h"
#include "sys.h"
void vfs_init(void){}
void vfs_list(void){ puts_line("(vfs)"); }
int vfs_create(const char *name){ (void)name; return 0; }
int vfs_delete(const char *name){ (void)name; return 0; }
int vfs_read(const char *name){ (void)name; return 0; }
