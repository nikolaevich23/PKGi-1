#include <stdint.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/GnmDriver.h>

#include <proto-include.h>

#ifndef NETWORK_H
#define NETWORK_H

#define CHUCK_SIZE 1024

typedef struct _Request {
	int reqId;
	int connId;
	int tmplId;
} Request;

class Network
{
private:
	int poolID;
	int sslID;
	int httpID;

	Request* CreateRequest(const char* url, int method);
	void CleanupRequest(Request* request);

public:
	Network();
	~Network();
	size_t GetContentSize(const char* url);
	void*  GetRequest(const char* url, size_t* len);
	void*  GetChunkedRequest(const char* url, uint64_t offset, uint64_t len, size_t* dl_lenght);
};

#endif