.section .text.boot
.global _start
_start:
    ldr x0, =_stack_top
    mov sp, x0
    bl kmain
1:  b 1b
