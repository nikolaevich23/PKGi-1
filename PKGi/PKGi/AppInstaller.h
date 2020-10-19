#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>

#include <orbis/libkernel.h>
#include <orbis/SystemService.h>

#ifndef APPINSTALLER_H
#define APPINSTALLER_H

// Thanks to flatz
struct bgft_download_task_progress_info {
	unsigned int bits;
	int error_result;
	unsigned long length;
	unsigned long transferred;
	unsigned long length_total;
	unsigned long transferred_total;
	unsigned int num_index;
	unsigned int num_total;
	unsigned int rest_sec;
	unsigned int rest_sec_total;
	int preparing_percent;
	int local_copy_percent;
};

class AppInstaller
{
private:
	int(*sceBgftInitialize)(void* parameters);
	int(*sceBgftFinalize)();

	int(*sceAppInstUtilInitialize)();
	int(*sceAppInstUtilTerminate)();
	int(*sceAppInstUtilAppUnInstall)(const char* titleid);
	int(*sceAppInstUtilAppExists)(const char* titleid, int* flags);


public:
	AppInstaller();
	~AppInstaller();
};

#endif