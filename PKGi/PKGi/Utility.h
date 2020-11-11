#include <stdint.h>
#include <sys/mman.h>

#include <orbis/libkernel.h>
#include <orbis/ImeDialog.h>

#ifndef UTILITY_H
#define UTILITY_H

class Utility
{
	public:
		static int   RemplaceText(const char* file, const char* text, const char* what);
		static char* StrReplace(char *orig, char *rep, char *with);
		static int   CopyFile(char* source, char* destination);
		static char* ReadFile(const char* path, size_t* size);
		static int   CleanupMap(void* data, size_t data_size);
		static int   AppendText(const char* file, const char* line);
		static char* OpenKeyboard(OrbisImeType type, const char* title);
		static bool  GetFWVersion(char* version);
		static bool  ByteToHex(char* buf, size_t buf_size, const void* data, size_t data_size);
};

#endif