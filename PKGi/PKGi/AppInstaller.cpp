#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "App.h"
#include "Network.h"
#include "Pkg.h"
#include "Sfo.h"
#include "AppInstaller.h"

#define SCE_SYSMODULE_INTERNAL_APPINSTUTIL 0x80000014
#define SCE_SYSMODULE_INTERNAL_BGFT 0x8000002A

static SceBgftInitParams bgft_initialization_param;

AppInstaller::AppInstaller(Application* app) {
	App = app;

	uint32_t appinstall_module = sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx", 0, NULL, 0, NULL, NULL); // sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL);
	uint32_t bgft_module = sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx", 0, NULL, 0, NULL, NULL); // sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);

	printf("Bgft: 0x%08x\n", bgft_module);
	printf("AppInstUtil: 0x%08x\n", appinstall_module);

	if (bgft_module <= 0 || appinstall_module <= 0) {
		printf("AppInstaller Module cannot be loaded !\n");
		printf("Module Error\n");
		return;
	}

	sceKernelDlsym(bgft_module, "sceBgftServiceInit", (void**)&sceBgftInitialize);
	sceKernelDlsym(bgft_module, "sceBgftServiceTerm", (void**)&sceBgftFinalize);
	sceKernelDlsym(bgft_module, "sceBgftServiceIntDebugDownloadRegisterPkg", (void**)&sceBgftDownloadRegisterTask);
	sceKernelDlsym(bgft_module, "sceBgftServiceDownloadStartTask", (void**)&sceBgftDownloadStartTask);

	sceKernelDlsym(appinstall_module, "sceAppInstUtilInitialize", (void**)&sceAppInstUtilInitialize);
	sceKernelDlsym(appinstall_module, "sceAppInstUtilTerminate", (void**)&sceAppInstUtilTerminate);
	sceKernelDlsym(appinstall_module, "sceAppInstUtilAppUnInstall", (void**)&sceAppInstUtilAppUnInstall);
	sceKernelDlsym(appinstall_module, "sceAppInstUtilAppExists", (void**)&sceAppInstUtilAppExists);

	printf("sceBgftInitialize: %p\n", sceBgftInitialize);
	printf("sceAppInstUtilInitialize: %p\n", sceAppInstUtilInitialize);

	if (!sceBgftInitialize || !sceAppInstUtilInitialize) {
		printf("!!!WARNING!!!: DLSYM ERROR\n");
		return;
	}

	memset(&bgft_initialization_param, 0, sizeof(bgft_initialization_param));
	{
		bgft_initialization_param.heapSize = BGFT_HEAP_SIZE;
		bgft_initialization_param.heap = (uint8_t*)malloc(bgft_initialization_param.heapSize);
		if (!bgft_initialization_param.heap) {
			printf("AppInstaller:: Error during malloc(1)\n");
			return;
		}

		memset(bgft_initialization_param.heap, 0, bgft_initialization_param.heapSize);
	}

	int ret = sceBgftInitialize(&bgft_initialization_param);
	if (ret) {
		printf("AppInstaller: sceBgftInitialize failed: 0x%08X\n", ret);
		return;
	}
}

AppInstaller::~AppInstaller() {
	sceBgftFinalize();
	sceAppInstUtilTerminate();
}

int AppInstaller::RegisterDownload(const char* url, int id) {
	Pkg* package = new Pkg();

	char package_url[500];
	char referer_url[500];
	char icon_url[500];

	snprintf(package_url, 500, "%s/pkgs/%i.pkg", url, id);
	snprintf(referer_url, 500, "%s/refs/%i.json", url, id);
	snprintf(icon_url, 500, "%s/icons/%i.png", url, id);

	// Get the Foreground user
	int userId;
	int err = sceUserServiceGetForegroundUser(&userId);
	if (err) {
		printf("sceUserServiceGetForegroundUser: error 0x%08x\n", err);
		delete package;
		return -1;
	}

	printf("Foreground: %i\n", userId);

	// Download & Load the PKG Header
	printf("Downloading PKG Header ...\n");

	size_t size = 0;
	void* header_data = App->Net->GetChunkedRequest(package_url, 0, sizeof(struct pkg_header), &size);

	if (!header_data || size <= 0) {
		printf("RegisterDownload: Unable to download pkg header. (data: %p size: %lu)\n", header_data, size);
		return -2;
	}

	printf("Loading PKG Header ...\n");

	if (!package->LoadHeader(header_data, size)) {
		printf("RegisterDownload: Unable to load header\n");
		free(header_data);
		return -3;
	}

	free(header_data);

	printf("Finding PKG Entries Table ...\n");

	// Download & Load the Entries Table
	uint64_t entries_offset = 0;
	size_t entries_count = 0;
	package->getEntriesPosition((uint32_t*)&entries_offset, &entries_count);

	size_t entries_total_size = entries_count * sizeof(struct pkg_table_entry);

	printf("Offset: 0x%016lx ...\n", entries_offset);
	printf("Count: 0x%016zx ...\n", entries_count);
	printf("Size: 0x%016zx ...\n", entries_total_size);

	printf("Downloading PKG Entries Table ...\n");

	size = 0;
	void* entries_data = App->Net->GetChunkedRequest(package_url, entries_offset, entries_total_size, &size);

	if (!entries_data || size < entries_total_size) {
		printf("RegisterDownload: Unable to download pkg entries. (data: %p size: %lu)\n", entries_data, size);
		delete package;
		return -4;
	}

	printf("Loading PKG Entries Table ...\n");

	if (!package->LoadEntries(entries_data, entries_count)) {
		printf("RegisterDownload: Error during the entries load.\n");
		free(entries_data);
		delete package;
		return -5;
	}

	free(entries_data);

	printf("Finding PKG SFO Entry ...\n");

	// Download & Load the SFO
	uint64_t sfo_offset = 0;
	size_t sfo_size = 0;
	if (!package->FindEntries(pkg_entry_id::PKG_ENTRY_ID__PARAM_SFO, (uint32_t*)&sfo_offset, &sfo_size)) {
		printf("RegisterDownload: Unable to find position of sfo file.\n");
		delete package;
		return -6;
	}

	if (!sfo_offset || sfo_size <= 0) {
		printf("RegisterDownload: Invalid sfo position. (data: 0x%08lx size: %lu)\n", sfo_offset, sfo_size);
		delete package;
		return -6;
	}

	printf("Downloading SFO Entry ...\n");

	size = 0;
	void* sfo_data = App->Net->GetChunkedRequest(package_url, sfo_offset, sfo_size, &size);

	if (!sfo_data || size <= 0) {
		printf("RegisterDownload: Unable to download sfo data. (data: %p size: %lu)\n", sfo_data, size);
		return -7;
	}

	Sfo* sfo = new Sfo(sfo_data, size);
	free(sfo_data);

	// Read inside the SFO and get neccesary
	char titleid[150];
	err = sfo->getSfoString("TITLE_ID", titleid, 150);
	if (err < 0) {
		printf("RegisterDownload: SFO Read error(1) ! (%i)\n", err);
	}
	printf("RegisterDownload: TITLE_ID: %s\n", titleid);

	char title[150];
	err = sfo->getSfoString("TITLE", title, 150);
	if (err < 0) {
		printf("RegisterDownload: SFO Read error(2) ! (%i)\n", err);
	}
	printf("RegisterDownload: TITLE: %s\n", title);

	char contentid[150];
	err = sfo->getSfoString("CONTENT_ID", contentid, 150);
	if (err < 0) {
		printf("RegisterDownload: SFO Read error(3) ! (%i)\n", err);
	}
	printf("RegisterDownload: CONTENT_ID: %s\n", contentid);

	delete sfo;

	// Get package digest, type and size
	const char* package_type = NULL;
	switch (package->getContentType()) {
		case PKG_CONTENT_TYPE_GD: package_type = "PS4GD"; break;
		case PKG_CONTENT_TYPE_AC: package_type = "PS4AC"; break;
		case PKG_CONTENT_TYPE_AL: package_type = "PS4AL"; break;
		case PKG_CONTENT_TYPE_DP: package_type = "PS4DP"; break;
		default:
			package_type = NULL;
			printf("Unsupported content type !");
			delete package;
			return -8;
			break;
	}

	printf("Package type: %s\n", package_type);

	size_t package_size = package->getPackageSize();

	char packageDigest[(PKG_DIGEST_SIZE * 2) + 1] = { 0 };
	package->getPackageDigest(packageDigest, sizeof(packageDigest));

	char pieceDigest[(PKG_MINI_DIGEST_SIZE * 2) + 1] = { 0 };
	package->getPieceDigest(pieceDigest, sizeof(pieceDigest));

	printf("packageDigest: %s\n", packageDigest);
	printf("pieceDigest: %s\n", pieceDigest);

	// Prepare the ref url
	char package_ref[500];
	snprintf(package_ref, 500, "%s/refs/%i.json", url, id);

	// Prepare the register of the download
	SceBgftDownloadParam taskParams;
	int taskId;

	// Tous marche avec sa
	// Mais il veut absolument un .json et un .png
	// Trouvé une solution
	// Solution simple imaginé : 
	// 1.Requete avec les information, récupération du nom "tmp" et utilisation
	// 2. Ont ré-imagine le systéme est les json sont pré-généré pour le téléchargement (sa peut se faire assez simplement)

	memset(&taskParams, 0, sizeof(taskParams));
	{
		taskParams.entitlementType = 5; /* TODO: figure out */
		taskParams.id = contentid;
		taskParams.contentUrl = referer_url;
		taskParams.contentName = title;
		taskParams.iconPath = icon_url;
		taskParams.playgoScenarioId = "0";
		taskParams.option = SCE_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM;
		taskParams.packageType = package_type;
		taskParams.packageSize = package_size;
	}

	err = sceBgftDownloadRegisterTask(&taskParams, &taskId);
	if (err < 0) {
		printf("Error during task creation (1) ! (taskID: %i ret: 0x%08x)\n", taskId, err);
		if (err == 0x80990088) {
			err = - 7; // Already installed
		}
		else{
			err = -8;
		}

		delete package;
		return err;
	}

	printf("New task was created ! (taskID: %i ret: 0x%08x)\n", taskId, err);

	err = sceBgftDownloadStartTask(taskId);
	if (err < 0) {
		printf("Error during task launch ! (taskID: %i ret: 0x%08x)\n", taskId, err);
		delete package;
		return -9;
	}

	printf("Installation launched.\n");
	delete package;
	return 0;
}
