#include "Graphics.h"

#ifndef RESOURCE_H
#define RESOURCE_H

#define DEFAULT_FONT_SIZE 32

class Application;

class Resource
{
public:
	Resource(Application* app);
	~Resource();

	// Application definition
	Application* App;

	// Image
	Image logo;
	Image unknown;
	Image cross;
	Image triangle;
	Image square;
	Image circle;
	Image add;

	// Font faces
	FT_Face robotoFont;
};

#endif