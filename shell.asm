BITS 64

jmp short one

two:
	pop rbx
	inc rbx
	
	mov rax, 1        ; write(
	mov rdi, 1        ;   STDOUT_FILENO,
	mov rsi, rbx      ;   "Hello, world!\n",
	mov rdx, 14       ;   sizeof("Hello, world!\n")
	syscall           ; );

	ret
	

one:
	call two
	ret
	db 'Hello, world!', 10
