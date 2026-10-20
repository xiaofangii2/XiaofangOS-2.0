# XiaofangOS 2.0

XiaofangOS 是一个用纯 C 从零写起的 ARM64 内核。它不依赖 libc，所有系统调用自己封装，所有字符串函数自己写，所有输出自己刷，自带 shell、用户管理、密码验证、文件操作和命令行编辑。内核可以编译成静态 ELF，在 ARM64 裸机上运行。

这是 XiaofangOS 系列的第二版。第一版是用 PHP 写的，跑在网页环境里，验证了命令分派和用户系统的基本逻辑。第二版改用 C，目标换成 ARM64，直接运行在硬件上。两版共享同一套设计思路：一个极简内核，一个可交互的 shell，一个用户系统，一个密码验证，一个文件系统。

XiaofangOS 2.0 的目标不是替代 Linux，也不是做一个通用的操作系统。它的目标是把操作系统里那些看起来理所当然的东西——提示符、命令分派、用户、密码、目录、文件、历史记录——用最基础的代码亲手实现一遍，看看能有多小、多直接、多可控。答案是一个几万字节的静态 ELF，没有任何外部依赖，从启动到 shell 全部自己写。

## 版本

XiaofangOS 1.0：PHP 实现，网页环境，验证命令分派和用户系统的基本逻辑。它证明了这套设计思路可行，但受限于网页环境，无法直接操作文件系统，也无法脱离浏览器运行。

XiaofangOS 2.0：C 实现，ARM64 内核，不依赖 libc，走 syscall，裸机和用户态都能跑。它把 1.0 的设计思路完整地用 C 重写了一遍，去掉了所有外部依赖，直接和硬件或宿主机内核打交道。

## 安装

环境要求：一台 ARM64 设备，Termux，clang，ld.lld。Termux 自带 clang 和 ld.lld，不需要额外安装交叉工具链。设备可以是 Android 手机，也可以是任何能跑 Termux 的 ARM64 Linux 环境。

第一步，克隆仓库：

    git clone https://github.com/xiaofangii2/XiaofangOS-2.0.git
    cd XiaofangOS-2.0

第二步，编译内核。每个 .c 文件单独编译成 .o，然后用 ld.lld 链接成 ELF。编译选项里的 --target=aarch64-none-elf 表示目标平台是 ARM64 裸机，-ffreestanding 表示不假设有标准库，-fno-pic 表示不生成位置无关代码，-mgeneral-regs-only 表示只用通用寄存器，-nostdlib 表示不链接标准库。

    clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c bare/sys.c -o bare/sys.o
    clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c bare/sha256.c -o bare/sha256.o
    clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c bare/vfs.c -o bare/vfs.o
    clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c bare/shell.c -o bare/shell.o
    clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c bare/kernel.c -o bare/kernel.o
    clang --target=aarch64-none-elf -c bare/boot.s -o bare/boot.o
    ld.lld -m aarch64elf -T bare/linker.ld -o bare/xiaofang.elf bare/boot.o bare/kernel.o bare/sys.o bare/vfs.o bare/shell.o bare/sha256.o

第三步，编译加载器。加载器是一个普通的 ARM64 用户态程序，负责读 ELF、解析程序头、mmap 到指定地址、跳入口。

    clang -O2 -o bare/loader bare/loader.c

第四步，运行。加载器会打印 ELF 的入口地址、虚拟地址范围、每个段的加载信息，然后跳进去。

    ./bare/loader ./bare/xiaofang.elf

装到 proot 容器。proot 容器需要三个东西：一个 rootfs 目录，一个启动脚本，一个 tar 包。启动脚本放在 bin/sh，内容是 exec /boot/loader /boot/xiaofang.elf。proot-distro 登录时会执行 bin/sh，所以 bin/sh 就是整个系统的入口。

    mkdir -p xiaofang-rootfs/boot xiaofang-rootfs/bin
    cp bare/xiaofang.elf bare/loader xiaofang-rootfs/boot/
    printf '#!/system/bin/sh\nexec /boot/loader /boot/xiaofang.elf\n' > xiaofang-rootfs/bin/sh
    chmod +x xiaofang-rootfs/bin/sh
    cd xiaofang-rootfs
    tar -czf ../xiaofang-rootfs.tar.gz .
    cd ..
    proot-distro remove xiaofang
    proot-distro install --override-alias xiaofang ./xiaofang-rootfs.tar.gz
    proot-distro login xiaofang

## 项目结构

mykernel/
  bare/                  内核源码目录
    boot.s               启动入口，汇编写的 _start，设置栈，跳 kmain
    linker.ld            链接脚本，加载地址 0x40000000，栈在 bss 之后
    kernel.c             内核主体，打印启动信息，初始化 VFS，进入 shell
    kernel.h             内核状态定义
    shell.c              命令分派、用户系统、密码、文件操作、行编辑
    shell.h              shell 对外接口
    sys.c                系统调用封装，write/read/openat/close/exit/mkdirat/ioctl/getdents64
    sys.h                系统调用声明和字符串工具
    sha256.c             SHA-256 实现，纯计算，不依赖任何系统调用
    sha256.h
    vfs.c                简易内存虚拟文件系统
    vfs.h
    loader.c             用户态 ELF 加载器，解析 ELF 头，mmap 到指定地址，跳入口
    xiaofang.elf         编译产物，内核 ELF
    loader               编译产物，加载器可执行文件
  xiaofang-rootfs/       proot 容器 rootfs
    boot/                内核 ELF 和 loader
    bin/sh               启动脚本，exec /boot/loader /boot/xiaofang.elf
    etc/                 用户列表 user、当前用户 users、OOBE 标记 oobe
    home/                用户目录，每个用户一个子目录
    var/xiaofang/        密码文件，每个用户一个 用户名.password
  README.md

## 功能

OOBE 首次启动向导：地区、键盘布局、用户名、密码。第一次启动时自动运行，设置完成后写 /etc/oobe 标记，以后不再运行。

用户系统：useradd、users、su、passwd、whoami。useradd 创建用户和用户目录，users 列出所有用户，su 切换用户，passwd 改密码，whoami 显示当前用户。

密码：SHA-256 哈希，存 /var/xiaofang/用户名.password。密码不存明文，只存 64 个十六进制字符的哈希。登录和 su 时对比哈希。

文件操作：ls、touch、cat。ls 用 getdents64 列目录，touch 创建空文件，cat 读文件内容。

路径：cd、pwd。cd 切换目录，pwd 显示当前目录。提示符显示用户名和当前目录，末尾带斜杠和空格。

命令历史：上下键。上键翻上一条，下键翻下一条，最多 8 条。

自动补全：Tab 补全唯一命令。如果输入的字符能唯一匹配某个命令，按 Tab 就补全。

## 为什么自己做

因为想看看一个 shell 到底能有多小，一个用户系统到底能有多简单，一个密码验证到底能有多直接。XiaofangOS 2.0 用几万字节回答这个问题。

市面上的 shell 都依赖 libc，市面上的用户系统都依赖 PAM，市面上的密码验证都依赖 OpenSSL。XiaofangOS 2.0 全部自己写，从系统调用到 SHA-256，从字符串函数到命令分派，一行都不借。

这样做的好处是完全可控。每个字节都知道从哪来、到哪去。没有隐藏的库，没有隐藏的依赖，没有隐藏的行为。编译出来就是一个静态 ELF，拷到哪里都能跑。

## 设计取舍

不依赖 libc，所有系统调用自己封装，换来完全可控的二进制。

不依赖 shell 解释器，命令直接分派到函数，换来没有中间层。

密码存 SHA-256 哈希，不存明文，换来基本的密码安全。

用户目录独立，每个用户一个 /home 子目录，换来用户数据隔离。

## 后续

rm 命令。目前 rm 只打印暂不支持，需要加 unlinkat 系统调用。

上下键重绘当前行。目前上下键只替换内容，不重绘提示符和光标位置。

更多内置命令。比如 mkdir、rmdir、mv、cp、head、tail、grep。

真机镜像打包。把 xiaofang.elf 打包成可刷入的镜像，支持从 U 盘或 SD 卡启动。
