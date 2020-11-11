#include "CreditView.h"
#include "App.h"
#include "Graphics.h"
#include "Controller.h"
#include "Resource.h"

#include "SourcesView.h"

#define HEADER_SIZE 200
#define FOOTER_SIZE 100
#define BORDER_X 100

// Load the credit view value
CreditView::CreditView(Application* app)
{
	printf("CreditView: loaded !\n");

	App = app;
	if (!App) { printf("[ERROR] App is null !\n"); return; }

	// Set colors
	bgColor = { 0, 0, 0, 255 };
	fgColor = { 255, 255, 255, 255 };

	printf("CreditView: Initialized !\n");
}

CreditView::~CreditView() 
{
}

int CreditView::Update() {
	if (App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE)) {
		SourcesView* src_view = new SourcesView(App);
		App->ChangeView(src_view);
		delete this;
		return 0;
	}

	return 0;
}

int CreditView::Render() {
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

	// Draw logo
	int logo_y = (HEADER_SIZE / 2) - (120 / 2);
	App->Graph->drawPNG(&App->Res->logo, BORDER_X, logo_y);

	// Draw credit name
	char titleName[10] = "Credit";
	FontSize titleSize;
	App->Graph->setFontSize(App->Res->robotoFont, 54);
	App->Graph->getTextSize(titleName, App->Res->robotoFont, &titleSize);
	//App->Graph->drawText(titleName, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, HEADER_SIZE - 40 - titleSize.height, dark, white);
	App->Graph->drawText(titleName, App->Res->robotoFont, BORDER_X + 120 + BORDER_X, (logo_y + ((120 / 2) - (titleSize.height / 2))), dark, white);
	App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

	// Draw header border
	App->Graph->drawRectangle(BORDER_X, HEADER_SIZE - 5, ScreenWidth - (BORDER_X * 2), 5, white);

	FontSize textSize;
	char creditText[300] = "TheoryWrong : Application\n\n0x199: API Help\n\nFlatZ: Writeup for PKG\n\nOpenOrbis Team: Mira & Toolchain\n\nPbhogan: TinyJson\n\nFreepik: Package icon\n\nThis is a beta build.";
	App->Graph->getTextSize(creditText, App->Res->robotoFont, &textSize);
	App->Graph->drawText(creditText, App->Res->robotoFont, (ScreenWidth / 2) - (textSize.width / 2), (ScreenHeight / 2) - (textSize.height / 2), bgColor, fgColor);

	// Draw footer background and border
	App->Graph->drawRectangle(0, ScreenHeight - FOOTER_SIZE, ScreenWidth, FOOTER_SIZE, dark);
	App->Graph->drawRectangle(BORDER_X, ScreenHeight - FOOTER_SIZE + 5, ScreenWidth - (BORDER_X * 2), 5, white);

	App->Graph->setFontSize(App->Res->robotoFont, FOOTER_TEXT_SIZE);

	char returnMain[255] = "Return";

	FontSize returnMainSize;
	App->Graph->getTextSize(returnMain, App->Res->robotoFont, &returnMainSize);

	int icon_width = (returnMainSize.width) + (1 * FOOTER_ICON_SIZE) + (1 * FOOTER_TEXT_BORDER);

	int icon_x = (ScreenWidth - BORDER_X) - icon_width;
	int icon_y = (ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
	int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (returnMainSize.height / 2));

	App->Graph->drawSizedPNG(&App->Res->circle, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
	icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
	App->Graph->drawText(returnMain, App->Res->robotoFont, icon_x, text_y, dark, white);

	App->Graph->setFontSize(App->Res->robotoFont, DEFAULT_FONT_SIZE);

	return 0;
}