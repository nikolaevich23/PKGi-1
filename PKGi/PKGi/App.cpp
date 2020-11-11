#include "App.h"

#include "Controller.h"
#include "Graphics.h"
#include "Resource.h"
#include "Network.h"
#include "Mira.h"
#include "AppInstaller.h"
#include "View.h"
#include "SourcesView.h"
#include "PackageListView.h"

#include <orbis/Sysmodule.h>

#define SCE_SYSMDOULE_LIBIME 0x0095
#define SCE_SYSMODULE_IME_DIALOG 0x0096

Application::Application() {
	isRunning = false;

	// Initialize system componant

	printf("Initialize Controller ...\n");
	Ctrl  = new Controller();
	printf("Initialize Graphics ...\n");
	Graph = new Graphics();
	printf("Initialize Resource ...\n");
	Res   = new Resource(this);
	printf("Initialize Network ...\n");
	Net = new Network();
	printf("Initialize Mira");
	Kernel = new Mira();

	// Load some needed module
	sceSysmoduleLoadModule(SCE_SYSMDOULE_LIBIME);
	sceSysmoduleLoadModule(SCE_SYSMODULE_IME_DIALOG);

	if (Kernel->isAvailable()) {
		// Mount /data and /system into sandbox
		Kernel->MountInSandbox("data", "/data", 511);
		Kernel->MountInSandbox("system", "/system", 511);

		// Change Auth ID by SceShellCore and give full capability
		Kernel->ChangeAuthID(SceAuthenticationId::SceShellCore, SceCapabilites::Max);

		// Initialize the AppInstaller
		AppInst = new AppInstaller(this);

		// Setup the default view
		printf("Initialize the Main View ...\n");

		SourcesView* main_view = new SourcesView(this);
		currentView = main_view;
	}
}

Application::~Application() {
	// Unmount
	Kernel->UnmountInSandbox("data");
	Kernel->UnmountInSandbox("system");

	// Cleanup
	printf("Cleanup Application ...\n");
	delete Res;
	delete Graph;
	delete Ctrl;
	delete Net;
	delete AppInst;
	delete Kernel;
}

void Application::Update() {
	Ctrl->Update();

	if (currentView)
		currentView->Update();
	else
		printf("[ERROR][%s][%d]: No view setup !\n", __FILE__, __LINE__);
}

void Application::ShowFatalReason(const char* reason) {
	while (1) {
		Graph->WaitFlip();

		int ScreenHeight = Graph->GetScreenHeight();
		int ScreenWidth = Graph->GetScreenWidth();

		Color red = { 0xFF, 0x00, 0x00, 0xFF };
		Color white = { 0xFF, 0xFF, 0xFF, 0xFF };

		// Draw red background
		Graph->drawRectangle(0, 0, ScreenWidth, ScreenHeight, red);

		// Draw message
		char message[255];
		snprintf(message, 255, "PKGi is unloadable !\n\nReason: %s\n\n\n\nPlease close the Application.", reason);
		FontSize messageSize;
		Graph->setFontSize(Res->robotoFont, 45);
		Graph->getTextSize(message, Res->robotoFont, &messageSize);
		Graph->drawText(message, Res->robotoFont, ((ScreenWidth / 2) - (messageSize.width / 2)), ((ScreenHeight / 2) - (messageSize.height / 2)), red, white);
		Graph->setFontSize(Res->robotoFont, DEFAULT_FONT_SIZE);

		Graph->SwapBuffer(flipArgs);
		flipArgs++;
	}
}

void Application::Render() {
	Graph->WaitFlip();
	
	if (currentView)
		currentView->Render();
	else
		printf("[ERROR][%s][%d]: No view setup !\n", __FILE__, __LINE__);

	Graph->SwapBuffer(flipArgs);
	flipArgs++;
}

void Application::ChangeView(View* view) {
	printf("Change view to %p !\n", view);

	if (!view)
		return;

	currentView = view;
}

void Application::Run() {
	printf("Application: RUN\n");

	flipArgs = 0;
	isRunning = true;

	// Check if Mira is available
	if (!Kernel->isAvailable()) {
		ShowFatalReason("Mira is needed.");
		return;
	}

	// Main Loop
	while (isRunning)
	{
		Update();
		Render();
	}
}