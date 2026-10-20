.section .text.boot
.global _start
_start:
    ldr x0, =_stack_top
    mov sp, x0
    bl kmain
    mov x0, #0
    mov x8, #93
    svc #0
1:  b 1b
