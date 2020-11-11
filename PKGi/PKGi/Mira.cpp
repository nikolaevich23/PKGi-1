#include "Mira.h"

#include "fcntl.h"
#include "unistd.h"
#include <orbis/libkernel.h>

Mira::Mira() {
	uint32_t kernel_module = sceKernelLoadStartModule("libkernel.sprx", 0, NULL, 0, NULL, NULL);

	mira_dev = sceKernelOpen("/dev/mira", 0, O_RDWR);
	if (mira_dev < 0) {
		printf("ERROR: Mira device is not available (%i)\n", mira_dev);
	}
}

void Mira::MountInSandbox(const char* name, const char* path, int permission) {
	MiraMountInSandbox arguments;
	strncpy(arguments.Path, path, _MAX_PATH);
	strncpy(arguments.Name, name, 50);
	arguments.Permissions = (int32_t)permission;

	ioctl(mira_dev, (unsigned long)MIRA_MOUNT_IN_SANDBOX, &arguments);
	printf("MountInSandbox : Name (%s) Path (%s) Perm (%i) Arg(%p)\n", name, path, permission, &arguments);
}

void Mira::UnmountInSandbox(const char* name) {
	MiraUnmountInSandbox arguments;
	strncpy(arguments.Name, name, 50);

	ioctl(mira_dev, (unsigned long)MIRA_UNMOUNT_IN_SANDBOX, &arguments);
	printf("UnmountInSandbox : Name (%s) Arg(%p)\n", name, &arguments);
}

void Mira::ChangeAuthID(SceAuthenticationId_t authid, SceCapabilites capab) {
	MiraThreadCredentials params;
	params.State = GSState::Get;
	params.ThreadId = 0;
	params.ProcessId = 0;
	ioctl(mira_dev, (unsigned long)MIRA_GET_PROC_THREAD_CREDENTIALS, &params);

	params.State = GSState::Set;
	params.SceAuthId = authid;

	params.Capabilities[0] = capab;
	params.Capabilities[1] = capab;
	params.Capabilities[2] = capab;
	params.Capabilities[3] = capab;

	ioctl(mira_dev, (unsigned long)MIRA_GET_PROC_THREAD_CREDENTIALS, &params);

	printf("ChangeAuthID : Changed to %016lx Arg(%p)\n", authid, &params);
}

bool Mira::isAvailable() {
	if (mira_dev < 0)
		return false;

	return true;
}

Mira::~Mira() {
	sceKernelClose(mira_dev);
}

