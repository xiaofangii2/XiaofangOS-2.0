#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "bare/xiaofang.elf";

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    struct stat st;
    fstat(fd, &st);

    unsigned char *img = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (img == MAP_FAILED) { perror("mmap img"); return 1; }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)img;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("not ELF\n");
        return 1;
    }
    if (eh->e_machine != EM_AARCH64) {
        printf("not aarch64\n");
        return 1;
    }

    printf("entry = 0x%lx, phnum = %d\n", (unsigned long)eh->e_entry, eh->e_phnum);

    /* 每个 PT_LOAD 单独 mmap 到指定 vaddr 附近, 这里简单做法:
       整体预留一大块, 从最低 vaddr 开始 */
    uint64_t min_v = UINT64_MAX, max_v = 0;
    Elf64_Phdr *ph = (Elf64_Phdr *)(img + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        if (ph[i].p_vaddr < min_v) min_v = ph[i].p_vaddr;
        uint64_t end = ph[i].p_vaddr + ph[i].p_memsz;
        if (end > max_v) max_v = end; if (max_v < 0x40020000) max_v = 0x40020000;
    }
    uint64_t span = max_v - min_v;
    printf("vaddr 0x%lx - 0x%lx, span %lu\n",
           (unsigned long)min_v, (unsigned long)max_v, (unsigned long)span);

    /* 在目标地址直接映射, 需要 MAP_FIXED, 但可能和现有映射冲突.
       先尝试 MAP_FIXED_NOREPLACE, 失败就换地方 */
    void *base = mmap((void *)min_v, span,
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                      -1, 0);
    if (base == MAP_FAILED) {
        perror("mmap fixed");
        printf("尝试非固定地址映射, 但跳转会失败(地址不匹配)\n");
        return 1;
    }
    printf("mapped at %p\n", base);

    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        memcpy((void *)ph[i].p_vaddr, img + ph[i].p_offset, ph[i].p_filesz);
        if (ph[i].p_memsz > ph[i].p_filesz) {
            memset((void *)(ph[i].p_vaddr + ph[i].p_filesz), 0,
                   ph[i].p_memsz - ph[i].p_filesz);
        }
        printf("loaded seg vaddr=0x%lx filesz=%lu memsz=%lu\n",
               (unsigned long)ph[i].p_vaddr,
               (unsigned long)ph[i].p_filesz,
               (unsigned long)ph[i].p_memsz);
    }

    printf("jumping to 0x%lx\n", (unsigned long)eh->e_entry);
    fflush(stdout);

    void (*entry)(void) = (void (*)(void))eh->e_entry;
    entry();

    printf("returned (不应该到这里)\n");
    return 0;
}
