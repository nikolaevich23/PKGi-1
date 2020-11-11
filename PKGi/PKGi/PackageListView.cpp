#include "PackageListView.h"
#include "App.h"
#include "Graphics.h"
#include "Controller.h"
#include "Resource.h"
#include "Network.h"
#include "TinyJson.h"
#include "Utility.h"
#include "AppInstaller.h"

#include "SourcesView.h"

// Load resource for the view
PackageListView::PackageListView(Application* app, PackageListArg* args)
{
	int rc;
	App = app;
	if (!App) { printf("[ERROR] App is null !\n"); return; }
	if (!args) { printf("[ERROR] Args is null !\n"); return; }

	memcpy(&source, args, sizeof(PackageListArg));

	// Setup variable
	pkgs = nullptr;
	pkgNbr = 0;
	pkgSelected = 0;
	currentPage = 1;
	totalPage = 0;
	totalPkgCount = 0;
	memset(searchTerm, 0, 255);
	memset(errorMessage, 0, 255);
	onError = false;

	fetchStepAnim = 0;
	fetchTimeStep = 0;

	scePthreadMutexInit(&fetch_mtx, NULL, "PKGiFetchMTX");

	// Download first page
	FetchPackages(MENU_NBR_PER_PAGE, 1, false);

	// Set colors
	bgColor = { 0, 0, 0, 255 };
	fgColor = { 255, 255, 255, 255 };
}

// Unload resource
PackageListView::~PackageListView()
{
	// Cleanup memory
	scePthreadMutexLock(&fetch_mtx);
	if (pkgs) {
		for (int i = 0; i < pkgNbr; i++) {
			if (pkgs[i].icon.img) {
				App->Graph->unloadPNG(&pkgs[i].icon);
			}
		}

		free(pkgs);
	}
	scePthreadMutexUnlock(&fetch_mtx);

	// Destroy mutex
	scePthreadMutexDestroy(&fetch_mtx);
}

// Download packages list
void PackageListView::DownloadPkgList(int line, int page, char* search, bool is_up) {
	printf("FetchPkgList called for page %i (%i line) !\n", page, line);

	char url[500];

	if (strlen(search) <= 0) {
		snprintf(url, 500, "%s/packages.php?line=%i&page=%i", source.api_url, line, page);
	}
	else {
		snprintf(url, 500, "%s/packages.php?line=%i&page=%i&search=%s", source.api_url, line, page, search);
	}

	printf("URL: %s\n", url);

	scePthreadMutexLock(&fetch_mtx);
	printf("FetchPkgList: Cleanup old data ...\n");

	if (pkgs) {
		for (int i = 0; i < pkgNbr; i++) {
			if (pkgs[i].icon.img) {
				App->Graph->unloadPNG(&pkgs[i].icon);
			}
		}

		free(pkgs);
		pkgs = nullptr;
	}

	printf("FetchPkgList: Downloading data ...\n");

	size_t data_len = 0;
	char* data = (char*)App->Net->GetRequest(url, &data_len);
	if (!data) {
		printf("FetchPkgList: Unable to got data from source\n");
		ShowError("Network Error: Unable to got data from source", false);
		scePthreadMutexUnlock(&fetch_mtx);
		return;
	}

	printf("FetchPkgList -> Data: %s\n", data);

	json_t mem[100];
	json_t const* json = json_create(data, mem, 100);
	if (!json) {
		printf("FetchPkgList: Unable to load json\n");
		ShowError("Network Error: Invalid JSON Format", false);
		scePthreadMutexUnlock(&fetch_mtx);
		return;
	}

	// Extract useful data
	json_t const* total = json_getProperty(json, "total");
	if (!total || JSON_INTEGER != json_getType(total)) {
		printf("FetchPkgList: Unable to get total number of packages\n");
		ShowError("Network Error: Total number not found on response", false);
		scePthreadMutexUnlock(&fetch_mtx);
		return;
	}

	currentPage = page;
	totalPkgCount = (int)json_getInteger(total);
	printf("totalPkgCount: %i\n", totalPkgCount);
	printf("line: %i\n", line);

	if (totalPkgCount == 0 || line == 0)
		totalPage = 1;
	else
		totalPage = totalPkgCount / line + (totalPkgCount % line > 0);

	printf("totalPage: %i\n", totalPage);

	// Extract package list

	json_t const* packagesList = json_getProperty(json, "packages");
	if (!packagesList || JSON_ARRAY != json_getType(packagesList)) {
		printf("FetchPkgList: Unable to find the packages list\n.");
		ShowError("Network Error: Package list not found on response", false);
		scePthreadMutexUnlock(&fetch_mtx);
		return;
	}

	json_t const* package;

	pkgNbr = 0;
	for (package = json_getChild(packagesList); package != 0; package = json_getSibling(package)) {
		if (JSON_OBJ == json_getType(package)) {
			pkgNbr++;
		}
	}

	if (pkgNbr == 0) {
		printf("FetchPkgList: No packages available !\n");
		scePthreadMutexUnlock(&fetch_mtx);
		return;
	}

	pkgs = (Package*)malloc(pkgNbr * sizeof(Package));
	if (!pkgs) {
		printf("FetchPkgList: Unable to malloc packages list !\n");
		ShowError("Memory Error: Unable to malloc packages list !", false);

		pkgNbr = 0;
		scePthreadMutexUnlock(&fetch_mtx);
		return;
	}

	memset(pkgs, 0, pkgNbr * sizeof(Package));

	int i = 0;
	for (package = json_getChild(packagesList); package != 0; package = json_getSibling(package)) {
		if (JSON_OBJ == json_getType(package)) {
			char const* packageName = json_getPropertyValue(package, "name");
			if (packageName) {
				strncpy(pkgs[i].name, packageName, 255);
			}

			json_t const* id = json_getProperty(package, "id");
			if (!id || JSON_INTEGER != json_getType(id)) {
				pkgs[i].id = -1;
			}
			else {
				pkgs[i].id = (int)json_getInteger(id);
			}

			char icon_url[500];
			snprintf(icon_url, 500, "%s/icons/%i.png", source.api_url, pkgs[i].id);

			printf("FetchPkgList: Downloading icon ...\nURL: %s\n", icon_url);

			size_t icon_len = 0;
			unsigned char* icon_data = (unsigned char*)App->Net->GetRequest(icon_url, &icon_len);
			printf("Data: %p Size: %i\n", icon_data, (int)icon_len);

			if (icon_data) {
				App->Graph->loadPNGFromMemory(&pkgs[i].icon, icon_data, (int)icon_len);
				free(icon_data);
			}

			i++;
		}
	}

	if (is_up) {
		pkgSelected = pkgNbr - 1;
	}
	else {
		pkgSelected = 0;
	}

	free(data);

	printf("FetchPkgList: Downloaded.\n");

	scePthreadMutexUnlock(&fetch_mtx);
}

// Thread of fetch
void PackageListView::FetchPackages_Thread(FetchArg* args) {
	printf("FetchPackages_Thread: Thread launched !\n");

	if (args) {
		printf("FetchPackages_Thread: args ok\n");

		args->MView->DownloadPkgList(args->line, args->page, args->searchTerm, args->is_up);
		free(args);
	}

	printf("FetchPackages_Thread: scePthreadExit\n");
	scePthreadExit(nullptr);
}

// Thread initialization
void PackageListView::FetchPackages(int line, int page, bool is_up) {
	FetchArg* args = (FetchArg*)malloc(sizeof(FetchArg));
	args->MView = this;
	args->line = line;
	args->page = page;
	args->is_up = is_up;

	strncpy(args->searchTerm, searchTerm, SEARCH_LEN);

	OrbisPthreadAttr attr;
	scePthreadAttrInit(&attr);
	scePthreadCreate(&fetch_thread, &attr, (void*)FetchPackages_Thread, (void*)args, "PKGiFetchThread");

	printf("FetchPackages: Creating thread ...\n");
}

// Show a message if error occurs
void PackageListView::ShowError(const char* error, bool dontReturn) {
	snprintf(errorMessage, sizeof(errorMessage), "%s", error);
	onError = true;
	ErrordontReturn = dontReturn;
}

void PackageListView::ActualiseList(int page) {
	pkgSelected = 0;
	FetchPackages(MENU_NBR_PER_PAGE, page, false);
	onError = false;
}

int PackageListView::Update() {
	// If mutex is locked, fetch is in progress
	if (scePthreadMutexTrylock(&fetch_mtx) >= 0) {
		if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE)) {
			if (ErrordontReturn) {
				onError = false;
				ErrordontReturn = false;
			}
			else {
				if (strlen(searchTerm) > 0) {
					memset(searchTerm, 0, SEARCH_LEN);

					currentPage = 1;
					totalPage = 0;
					totalPkgCount = 0;
					ActualiseList(0);
				}
				else {
					SourcesView* src_view = new SourcesView(App);
					App->ChangeView(src_view);
					scePthreadMutexUnlock(&fetch_mtx);
					delete this;
					return 0;
				}
			}
		}

		if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_SQUARE)) {
			ActualiseList(currentPage);
		}

		else {
			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_UP)) {
				if ((pkgSelected - 1) >= 0) {
					pkgSelected--;
				}
				else if ((pkgSelected - 1) < 0 && currentPage > 1) {
					FetchPackages(MENU_NBR_PER_PAGE, currentPage - 1, true);
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_DOWN)) {
				if ((pkgSelected + 1) < pkgNbr) {
					pkgSelected++;
				}
				else if ((pkgSelected + 1) >= pkgNbr && currentPage < totalPage) {
					FetchPackages(MENU_NBR_PER_PAGE, currentPage + 1, false);
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
				if (pkgs && pkgSelected >= 0) {
					printf("Request download for %s (id: %i)\n", source.api_url, pkgs[pkgSelected].id);
					int install_err = App->AppInst->RegisterDownload(source.api_url, pkgs[pkgSelected].id);
					if (install_err < 0) {
						switch (install_err) {
							case -7:
								ShowError("This package is already installed", true);
								break;
							default:
								char nfErr[255] = { 0 };
								snprintf(nfErr, sizeof(nfErr), "Installation failed (%i)", install_err);
								ShowError(nfErr, true);
								break;
						}
					}
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
				char* search = Utility::OpenKeyboard(ORBIS_TYPE_DEFAULT, "Enter search keywords.");
				if (search) {
					strncpy(searchTerm, search, SEARCH_LEN);
					free(search);

					// Actualise search
					ActualiseList(0);
				}
			}
		}

		// Unlock the mutex
		scePthreadMutexUnlock(&fetch_mtx);
	}

	return 0;
}

int PackageListView::Render() {
	int ScreenHeight = App->Graph->GetScreenHeight();
	int ScreenWidth =  App->Graph->GetScreenWidth();

	// Setup color
	Color notActive_color = { 0xA6, 0xA6, 0xA6, 0xFF };
	Color active_color = { 0x05, 0x2F, 0x7E, 0xFF };
	Color buttonText_color = { 0x0, 0x0, 0x0, 0xFF };
	Color green = { 0x0, 0xFF, 0x00, 0xFF };
	Color white = { 0xFF, 0xFF, 0xFF, 0xFF };
	Color dark = { 0x0, 0x00, 0x00, 0xFF };
	Color grey = { 0x40, 0x40, 0x40, 0xFF };
	Color orange = { 0xE3, 0x8A, 0x1E, 0xFF };

	// Calculate drawable package
	int menu_x = BORDER_X;
	int menu_y = HEADER_SIZE + MENU_BORDER;
	int menu_y_end = ScreenHeight - FOOTER_SIZE;

	int menu_size_height = menu_y_end - menu_y;
	int menu_size_width = ScreenWidth - (BORDER_X * 2);

	int consumable_border = (MENU_BORDER * (MENU_NBR_PER_PAGE - 2));
	int pkg_line_height = (menu_size_height - consumable_border) / MENU_NBR_PER_PAGE;

	int delta = pkgSelected / MENU_NBR_PER_PAGE;
	int maxPage = pkgNbr / MENU_NBR_PER_PAGE;

	char currentPageStr[255];
	sprintf(currentPageStr, "Page %i / %i", currentPage, totalPage);

	// Draw logo
	int logo_y = (HEADER_SIZE / 2) - (120 / 2);
	App->Graph->drawPNG(&App->Res->logo, BORDER_X, logo_y);

	// Draw source name
	FontSize sourceNameSize;
	App->Graph->setFontSize(App->Res->robotoFont, 54);

	if (strlen(searchTerm) > 0) {
		char searchTitle[500];
		snprintf(searchTitle, sizeof(searchTitle), "Search for: %s", searchTerm);

		App->Graph->getTextSize(searchTitle, App->Res->robotoFont, &sourceNameSize);
		App->Graph->drawText(searchTitle, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, (logo_y + ((120 / 2) - (sourceNameSize.height / 2))), dark, white);
	}
	else {
		App->Graph->getTextSize(source.title, App->Res->robotoFont, &sourceNameSize);
		App->Graph->drawText(source.title, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, (logo_y + ((120 / 2) - (sourceNameSize.height / 2))), dark, white);
	}

	App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

	// Draw header border
	App->Graph->drawRectangle(BORDER_X, HEADER_SIZE - 5, ScreenWidth - (BORDER_X * 2), 5, white);

	// If mutex is locked, fetch is in progress
	if (scePthreadMutexTrylock(&fetch_mtx) < 0) {
		char progress[255] = "Fetching packages list .";

		uint64_t anim_time = sceKernelGetProcessTime();

		// stepAnim go from 0 to x each seconds
		if ((fetchTimeStep + 1000000) <= anim_time) {
			fetchTimeStep = anim_time;
			fetchStepAnim++;
			if (fetchStepAnim > 3)
				fetchStepAnim = 0;
		}

		if (fetchStepAnim == 0)
			snprintf(progress, 255, "%s.", progress);
		else if (fetchStepAnim == 1)
			snprintf(progress, 255, "%s..", progress);
		else if (fetchStepAnim == 2)
			snprintf(progress, 255, "%s...", progress);

		// Show the progress bar
		FontSize progressSize;
		App->Graph->getTextSize(progress, App->Res->robotoFont, &progressSize);

		int text_x = ((ScreenWidth / 2) - (progressSize.width / 2));
		int text_y = ((ScreenHeight / 2) - (progressSize.height / 2));
		App->Graph->drawText(progress, App->Res->robotoFont, text_x, text_y, dark, white);

		// Draw footer background and border
		App->Graph->drawRectangle(0, ScreenHeight - FOOTER_SIZE, ScreenWidth, FOOTER_SIZE, dark);
		App->Graph->drawRectangle(BORDER_X, ScreenHeight - FOOTER_SIZE + 5, ScreenWidth - (BORDER_X * 2), 5, white);
	}
	else {
		fetchStepAnim = 0;
		fetchTimeStep = 0;

		// Mutex now locked, draw packages list
		if (onError) {
			FontSize errorSize;
			App->Graph->getTextSize(errorMessage, App->Res->robotoFont, &errorSize);
			App->Graph->drawText(errorMessage, App->Res->robotoFont, ((ScreenWidth / 2) - (errorSize.width / 2)), ((ScreenHeight / 2) - (errorSize.height / 2)), dark, white);
		} else if (pkgs) {
			// Draw packages
			for (int posMenu = 0; posMenu < MENU_NBR_PER_PAGE; posMenu++) {
				int currentPkg = (delta * MENU_NBR_PER_PAGE) + posMenu;
				if (currentPkg >= pkgNbr) {
					break;
				}

				Color selection = grey;
				if (pkgSelected == currentPkg) {
					selection = orange;
				}

				FontSize titleSize;
				App->Graph->getTextSize(pkgs[currentPkg].name, App->Res->robotoFont, &titleSize);

				int current_menu_y = menu_y + (posMenu * pkg_line_height) + (posMenu * MENU_BORDER);
				App->Graph->drawRectangle(menu_x, current_menu_y, menu_size_width, pkg_line_height, selection);
				

				int icon_size = pkg_line_height - (2 * MENU_IN_IMAGE_BORDER);

				if (pkgs[currentPkg].icon.img)
					App->Graph->drawSizedPNG(&pkgs[currentPkg].icon, menu_x + MENU_IN_IMAGE_BORDER, current_menu_y + MENU_IN_IMAGE_BORDER, icon_size, icon_size);
				else
					App->Graph->drawSizedPNG(&App->Res->unknown, menu_x + MENU_IN_IMAGE_BORDER, current_menu_y + MENU_IN_IMAGE_BORDER, icon_size, icon_size);

				App->Graph->drawText(pkgs[currentPkg].name, App->Res->robotoFont, menu_x + icon_size + (3 * MENU_IN_IMAGE_BORDER), current_menu_y + ((pkg_line_height / 2) - (titleSize.height / 2)), selection, white);
			}
		}
		else{
			FontSize ohnoSize;
			char ohno[255] = "No package available :(";
			App->Graph->getTextSize(ohno, App->Res->robotoFont, &ohnoSize);
			App->Graph->drawText(ohno, App->Res->robotoFont, ((ScreenWidth / 2) - (ohnoSize.width / 2)), ((ScreenHeight / 2) - (ohnoSize.height / 2)), dark, white);
		}

		// Draw footer background and border
		App->Graph->drawRectangle(0, ScreenHeight - FOOTER_SIZE, ScreenWidth, FOOTER_SIZE, dark);
		App->Graph->drawRectangle(BORDER_X, ScreenHeight - FOOTER_SIZE + 5, ScreenWidth - (BORDER_X * 2), 5, white);

		App->Graph->setFontSize(App->Res->robotoFont, FOOTER_TEXT_SIZE);

		if (onError) {
			char returnText[255] = "Return";
			FontSize returnSize;
			App->Graph->getTextSize(returnText, App->Res->robotoFont, &returnSize);

			int icon_width = returnSize.width + FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;

			int icon_x = (ScreenWidth - BORDER_X) - icon_width;
			int icon_y = (ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
			int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (returnSize.height / 2));

			App->Graph->drawSizedPNG(&App->Res->circle, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(returnText, App->Res->robotoFont, icon_x, text_y, dark, white);
		}
		else {
			char returnMenu[255] = "Return";
			char select[255] = "Select";
			char refresh[255] = "Refresh";
			char search[255] = "Search";

			FontSize returnMenuSize;
			FontSize selectSize;
			FontSize refreshSize;
			FontSize searchSize;

			App->Graph->getTextSize(returnMenu, App->Res->robotoFont, &returnMenuSize);
			App->Graph->getTextSize(select, App->Res->robotoFont, &selectSize);
			App->Graph->getTextSize(refresh, App->Res->robotoFont, &refreshSize);
			App->Graph->getTextSize(search, App->Res->robotoFont, &searchSize);

			int icon_width = (selectSize.width + returnMenuSize.width + refreshSize.width + searchSize.width) + (5 * FOOTER_ICON_SIZE) + (5 * FOOTER_TEXT_BORDER);

			int icon_x = (ScreenWidth - BORDER_X) - icon_width;
			int icon_y = (ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
			int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (returnMenuSize.height / 2));

			App->Graph->drawSizedPNG(&App->Res->cross, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(select, App->Res->robotoFont, icon_x, text_y, dark, white);
			icon_x += selectSize.width + FOOTER_TEXT_BORDER;
			App->Graph->drawSizedPNG(&App->Res->circle, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(returnMenu, App->Res->robotoFont, icon_x, text_y, dark, white);
			icon_x += returnMenuSize.width + FOOTER_TEXT_SIZE;
			App->Graph->drawSizedPNG(&App->Res->square, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(refresh, App->Res->robotoFont, icon_x, text_y, dark, white);
			icon_x += refreshSize.width + FOOTER_TEXT_SIZE;
			App->Graph->drawSizedPNG(&App->Res->triangle, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(search, App->Res->robotoFont, icon_x, text_y, dark, white);

			// Draw page number
			FontSize pageSize;
			App->Graph->getTextSize(currentPageStr, App->Res->robotoFont, &pageSize);
			App->Graph->drawText(currentPageStr, App->Res->robotoFont, BORDER_X, text_y, dark, white);
		}

		App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

		// Unlock the mutex
		scePthreadMutexUnlock(&fetch_mtx);
	}

	return 0;
}