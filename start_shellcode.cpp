#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

unsigned char shellcode[] = 
{
    0xeb, 0x1b, 0x5b, 0x48, 0xff, 0xc3, 0x48, 0x31,  0xc0, 0x48, 0xff, 0xc0, 0x48, 0x31, 0xff, 0x48,
    0xff, 0xc7, 0x48, 0x89, 0xde, 0x48, 0x31, 0xd2,  0xb2, 0x0e, 0x0f, 0x05, 0xc3, 0xe8, 0xe0, 0xff,
    0xff, 0xff, 0xc3, 0x48, 0x65, 0x6c, 0x6c, 0x6f,  0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21,
    0x0a
};

int main() {
    // Выделяем память для шеллкода
    void *exec_mem = mmap(
        NULL,
        sizeof(shellcode),
        PROT_READ | PROT_WRITE | PROT_EXEC, // Разрешаем выполнение
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (exec_mem == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // Копируем шеллкод в выделенную память
    memcpy(exec_mem, shellcode, sizeof(shellcode));

    // Выполняем шеллкод
    ((void(*)())exec_mem)();

    // Освобождаем память (опционально, если шеллкод завершится)
    munmap(exec_mem, sizeof(shellcode));

    return 0;
}
