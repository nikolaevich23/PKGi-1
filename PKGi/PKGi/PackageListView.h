#include "View.h"
#include "Graphics.h"

#ifndef PACKAGE_LIST_VIEW_H
#define PACKAGE_LIST_VIEW_H

#define SEARCH_LEN 255

class Application;
class PackageListView;

typedef struct  {
	int id;
	char name[255];
	Image icon;
	void(*callback)(void);
} Package;

typedef struct {
	char title[100];
	char api_url[100];
} PackageListArg;

typedef struct {
	PackageListView* MView;
	int line;
	int page;
	bool is_up;
	char searchTerm[255];
} FetchArg;

class PackageListView : public View
{
public:
	PackageListView(Application* app, PackageListArg* args);
	~PackageListView();
	int Update(void);
	int Render(void);
	void DownloadPkgList(int line, int page, char* search, bool is_up);

private:
	Application* App;

	// Background and foreground colors
	Color bgColor;
	Color fgColor;

	// Package list system
	OrbisPthreadMutex fetch_mtx;
	OrbisPthread fetch_thread;

	PackageListArg source;

	Package* pkgs;
	int pkgNbr;
	int pkgSelected;

	int currentPage;
	int totalPage;
	int totalPkgCount;

	char searchTerm[SEARCH_LEN];
	char errorMessage[255];
	bool onError;
	bool ErrordontReturn;

	// Fetch animation
	int fetchStepAnim;
	int fetchTimeStep;

	static void FetchPackages_Thread(FetchArg* args);
	void FetchPackages(int page, int line, bool is_up);
	void ShowError(const char* error, bool dontReturn);
	void ActualiseList(int page);
};
#endif