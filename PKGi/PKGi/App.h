#include <stdint.h>
#include "View.h"

#ifndef APPLICATION_H
#define APPLICATION_H

#define APP_VER 100

class Controller;
class Graphics;
class Resource;
class Network;
class Mira;
class AppInstaller;

class Application
{
public:
	Controller* Ctrl;
	Graphics*   Graph;
	Resource*   Res;
	Network*    Net;
	Mira*       Kernel;
	AppInstaller* AppInst;

	Application();
	~Application();
	void Run();
	void Update();
	void Render();
	void ChangeView(View* view);
private:
	bool isRunning;
	View* currentView;
	int flipArgs;

	void ShowFatalReason(const char* reason);
};

#endif