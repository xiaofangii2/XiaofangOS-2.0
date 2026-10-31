#!/data/data/com.termux/files/usr/bin/bash
cd ~/mykernel
mkdir -p ~/mykernel/build
TMP=~/mykernel/build/.tmp
if [ $# -eq 0 ]; then
    set -- userland bare
fi
for t in "$@"; do
    rm -rf "$TMP"
    mkdir -p "$TMP"
    case "$t" in
        userland)
            mkdir -p "$TMP/root/boot" "$TMP/root/bin"
            clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c ~/mykernel/bare/sys.c -o "$TMP/sys.o"
            clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c ~/mykernel/bare/sha256.c -o "$TMP/sha256.o"
            clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c ~/mykernel/bare/vfs.c -o "$TMP/vfs.o"
            clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c ~/mykernel/bare/shell.c -o "$TMP/shell.o"
            clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c ~/mykernel/bare/kernel.c -o "$TMP/kernel.o"
            clang --target=aarch64-none-elf -c ~/mykernel/bare/boot.s -o "$TMP/boot.o"
            ld.lld -m aarch64elf -T ~/mykernel/bare/linker.ld -o "$TMP/xiaofang.elf" "$TMP/boot.o" "$TMP/kernel.o" "$TMP/sys.o" "$TMP/vfs.o" "$TMP/shell.o" "$TMP/sha256.o"
            cp "$TMP/xiaofang.elf" "$TMP/root/boot/"
            cp ~/mykernel/bare/loader "$TMP/root/boot/"
            printf '#!/system/bin/sh\nexec /boot/loader /boot/xiaofang.elf\n' > "$TMP/root/bin/sh"
            chmod +x "$TMP/root/bin/sh"
            cd "$TMP/root"
            tar -czf ~/mykernel/build/xiaofang-userland.tar.gz .
            cd ~/mykernel
            echo "userland: build/xiaofang-userland.tar.gz"
            ;;
        bare)
            clang --target=aarch64-none-elf -c ~/mykernel/bare/boot.s -o "$TMP/boot.o"
            clang --target=aarch64-none-elf -ffreestanding -fno-pic -mgeneral-regs-only -nostdlib -O2 -c ~/mykernel/bare/kernel.c -o "$TMP/kernel.o"
            ld.lld -m aarch64elf -T ~/mykernel/bare/linker.ld -o "$TMP/xiaofang-bare.elf" "$TMP/boot.o" "$TMP/kernel.o"
            llvm-objcopy -O binary "$TMP/xiaofang-bare.elf" "$TMP/xiaofang-bare.bin"
            mkdir -p "$TMP/root"
            cp "$TMP/xiaofang-bare.elf" "$TMP/root/"
            cp "$TMP/xiaofang-bare.bin" "$TMP/root/"
            cd "$TMP/root"
            tar -czf ~/mykernel/build/xiaofang-bare.tar.gz .
            cd ~/mykernel
            echo "bare: build/xiaofang-bare.tar.gz"
            ;;
        *)
            echo "未知目标: $t"
            ;;
    esac
    rm -rf "$TMP"
done
