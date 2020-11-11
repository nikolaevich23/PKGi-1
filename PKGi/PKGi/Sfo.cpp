// Based on sfo.c of VitaShell
// https://github.com/TheOfficialFloW/VitaShell/blob/master/sfo.c
// VitaShell - Copyright (C) 2015-2018, TheFloW - GNU General Public Licence

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Sfo.h"

Sfo::Sfo(void* buffer, size_t sfo_len) {
	sfo_buffer = nullptr;
	if (buffer) {
		sfo_buffer = malloc(sfo_len);
		if (!sfo_buffer) {
			printf("Sfo: Unable to malloc !\n");
			return;
		}
	
		memcpy(sfo_buffer, buffer, sfo_len);
	}
}

Sfo::~Sfo() {
	if (sfo_buffer) {
		free(sfo_buffer);
		sfo_buffer = nullptr;
	}
}

int Sfo::getSfoValue(const char *name, uint32_t *value) {
	if (!sfo_buffer) {
		printf("Sfo: Buffer is not initialized\n");
		return -2;
	}

	SfoHeader *header = (SfoHeader *)sfo_buffer;
	SfoEntry *entries = (SfoEntry *)((uint64_t)sfo_buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
		return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp((char*)((uint64_t)sfo_buffer + header->keyofs + entries[i].nameofs), name) == 0) {
			*value = *(uint32_t *)((uint64_t)sfo_buffer + header->valofs + entries[i].dataofs);
			return 0;
		}
	}

	return -2;
}

int Sfo::getSfoString(const char *name, char *string, int length) {
	if (!sfo_buffer) {
		printf("Sfo: Buffer is not initialized\n");
		return -2;
	}

	SfoHeader *header = (SfoHeader *)sfo_buffer;
	SfoEntry *entries = (SfoEntry *)((uint64_t)sfo_buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
		return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp((char*)((uint64_t)sfo_buffer + header->keyofs + entries[i].nameofs), name) == 0) {
			memset(string, 0, length);
			strncpy(string, (char*)((uint64_t)sfo_buffer + header->valofs + entries[i].dataofs), length);
			string[length - 1] = '\0';
			return 0;
		}
	}

	return -2;
}

int Sfo::setSfoValue(const char *name, uint32_t value) {
	if (!sfo_buffer) {
		printf("Sfo: Buffer is not initialized\n");
		return -2;
	}

	SfoHeader *header = (SfoHeader *)sfo_buffer;
	SfoEntry *entries = (SfoEntry *)((uint64_t)sfo_buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
		return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp((char*)((uint64_t)sfo_buffer + header->keyofs + entries[i].nameofs), name) == 0) {
			*(uint32_t *)((uint64_t)sfo_buffer + header->valofs + entries[i].dataofs) = value;
			return 0;
		}
	}

	return -2;
}

int Sfo::setSfoString(const char *name, const char *string) {
	if (!sfo_buffer) {
		printf("Sfo: Buffer is not initialized\n");
		return -2;
	}

	SfoHeader *header = (SfoHeader *)sfo_buffer;
	SfoEntry *entries = (SfoEntry *)((uint64_t)sfo_buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
		return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp((char*)((uint64_t)sfo_buffer + header->keyofs + entries[i].nameofs), name) == 0) {
			strcpy((char*)((uint64_t)sfo_buffer + header->valofs + entries[i].dataofs), string);
			return 0;
		}
	}

	return -2;
}
