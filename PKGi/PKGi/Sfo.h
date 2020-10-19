// Based on sfo.h of VitaShell
// https://github.com/TheOfficialFloW/VitaShell/blob/master/sfo.h
// VitaShell - Copyright (C) 2015-2018, TheFloW - GNU General Public Licence

#include <stdint.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>

#ifndef SFO_H
#define SFO_H

#define SFO_MAGIC 0x46535000

#define PSF_TYPE_BIN 0
#define PSF_TYPE_STR 2
#define PSF_TYPE_VAL 4

typedef struct SfoHeader {
	uint32_t magic;
	uint32_t version;
	uint32_t keyofs;
	uint32_t valofs;
	uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoEntry {
	uint16_t nameofs;
	uint8_t  alignment;
	uint8_t  type;
	uint32_t valsize;
	uint32_t totalsize;
	uint32_t dataofs;
} __attribute__((packed)) SfoEntry;

class Sfo
{
public:
	void* sfo_buffer;

	Sfo(void* buffer, size_t sfo_len);
	~Sfo();
	int getSfoValue(const char *name, uint32_t *value);
	int getSfoString(const char *name, char *string, int length);
	int setSfoValue(const char *name, uint32_t value);
	int setSfoString(const char *name, const char *string);

};

#endif