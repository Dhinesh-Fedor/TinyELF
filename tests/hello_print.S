.global _start
.text
_start:
    mov $1, %rax
    mov $1, %rdi
    mov $msg, %esi
    mov $(msg_end - msg), %edx
    syscall

    mov $60, %rax
    xor %rdi, %rdi
    syscall

.data
msg: .ascii "hello from tinyelf\n"
msg_end:
