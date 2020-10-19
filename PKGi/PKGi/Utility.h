#include <stdint.h>
#include <sys/mman.h>

#include <orbis/libkernel.h>
#include <orbis/SystemService.h>

#ifndef UTILITY_H
#define UTILITY_H

class Utility
{
	public:
		int CopyFile(char* source, char* destination);
};

#endif