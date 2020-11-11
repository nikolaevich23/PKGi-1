#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <time.h>

#include "App.h"

int main()
{
	// Start the Application
	Application* mainApp = new Application();
	mainApp->Run();
	delete mainApp;

    return 0;
}
