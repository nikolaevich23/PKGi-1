#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>

#ifndef APPINSTALLER_H
#define APPINSTALLER_H

#define BGFT_HEAP_SIZE (1 * 1024 * 1024)

struct _SceBgftInitParams {
	void* heap;
	size_t heapSize;
};
typedef struct _SceBgftInitParams SceBgftInitParams;

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

enum _SceBgftTaskSubType {
	SCE_BGFT_TASK_SUB_TYPE_UNKNOWN = 0,
	SCE_BGFT_TASK_SUB_TYPE_PHOTO = 1,
	SCE_BGFT_TASK_SUB_TYPE_MUSIC = 2,
	SCE_BGFT_TASK_SUB_TYPE_VIDEO = 3,
	SCE_BGFT_TASK_SUB_TYPE_MARLIN_VIDEO = 4,
	SCE_BGFT_TASK_SUB_TYPE_UPDATA_ORBIS = 5,
	SCE_BGFT_TASK_SUB_TYPE_GAME = 6,
	SCE_BGFT_TASK_SUB_TYPE_GAME_AC = 7,
	SCE_BGFT_TASK_SUB_TYPE_GAME_PATCH = 8,
	SCE_BGFT_TASK_SUB_TYPE_GAME_LICENSE = 9,
	SCE_BGFT_TASK_SUB_TYPE_SAVE_DATA = 10,
	SCE_BGFT_TASK_SUB_TYPE_CRASH_REPORT = 11,
	SCE_BGFT_TASK_SUB_TYPE_PACKAGE = 12,
	SCE_BGFT_TASK_SUB_TYPE_MAX = 13,
};
typedef enum _SceBgftTaskSubType SceBgftTaskSubType;

enum _SceBgftTaskOpt {
	SCE_BGFT_TASK_OPT_NONE = 0x0,
	SCE_BGFT_TASK_OPT_DELETE_AFTER_UPLOAD = 0x1,
	SCE_BGFT_TASK_OPT_INVISIBLE = 0x2,
	SCE_BGFT_TASK_OPT_ENABLE_PLAYGO = 0x4,
	SCE_BGFT_TASK_OPT_FORCE_UPDATE = 0x8,
	SCE_BGFT_TASK_OPT_REMOTE = 0x10,
	SCE_BGFT_TASK_OPT_COPY_CRASH_REPORT_FILES = 0x20,
	SCE_BGFT_TASK_OPT_DISABLE_INSERT_POPUP = 0x40,
	SCE_BGFT_TASK_OPT_INTERNAL = 0x80, /* ignores release date */
	SCE_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM = 0x10000,
};
typedef enum _SceBgftTaskOpt SceBgftTaskOpt;

struct _SceBgftDownloadParam {
	int userId;
	int entitlementType;
	const char* id; /* max size = 0x30 */
	const char* contentUrl; /* max size = 0x800 */
	const char* contentExUrl;
	const char* contentName; /* max size = 0x259 */
	const char* iconPath; /* max size = 0x800 */
	const char* skuId;
	SceBgftTaskOpt option;
	const char* playgoScenarioId;
	const char* releaseDate;
	const char* packageType;
	const char* packageSubType;
	unsigned long packageSize;
};
typedef struct _SceBgftDownloadParam SceBgftDownloadParam;

struct _SceBgftDownloadParamEx {
	SceBgftDownloadParam params;
	unsigned int slot;
};
typedef struct _SceBgftDownloadParamEx SceBgftDownloadParamEx;

struct _SceBgftDownloadTaskInfo {
	char* contentTitle;
	char* iconPath;
	unsigned long notificationUtcTick;
};
typedef struct _SceBgftDownloadTaskInfo SceBgftDownloadTaskInfo;

class AppInstaller
{
private:
	Application* App;

	int(*sceBgftInitialize)(SceBgftInitParams* parameters);
	int(*sceBgftDownloadRegisterTask)(SceBgftDownloadParam* params, int* taskId);
	int(*sceBgftDownloadStartTask)(int taskId);
	int(*sceBgftFinalize)();

	int(*sceAppInstUtilInitialize)();
	int(*sceAppInstUtilTerminate)();
	int(*sceAppInstUtilAppUnInstall)(const char* titleid);
	int(*sceAppInstUtilAppExists)(const char* titleid, int* flags);
public:
	AppInstaller(Application* app);
	~AppInstaller();

	int RegisterDownload(const char* url, int id);
};

#endif