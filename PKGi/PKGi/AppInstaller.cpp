#include "AppInstaller.h"

AppInstaller::AppInstaller() {
	uint32_t appinstall_module = sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx", 0, NULL, 0, NULL, NULL);
	uint32_t bgft_module = sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx", 0, NULL, 0, NULL, NULL);

	sceKernelDlsym(bgft_module, "sceBgftInitialize", (void**)&sceBgftInitialize);
	sceKernelDlsym(bgft_module, "sceBgftFinalize", (void**)&sceBgftFinalize);

	sceKernelDlsym(appinstall_module, "sceAppInstUtilInitialize", (void**)&sceAppInstUtilInitialize);
	sceKernelDlsym(appinstall_module, "sceAppInstUtilTerminate", (void**)&sceAppInstUtilTerminate);
	sceKernelDlsym(appinstall_module, "sceAppInstUtilAppUnInstall", (void**)&sceAppInstUtilAppUnInstall);
	sceKernelDlsym(appinstall_module, "sceAppInstUtilAppExists", (void**)&sceAppInstUtilAppExists);
}

AppInstaller::~AppInstaller() {
	sceBgftFinalize();
	sceAppInstUtilTerminate();
}