#include <sys/mman.h>
#include <iostream>
#include <pthread.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sys/sysinfo.h>
#include <thread>

const char* debuggers[] = {
	// Debuggers
	"gdb",
	"OllyDbg",
	"SoftICE",
	"Valgrind",
	"strace",
	// Sandbox
	"VBoxClient"
};

const char* sandboxModules[] = {
	"sandboxmodule.so"
};

void* Mannequin(void*)
{
	while(true) { usleep(100000); }
	return nullptr;
}

bool checkDebug() {
//---------------------------------------Check debug-------------------------------------
	// Проверка переменных окружения
	if (getenv("LD_PRELOAD") || getenv("LD_DEBUG"))
		return true;

	// Проверка трайсировки
	if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
		return true;

	// Проверка задержки выполнения функций
	{
		pthread_t thread = 0;
		pthread_attr_t attr = {};

		time_t time1 = time(0);
		pthread_create(&thread, &attr, &Mannequin, nullptr);
		time_t time2 = time(0);
		if(time2 - time1 != 0)
		{
			pthread_detach(thread);
			return true;
		}

		time1 = time(0);
		pthread_detach(thread);
		time2 = time(0);

		if(time2 - time1 != 0)
			return true;

		time1 = time(0);
		connect(0, 0, 0);
		time2 = time(0);
		if(time2 - time1 != 0)
			return true;
	}
	
	// Просмотр списка процессов
	{
		FILE *f = popen("ps aux", "r");
		if (!f)
		{
			char buffer[256];
			while (fgets(buffer, sizeof(buffer), f)) 
			{
				for(const char* i : debuggers)
				{
					if (strcmp(buffer, i)) {
						return true;
					}
				}
				for(int j = 0; j < 256; ++j)
					buffer[j] = 0;
			}
		}
	}

//---------------------------------------Check sandbox-----------------------------------
	// Проверка загруженных модулей
	{
		std::ifstream mapsFile("/proc/self/maps");
		if (mapsFile.is_open()) {

			std::string line;
			while (std::getline(mapsFile, line)) {
				for(const char* i : sandboxModules)
				{
					if (line.find(i) != std::string::npos) {
						mapsFile.close();
						return true;
					}
				}
			}
			mapsFile.close();
		}
	}
	
	// Проверка объёма оперативной памяти
	{
		struct sysinfo info{};
		sysinfo(&info);
		if(info.totalram * info.mem_unit / 1024 / 1024 / 1024 < 8)
			return true;
	}
	
	// Проверка кол-ва ядер процессора
	{
		 unsigned int coreCount = std::thread::hardware_concurrency();
		 if(coreCount < 4)
		 	return true;
	}
	
	return false;
}

unsigned char shellcode[] = 
{
    0xeb, 0x19, 0x5b, 0x48, 0xff, 0xc3, 0xb8, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x01, 0x00, 0x00, 0x00,
    0x48, 0x89, 0xde, 0xba, 0x0e, 0x00, 0x00, 0x00, 0x0f, 0x05, 0xc3, 0xe8, 0xe2, 0xff, 0xff, 0xff,
    0xc3, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a
};

int main() {
	if(checkDebug())
	{
		std::cout << "Любопытной Варваре на базаре нос оторвали :)\n";
		return 0;
	}

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
