#include "View.h"
#include "Graphics.h"

#ifndef SOURCES_VIEW_H
#define SOURCES_VIEW_H

class Application;
class SourcesView;

#define MAX_URL_LEN 100

typedef struct  {
	char name[100];
	char api_url[MAX_URL_LEN];
	Image icon;
	bool is_available;
} Source;

class SourcesView : public View
{
public:
	SourcesView(Application* app);
	~SourcesView();
	int Update(void);
	int Render(void);

	static void FetchSources_Thread(SourcesView* current_view);
	void DownloadSourcesInfo();
private:
	Application* App;

	// Background and foreground colors
	Color bgColor;
	Color fgColor;

	// Package list system
	OrbisPthreadMutex sources_mtx;
	OrbisPthread fetch_thread;

	Source* sources;
	int sourceNbr;
	int sourceSelected;

	char errorMessage[255];
	bool onError;

	// Fetch animation
	int fetchStepAnim;
	int fetchTimeStep;

	void ShowError(const char* error);
	Source* LoadSources(int* nbr);
	void CleanupSources();
	void FetchSources();
};
#endif