#include "SourcesView.h"
#include "App.h"
#include "Graphics.h"
#include "Controller.h"
#include "Resource.h"
#include "Network.h"
#include "TinyJson.h"
#include "Utility.h"

#include "PackageListView.h"
#include "CreditView.h"

// Load sources list from sources.list
Source* SourcesView::LoadSources(int* nbr) {
	size_t size;
	char* data = Utility::ReadFile("/data/sources.list", &size);

	int current_pos = 0;
	bool in_comment = false;
	char url[MAX_URL_LEN] = { 0 };

	printf("LoadSources: Loading sources ...\n");

	int sources_nbr = 1;
	Source* sources_list = (Source*)malloc(sizeof(Source));
	memset((void*)sources_list, 0, sizeof(Source));

	printf("LoadSources: 1\n");

	if (!sources_list) {
		printf("LoadSources: Error during malloc !\n");
		return NULL;
	}

	printf("LoadSources: 2\n");

	// Initialize "Add Source"
	strncpy(sources_list[0].name, "Add a source", 100);
	strncpy(sources_list[0].api_url, "!addsource", MAX_URL_LEN);

	printf("LoadSources: 3\n");

	// If data is not found, only add "Add a source" on the list
	if (!data) {
		printf("sources.list not found.\n");
		*nbr = sources_nbr;
		return sources_list;
	}

	printf("LoadSources: 4 (data: %p)\n", data);


	// Yeah i know, it's weird
	char* sources_text = (char*)malloc(2 * size);
	memset(sources_text, 0, 2 * size);
	memcpy(sources_text, data, size);

	int urls = 0;
	char *lines_str = strtok(sources_text, "\n");

	printf("LoadSources: 5 (line_str: %p)\n", lines_str);

	while (lines_str != NULL) {
		if (lines_str[0] != '#') {
			if (strstr(lines_str, "http") != NULL) {
				sources_nbr++;
				sources_list = (Source*)realloc(sources_list, sources_nbr * sizeof(Source));
				memset(&sources_list[sources_nbr - 1], 0, sizeof(Source));
				strncpy(sources_list[sources_nbr - 1].api_url, lines_str, MAX_URL_LEN);

				printf("Source #%i: %s\n", urls, lines_str);
				urls++;
			}
			else {
				printf("Malformed url: %s\n", lines_str);
			}
		}
		else {
			printf("Comment: %s\n", lines_str);
		}

		lines_str = strtok(NULL, "\n");
	}

	printf("LoadSources: 6\n");

	free(sources_text);
	Utility::CleanupMap(data, size);

	printf("Final source list (%p) (nbr: %i)\n", sources_list, sources_nbr);

	*nbr = sources_nbr;
	return sources_list;
}

// Cleanup sources list
void SourcesView::CleanupSources() {
	printf("Cleanup source list ...");

	if (sources) {
		for (int i = 0; i < sourceNbr; i++) {
			if (sources[i].icon.img != NULL) {
				App->Graph->unloadPNG(&sources[i].icon);
			}
		}

		free(sources);
	}

	sourceNbr = 0;
	sourceSelected = 0;
	sources = NULL;
}

// Download sources information (if the list exist)
void SourcesView::DownloadSourcesInfo() {
	printf("DownloadSourcesInfo is called !\n");

	if (!sources) {
		printf("Unable to download sources info : Sources list doesn't exist.\n");
		scePthreadMutexUnlock(&sources_mtx);
		return;
	}

	scePthreadMutexLock(&sources_mtx);

	for (int i = 0; i < sourceNbr; i++) {
		char url[500];
		snprintf(url, 500, "%s/info.json", sources[i].api_url);

		if (strstr(url, "http") != NULL) {
			printf("DownloadSourcesInfo: Downloading info for %s ...\n", url);

			size_t data_len = 0;
			char* data = (char*)App->Net->GetRequest(url, &data_len);
			if (data == NULL) {
				printf("DownloadSourcesInfo: Unable to got info from source\n");
				sources[i].is_available = false;
				continue;
			}

			printf("DownloadSourcesInfo -> Data: %s\n", data);

			json_t mem[100];
			json_t const* json = json_create(data, mem, 100);
			if (!json) {
				printf("DownloadSourcesInfo: Unable to load json\n");
				sources[i].is_available = false;
				continue;
			}

			// Extract useful data
			json_t const* name_j = json_getProperty(json, "name");
			if (!name_j || JSON_TEXT != json_getType(name_j)) {
				printf("DownloadSourcesInfo: Unable to got the name\n");
				sources[i].is_available = false;
				continue;
			}

			const char* name = json_getValue(name_j);

			if (name)
				strncpy(sources[i].name, name, 100);

			char icon_url[500];
			snprintf(icon_url, 500, "%s/logo.png", sources[i].api_url);

			printf("DownloadSourcesInfo: Downloading icon ...\nURL: %s\n", icon_url);

			size_t icon_len = 0;
			unsigned char* icon_data = (unsigned char*)App->Net->GetRequest(icon_url, &icon_len);
			printf("Data: %p Size: %i\n", icon_data, (int)icon_len);

			if (icon_data) {
				App->Graph->loadPNGFromMemory(&sources[i].icon, icon_data, (int)icon_len);
				free(icon_data);
			}
		}
	}

	sourceSelected = 0;
	printf("DownloadSourcesInfo: Downloaded.\n");

	scePthreadMutexUnlock(&sources_mtx);
}

// Thread of fetch
void SourcesView::FetchSources_Thread(SourcesView* current_view) {
	printf("FetchSources_Thread: Thread launched !\n");

	if (current_view) {
		printf("FetchSources_Thread: current_view ok\n");
		current_view->DownloadSourcesInfo();
	}

	printf("FetchSources_Thread: scePthreadExit\n");
	scePthreadExit(nullptr);
}

// Thread initialization
void SourcesView::FetchSources() {
	printf("FetchPackages: Creating thread ...\n");

	OrbisPthreadAttr attr;
	scePthreadAttrInit(&attr);
	scePthreadCreate(&fetch_thread, &attr, (void*)FetchSources_Thread, (void*)this, "PKGiFetchThread");
}

// Load resource for the view
SourcesView::SourcesView(Application* app)
{
	int rc;
	App = app;
	if (!App) { printf("[ERROR] App is null !\n"); return; }

	// Setup variable

	sourceNbr = 1;
	sources = new Source[1];
	strncpy(sources[0].name, "Add source", 100);
	strncpy(sources[0].api_url, "!addsource", 100);

	sources = LoadSources(&sourceNbr);
	FetchSources();

	printf("List ok, sources: %p, sourceNbr: %i\n", sources, sourceNbr);

	sourceSelected = 0;

	memset(errorMessage, 0, 255);
	onError = false;

	fetchStepAnim = 0;
	fetchTimeStep = 0;

	scePthreadMutexInit(&sources_mtx, NULL, "PKGiSourceMTX");

	// Set colors
	bgColor = { 0, 0, 0, 255 };
	fgColor = { 255, 255, 255, 255 };

	printf("SourcesView: End of constructor\n");
}

// Unload resource
SourcesView::~SourcesView()
{
	// Cleanup memory
	scePthreadMutexLock(&sources_mtx);
	CleanupSources();
	scePthreadMutexUnlock(&sources_mtx);

	// Destroy mutex
	scePthreadMutexDestroy(&sources_mtx);
}

// Show a message if error occurs
void SourcesView::ShowError(const char* error) {
	snprintf(errorMessage, sizeof(errorMessage), "%s", error);
	onError = true;
}

int SourcesView::Update() {
	// If mutex is locked, sources fetch is in progress
	if (scePthreadMutexTrylock(&sources_mtx) >= 0) {
		if (onError) {
			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
				onError = false;
			}
		}
		else {
			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_UP)) {
				if ((sourceSelected - 1) >= 0) {
					sourceSelected--;
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_DOWN)) {
				if ((sourceSelected + 1) < sourceNbr) {
					sourceSelected++;
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
				CreditView* cdt_view = new CreditView(App);
				App->ChangeView(cdt_view);
				scePthreadMutexUnlock(&sources_mtx);
				delete this;
				return 0;
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_SQUARE)) {
				if (sources && sourceSelected >= 0) {
					Utility::RemplaceText("/data/sources.list", sources[sourceSelected].api_url, "");

					// Cleanup memory and reload the list
					printf("Cleanup sources.\n");
					CleanupSources();

					printf("Reload sources.\n");
					sources = LoadSources(&sourceNbr);
					FetchSources();
				}
			}

			if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
				if (sources && sourceSelected >= 0) {
					if (strncmp(sources[sourceSelected].api_url, "!addsource", 10) == 0) {
						printf("Add source called.");

						char* source_url = Utility::OpenKeyboard(ORBIS_TYPE_TYPE_URL, "Enter Source URL");
						if (source_url) {
							if (strstr(source_url, "http") != NULL) {
								printf("Valid source url.\n");
								printf("Source url: %s\n", source_url);
								Utility::AppendText("/data/sources.list", source_url);

								// Cleanup memory and reload the list
								printf("Cleanup sources.\n");
								CleanupSources();

								printf("Reload sources.\n");
								sources = LoadSources(&sourceNbr);
								FetchSources();
							}
							else {
								printf("Not a valid source url.\n");
								ShowError("This is not a valid sources url");

							}

							free(source_url);
						}
					}
					else {
						// Goto PackageListView
						PackageListArg args;
						snprintf(args.title, 100, "%s", sources[sourceSelected].name);
						snprintf(args.api_url, 100, "%s", sources[sourceSelected].api_url);
						PackageListView* pkgs_view = new PackageListView(App, &args);
						App->ChangeView(pkgs_view);
						scePthreadMutexUnlock(&sources_mtx);
						delete this;
						return 0;
					}
				}
			}
		}

		// Unlock the mutex
		scePthreadMutexUnlock(&sources_mtx);
	}

	return 0;
}

int SourcesView::Render() {
	int ScreenHeight = App->Graph->GetScreenHeight();
	int ScreenWidth = App->Graph->GetScreenWidth();

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
	int source_line_height = (menu_size_height - consumable_border) / MENU_NBR_PER_PAGE;

	int delta = sourceSelected / MENU_NBR_PER_PAGE;
	int maxPage = sourceNbr / MENU_NBR_PER_PAGE;

	char currentPageStr[255];
	sprintf(currentPageStr, "Page %i / %i", (delta + 1), (maxPage + 1));

	// Draw logo
	int logo_y = (HEADER_SIZE / 2) - (120 / 2);
	App->Graph->drawPNG(&App->Res->logo, BORDER_X, logo_y);

	// Draw source name
	char titleName[20] = "Sources List";
	FontSize titleSize;
	App->Graph->setFontSize(App->Res->robotoFont, 54);
	App->Graph->getTextSize(titleName, App->Res->robotoFont, &titleSize);
	//App->Graph->drawText(titleName, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, HEADER_SIZE - 40 - titleSize.height, dark, white);
	App->Graph->drawText(titleName, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, (logo_y + ((120 / 2) - (titleSize.height / 2))), dark, white);
	App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

	// Draw header border
	App->Graph->drawRectangle(BORDER_X, HEADER_SIZE - 5, ScreenWidth - (BORDER_X * 2), 5, white);

	// If mutex is locked, fetch is in progress
	if (scePthreadMutexTrylock(&sources_mtx) < 0) {
		char progress[255] = "Fetching sources list .";

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

		// Mutex now locked, draw packages list
		if (onError) {
			FontSize errorSize;
			App->Graph->getTextSize(errorMessage, App->Res->robotoFont, &errorSize);
			App->Graph->drawText(errorMessage, App->Res->robotoFont, ((ScreenWidth / 2) - (errorSize.width / 2)), ((ScreenHeight / 2) - (errorSize.height / 2)), dark, white);
		}
		else if (sources) {
			// Draw sources
			for (int posMenu = 0; posMenu < MENU_NBR_PER_PAGE; posMenu++) {
				int currentSource = (delta * MENU_NBR_PER_PAGE) + posMenu;
				if (currentSource >= sourceNbr) {
					break;
				}

				Color selection = grey;
				if (sourceSelected == currentSource) {
					selection = orange;
				}

				FontSize titleSize;

				if (strlen(sources[currentSource].name) > 0) {
					App->Graph->getTextSize(sources[currentSource].name, App->Res->robotoFont, &titleSize);
				} 
				else
				{
					App->Graph->getTextSize(sources[currentSource].api_url, App->Res->robotoFont, &titleSize);
				}

				int current_menu_y = menu_y + (posMenu * source_line_height) + (posMenu * MENU_BORDER);
				App->Graph->drawRectangle(menu_x, current_menu_y, menu_size_width, source_line_height, selection);

				int icon_size = source_line_height - (2 * MENU_IN_IMAGE_BORDER);


				if (currentSource == 0) {
					App->Graph->drawSizedPNG(&App->Res->add, menu_x + MENU_IN_IMAGE_BORDER, current_menu_y + MENU_IN_IMAGE_BORDER, icon_size, icon_size);
				}
				else if (sources[currentSource].icon.img != NULL) {
					App->Graph->drawSizedPNG(&sources[currentSource].icon, menu_x + MENU_IN_IMAGE_BORDER, current_menu_y + MENU_IN_IMAGE_BORDER, icon_size, icon_size);
				}
				else {
					App->Graph->drawSizedPNG(&App->Res->unknown, menu_x + MENU_IN_IMAGE_BORDER, current_menu_y + MENU_IN_IMAGE_BORDER, icon_size, icon_size);
				}

				if (strlen(sources[currentSource].name) > 0) {
					App->Graph->drawText(sources[currentSource].name, App->Res->robotoFont, menu_x + icon_size + (3 * MENU_IN_IMAGE_BORDER), current_menu_y + ((source_line_height / 2) - (titleSize.height / 2)), selection, white);
				}
				else {
					App->Graph->drawText(sources[currentSource].api_url, App->Res->robotoFont, menu_x + icon_size + (3 * MENU_IN_IMAGE_BORDER), current_menu_y + ((source_line_height / 2) - (titleSize.height / 2)), selection, white);
				}
			}
		}
		else {
			FontSize ohnoSize;
			char ohno[255] = "No sources available :(";
			App->Graph->getTextSize(ohno, App->Res->robotoFont, &ohnoSize);
			App->Graph->drawText(ohno, App->Res->robotoFont, ((ScreenWidth / 2) - (ohnoSize.width / 2)), ((ScreenHeight / 2) - (ohnoSize.height / 2)), dark, white);
		}

		// Draw footer background and border
		App->Graph->drawRectangle(0, ScreenHeight - FOOTER_SIZE, ScreenWidth, FOOTER_SIZE, dark);
		App->Graph->drawRectangle(BORDER_X, ScreenHeight - FOOTER_SIZE + 5, ScreenWidth - (BORDER_X * 2), 5, white);

		App->Graph->setFontSize(App->Res->robotoFont, FOOTER_TEXT_SIZE);

		if (onError) {
			char ok[255] = "OK";
			FontSize okSize;
			App->Graph->getTextSize(ok, App->Res->robotoFont, &okSize);

			int icon_width = okSize.width + FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;

			int icon_x = (ScreenWidth - BORDER_X) - icon_width;
			int icon_y = (ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
			int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (okSize.height / 2));

			App->Graph->drawSizedPNG(&App->Res->cross, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(ok, App->Res->robotoFont, icon_x, text_y, dark, white);
		}
		else {
			char select[255] = "Select";
			char credit[255] = "Credit";
			char remove[255] = "Remove";

			FontSize selectSize;
			FontSize removeSize;
			FontSize creditSize;

			App->Graph->getTextSize(select, App->Res->robotoFont, &selectSize);
			App->Graph->getTextSize(credit, App->Res->robotoFont, &creditSize);
			App->Graph->getTextSize(remove, App->Res->robotoFont, &removeSize);

			int icon_width = (selectSize.width + removeSize.width + creditSize.width) + (4 * FOOTER_ICON_SIZE) + (4 * FOOTER_TEXT_BORDER);

			int icon_x = (ScreenWidth - BORDER_X) - icon_width;
			int icon_y = (ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
			int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (selectSize.height / 2));

			App->Graph->drawSizedPNG(&App->Res->cross, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(select, App->Res->robotoFont, icon_x, text_y, dark, white);
			icon_x += selectSize.width + FOOTER_TEXT_SIZE;
			App->Graph->drawSizedPNG(&App->Res->square, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(remove, App->Res->robotoFont, icon_x, text_y, dark, white);
			icon_x += removeSize.width + FOOTER_TEXT_SIZE;
			App->Graph->drawSizedPNG(&App->Res->triangle, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
			icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
			App->Graph->drawText(credit, App->Res->robotoFont, icon_x, text_y, dark, white);
			//icon_x += creditSize.width + FOOTER_TEXT_SIZE;

			// Draw page number
			FontSize pageSize;
			App->Graph->getTextSize(currentPageStr, App->Res->robotoFont, &pageSize);
			App->Graph->drawText(currentPageStr, App->Res->robotoFont, BORDER_X, text_y, dark, white);
		}

		App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

		// Unlock the mutex
		scePthreadMutexUnlock(&sources_mtx);
	}

	return 0;
}