#include "Resource.h"
#include "App.h"
#include "Graphics.h"

// Initialize resources
Resource::Resource(Application* app) {
	int rc;

	App = app;
	if (!App) { printf("[ERROR] App is null !\n"); return;  }

	// Images load
	rc = App->Graph->loadPNG(&logo, "/app0/Logo.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load logo !\n");
	}
	logo.use_alpha = false;

	rc = App->Graph->loadPNG(&unknown, "/app0/UnknownImage.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load unknown image !\n");
	}
	unknown.use_alpha = false;

	rc = App->Graph->loadPNG(&cross, "/app0/Cross.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load cross image !\n");
	}
	cross.use_alpha = false;

	rc = App->Graph->loadPNG(&triangle, "/app0/Triangle.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load triangle image !\n");
	}
	triangle.use_alpha = false;

	rc = App->Graph->loadPNG(&square, "/app0/Square.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load square image !\n");
	}
	square.use_alpha = false;

	rc = App->Graph->loadPNG(&circle, "/app0/Circle.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load circle image !\n");
	}
	circle.use_alpha = false;

	rc = App->Graph->loadPNG(&add, "/app0/Add.png");
	if (rc < 0) {
		printf("[WARNING] Unable to load add image !\n");
	}
	add.use_alpha = false;

	// Initialize Roboto font faces
	rc = App->Graph->initFont(&robotoFont, "/app0/roboto.ttf", DEFAULT_FONT_SIZE);
	if (rc != 0)
	{
		printf("[WARNING] Failed to initialize freetype !\n");
	}
}

// Free'ing resources
Resource::~Resource() {
	if (!App) { printf("[ERROR] App is null !\n"); return; }

	App->Graph->unloadPNG(&unknown);
	App->Graph->unloadPNG(&logo);
	App->Graph->unloadPNG(&cross);
	App->Graph->unloadPNG(&triangle);
	App->Graph->unloadPNG(&square);
	App->Graph->unloadPNG(&circle);
	App->Graph->unloadPNG(&add);
}