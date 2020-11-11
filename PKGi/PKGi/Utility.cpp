#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "Utility.h"

// Remplace a string inside a chain
// Source: https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
char* Utility::StrReplace(char *orig, char *rep, char *with) {
	char *result; // the return string
	char *ins;    // the next insert point
	char *tmp;    // varies
	int len_rep;  // length of rep (the string to remove)
	int len_with; // length of with (the string to replace rep with)
	int len_front; // distance between rep and end of last rep
	int count;    // number of replacements

	// sanity checks and initialization
	if (!orig || !rep)
		return NULL;
	len_rep = strlen(rep);
	if (len_rep == 0)
		return NULL; // empty rep causes infinite loop during count

	len_with = strlen(with);

	// count the number of replacements needed
	ins = orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count) {
		ins = tmp + len_rep;
	}

	tmp = result = (char*)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
		return NULL;

	// first time through the loop, all the variable are set correctly
	// from here on,
	//    tmp points to the end of the result string
	//    ins points to the next occurrence of rep in orig
	//    orig points to the remainder of orig after "end of rep"
	while (count--) {
		ins = strstr(orig, rep);
		len_front = ins - orig;
		tmp = strncpy(tmp, orig, len_front) + len_front;
		tmp = strcpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	strcpy(tmp, orig);
	return result;
}

// Copy file
int Utility::CopyFile(char* source, char* destination) {
	printf("Copying %s to %s ...\n", source, destination);

	int fd_src = sceKernelOpen(source, O_RDONLY, 0777);
	int fd_dst = sceKernelOpen(destination, O_WRONLY | 0x0200 | 0x0400, 0777);
	if (fd_src < 0 || fd_dst < 0)
		return 1;

	int data_size = sceKernelLseek(fd_src, 0, SEEK_END);
	sceKernelLseek(fd_src, 0, SEEK_SET);

	void* data = NULL;

	sceKernelMmap(0, data_size, PROT_READ, MAP_PRIVATE, fd_src, 0, &data);
	sceKernelWrite(fd_dst, data, data_size);
	sceKernelMunmap(data, data_size);

	sceKernelClose(fd_dst);
	sceKernelClose(fd_src);

	return 0;
}

// Read file
char* Utility::ReadFile(const char* path, size_t* size) {
	printf("Reading %s ...\n", path);

	int fd_src = sceKernelOpen(path, 0x0000, 0777);
	if (fd_src < 0) {
		printf("Cannot open %s (0x%08x)\n", path, fd_src);
		return NULL;
	}

	int data_size = sceKernelLseek(fd_src, 0, SEEK_END);
	sceKernelLseek(fd_src, 0, SEEK_SET);

	if (data_size <= 0) {
		printf("Error with size (0x%08x)\n", data_size);
		sceKernelClose(fd_src);
		return NULL;
	}

	void* data = NULL;
	sceKernelMmap(0, data_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_src, 0, &data);
	sceKernelClose(fd_src);

	*size = data_size;

	printf("sources.list data: %p\n", data);
	printf("Data: %s (Size: %d)\n", data, data_size);

	return (char*)data;
}

// Cleanup file data
int Utility::CleanupMap(void* data, size_t data_size) {
	printf("Cleanup map %p (size: %lu)\n", data, data_size);
	return sceKernelMunmap(data, data_size);
}

// Append text on a file
int Utility::AppendText(const char* file, const char* line) {
	printf("Appening %s ...", file);

	int fd_dst = sceKernelOpen(file, 0x0002| 0x0200, 0777);

	if (fd_dst < 0) {
		printf("Cannot open(write) %s (0x%08x)\n", file, fd_dst);
		return 1;
	}

	int old_size = sceKernelLseek(fd_dst, 0, SEEK_END);

	sceKernelWrite(fd_dst, "\n", 1);
	sceKernelWrite(fd_dst, line, strlen(line));
	sceKernelClose(fd_dst);

	printf(" [Done].\n");
	return 0;
}

// Remplace text on a file
int Utility::RemplaceText(const char* file, const char* text, const char* what){
	printf("Remplace %s ...", file);

	size_t size;
	char* data = Utility::ReadFile(file, &size);
	if (!data)
		return 1;

	char* final = Utility::StrReplace(data, (char*)text, (char*)what);
	Utility::CleanupMap(data, size);

	if (!final)
		return 2;

	int fd_dst = sceKernelOpen(file, 0x0002 | 0x0200 | 0x0400, 0777);

	if (fd_dst < 0) {
		printf("Cannot open(write) %s (0x%08x)\n", file, fd_dst);
		return 1;
	}

	sceKernelWrite(fd_dst, final, strlen(final));
	sceKernelClose(fd_dst);

	printf(" [Done].\n");
	free(final);
	return 0;
}

// Open Keyboard (return a char on a malloc)
char* Utility::OpenKeyboard(OrbisImeType type, const char* title) {
	printf("Opening keyboard ...\n");
	OrbisImeDialogSetting param;
	memset(&param, 0, sizeof(OrbisImeDialogSetting));

	wchar_t bufferText[255];
	wchar_t buffTitle[100];

	memset(bufferText, 0, sizeof(bufferText));
	memset(buffTitle, 0, sizeof(buffTitle));

	mbstowcs(buffTitle, title, strlen(title) + 1);

	param.maxTextLength = sizeof(bufferText);
	param.inputTextBuffer = bufferText;
	param.title = buffTitle;
	param.userId = 254;
	param.type = type;
	param.enterLabel = ORBIS_BUTTON_LABEL_DEFAULT;
	param.inputMethod = OrbisInput::ORBIS__DEFAULT;

	int initDialog = sceImeDialogInit(&param, NULL);

	bool keyboardRunning = true;
	while (keyboardRunning) {
		int status = sceImeDialogGetStatus();

		if (status == ORBIS_DIALOG_STATUS_STOPPED) {
			OrbisDialogResult result;
			memset(&result, 0, sizeof(OrbisDialogResult));
			sceImeDialogGetResult(&result);

			if (result.endstatus == ORBIS_DIALOG_OK) {
				printf("DIALOG OK\n");

				char* finalText = (char*)malloc(sizeof(bufferText) + 1);

				if (!finalText) {
					printf("OpenKeyboard: Error during malloc !\n");
					return NULL;
				}

				wcstombs(finalText, bufferText, sizeof(bufferText));

				printf("Text: %s\n", finalText);
				keyboardRunning = false;
				sceImeDialogTerm();

				return finalText;
			}

			keyboardRunning = false;
			sceImeDialogTerm();
		}
		else if (status == ORBIS_DIALOG_STATUS_NONE) {
			keyboardRunning = false;
		}
	}

	return NULL;
}

// Get firmware version from libc (For prevent kernel change)
bool Utility::GetFWVersion(char* version)
{
	char fw[2] = { 0 };
	int fd = sceKernelOpen("/system/common/lib/libc.sprx", 0x0, 0777);
	if (fd) {
		sceKernelLseek(fd, 0x374, SEEK_CUR);
		sceKernelRead(fd, &fw, 2);
		sceKernelClose(fd);

		sprintf(version, "%02x.%02x", fw[1], fw[0]);

		return true;
	}
	else {
		printf("Failed to open libc !");
		return false;
	}
}

// Transform bytes to hex string
bool Utility::ByteToHex(char* buf, size_t buf_size, const void* data, size_t data_size) {
	static const char* digits = "0123456789ABCDEF";
	const uint8_t* in = (const uint8_t*)data;
	char* out = buf;
	uint8_t c;
	size_t i;
	bool ret;

	if (!buf || !data) {
		ret = false;
		goto err;
	}
	if (!buf_size || buf_size < (data_size * 2 + 1)) {
		ret = false;
		goto err;
	}
	if (!data_size) {
		*out = '\0';
		goto done;
	}

	for (i = 0; i < data_size; ++i) {
		c = in[i];
		*out++ = digits[c >> 4];
		*out++ = digits[c & 0xF];
	}
	*out++ = '\0';

done:
	ret = true;

err:
	return ret;
}