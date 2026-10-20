#include "sys.h"
#include "vfs.h"
#include "shell.h"

void kmain(void) {
    puts_line("");
    puts_line("正在启动 XiaofangOS 2.0...");
    puts_line("启动完成");
    puts_line("");
    vfs_init();
    shell_prompt("root");
    puts_line("再见。");
    sys_exit(0);
}
