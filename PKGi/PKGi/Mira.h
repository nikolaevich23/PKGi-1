#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sparse.h"

#ifndef MIRA_H
#define MIRA_H

#define _MAX_PATH 260

typedef enum SceAuthenticationId_t : uint64_t
{
	SceVdecProxy = 0x3800000000000003ULL,
	SceVencProxy = 0x3800000000000004ULL,
	Orbis_audiod = 0x3800000000000005ULL,
	Coredump = 0x3800000000000006ULL,
	SceSysCore = 0x3800000000000007ULL,
	Orbis_setip = 0x3800000000000008ULL,
	GnmCompositor = 0x3800000000000009ULL,
	SceShellUI = 0x380000000000000fULL, // NPXS20001
	SceShellCore = 0x3800000000000010ULL,
	NPXS20103 = 0x3800000000000011ULL,
	NPXS21000 = 0x3800000000000012ULL,
	OrbisSWU = 0x3801000000000024ULL,
	Decid = 0x3800000000010003,
} SceAuthenticationId;

typedef enum SceCapabilities_t : uint64_t
{
	Max = 0xFFFFFFFFFFFFFFFFULL,
} SceCapabilites;

typedef enum class _State : uint32_t
{
	Get,
	Set,
	COUNT
} GSState;


// "safe" way in order to modify kernel ucred externally
typedef struct _MiraThreadCredentials {
	typedef enum class _MiraThreadCredentialsPrison : uint32_t
	{
		// Non-root prison
		Default,

		// Switch prison to root vnode
		Root,

		// Total options count
		COUNT
	} MiraThreadCredentialsPrison;

	// Is this a get or set operation
	GSState State;

	// Process ID to modify
	int32_t ProcessId;

	// Threaad ID to modify, or -1 for (all threads in process, USE WITH CAUTION)
	int32_t ThreadId;

	// Everything below is Get/Set
	int32_t EffectiveUserId;
	int32_t RealUserId;
	int32_t SavedUserId;
	int32_t NumGroups;
	int32_t RealGroupId;
	int32_t SavedGroupId;
	MiraThreadCredentialsPrison Prison;
	SceAuthenticationId SceAuthId;
	SceCapabilites Capabilities[4];
	uint64_t Attributes[4];
} MiraThreadCredentials;

#define MIRA_IOCTL_BASE 'M'

typedef struct _MiraMountInSandbox
{
	int32_t Permissions;
	char Path[_MAX_PATH];
	char Name[50];
} MiraMountInSandbox;

typedef struct _MiraUnmountInSandbox
{
	char Name[50];
} MiraUnmountInSandbox;

#define MIRA_MOUNT_IN_SANDBOX 0x813C4D04
#define MIRA_UNMOUNT_IN_SANDBOX 0x813c4d04
#define MIRA_GET_PROC_THREAD_CREDENTIALS 0xC0704D01

class Mira {
private:
	int mira_dev;
	int(*mira_ioctl)(int, unsigned long, ...);
public:
	Mira();
	~Mira();
	bool isAvailable();
	void MountInSandbox(const char* name, const char* path, int permission);
	void UnmountInSandbox(const char* name);
	void ChangeAuthID(SceAuthenticationId_t authid);
};

#endif