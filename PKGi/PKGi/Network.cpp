#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <orbis/Net.h>
#include <orbis/Http.h>
#include <orbis/Ssl.h>

#include "App.h"
#include "Network.h"

Network::Network() {
	char poolName[] = "PKGiNetPool";
	poolID = sceNetPoolCreate(poolName, 4096, 0);
	sslID = sceSslInit(SSL_POOLSIZE * 10);
	httpID = sceHttpInit(poolID, sslID, LIBHTTP_POOLSIZE);
	printf("Network initialized\n");
}

Network::~Network() {
	sceHttpTerm(httpID);
	sceSslTerm();

	sceNetPoolDestroy();
	sceNetTerm();
}

Request* Network::CreateRequest(const char* url, int method) {
	Request* request = (Request*)malloc(sizeof(Request));

	char templateName[] = "PKGiNetTmpl";

	char user_agent[50];
	snprintf(user_agent, 50, "PKGi/%i", APP_VER);

	request->tmplId = sceHttpCreateTemplate(httpID, templateName, 1, 0);
	if (request->tmplId < 0) {
		CleanupRequest(request);
		printf("Network: Bad template id (0x%08x)\n", request->tmplId);
		return nullptr;
	}

	// Disable HTTPS Certificate check
	sceHttpsDisableOption(request->tmplId, 0x80 | 0x20 | 0x01);

	request->connId = sceHttpCreateConnectionWithURL(request->tmplId, url, 1);
	if (request->connId < 0) {
		CleanupRequest(request);
		printf("Network: Bad connection id (0x%08x)\n", request->connId);
		return nullptr;
	}

	request->reqId = sceHttpCreateRequestWithURL(request->connId, method, url, 0);
	if (request->reqId < 0) {
		CleanupRequest(request);
		printf("Network: Bad request id (0x%08x)\n", request->reqId); // 0x804311fe
		return nullptr;
	}

	// Max timeout : 5 seconds for reliability
	sceHttpSetConnectTimeOut(request->reqId, 5000000);

	return request;
}

void Network::CleanupRequest(Request* request) {
	sceHttpDeleteRequest(request->reqId);
	sceHttpDeleteConnection(request->connId);
	sceHttpDeleteTemplate(request->tmplId);
	free(request);
}

size_t Network::GetContentSize(const char* url) {
	Request* req = CreateRequest(url, ORBIS_METHOD_GET);

	int ret = sceHttpSendRequest(req->reqId, nullptr, 0);
	if (ret < 0) {
		printf("Network: Request send error (0x%08x)\n", ret);
		return 0;
	}

	int result;
	size_t lenght;
	ret = sceHttpGetResponseContentLength(req->reqId, &result, &lenght);
	if (ret < 0 || result != 0) {
		printf("Network: Content lenght error (ret: 0x%08x result: %i)\n", ret, result);
		CleanupRequest(req);
		return 0;
	}

	CleanupRequest(req);

	return lenght;
}

void* Network::GetRequest(const char* url, size_t* len) {
	Request* req = CreateRequest(url, ORBIS_METHOD_GET);
	
	int ret = sceHttpSendRequest(req->reqId, nullptr, 0);
	if (ret < 0) {
		printf("Network: Request send error (0x%08x)\n", ret);

		return nullptr;
	}

	int result;
	size_t lenght;
	void* reponse = NULL;

	ret = sceHttpGetResponseContentLength(req->reqId, &result, &lenght);

	if (ret < 0 || result != 0) {
		printf("Network: Content lenght error (ret: 0x%08x result: %i)\n", ret, result);
		printf("Network: Download content part by part ...\n");

		reponse = malloc(CHUCK_SIZE);

		lenght = 0;
		int downloaded_len = -1;

		while (downloaded_len == CHUCK_SIZE || downloaded_len == -1) {
			downloaded_len = sceHttpReadData(req->reqId, reponse, CHUCK_SIZE);
			lenght += CHUCK_SIZE;
			reponse = realloc(reponse, lenght + CHUCK_SIZE);
			if (reponse == NULL) {
				printf("Network: Unable to realloc !\n");
				break;
			}
		}

		if (downloaded_len < 0 || reponse == NULL) {
			printf("Network: downloaded_len cannot be < 0 or reponse cannot be null. (downloaded_len: %i reponse: %p)\n", downloaded_len, reponse);

			if (reponse)
				free(reponse);

			reponse = NULL;
			lenght = 0;
		}

		lenght += (size_t)downloaded_len;
	}
	else {
		reponse = malloc(lenght);
		if (reponse) {
			sceHttpReadData(req->reqId, reponse, (unsigned int)lenght);
		}
		else {
			printf("Network: Unable to malloc !\n");
		}
	}

	CleanupRequest(req);

	*len = lenght;

	return reponse;
}

void* Network::GetChunkedRequest(const char* url, uint64_t offset, uint64_t len, size_t* dl_lenght) {
	Request* req = CreateRequest(url, ORBIS_METHOD_GET);

	// Add chunck header
	char range_str[48];
	snprintf(range_str, sizeof(range_str), "bytes=%lu-%lu", offset, offset + len - 1);
	sceHttpAddRequestHeader(req->reqId, "Range", range_str, 0);

	// Send request
	int ret = sceHttpSendRequest(req->reqId, nullptr, 0);
	if (ret < 0) {
		printf("Network: Request send error (0x%08x)\n", ret);
		return nullptr;
	}

	// Save data
	int final_lenght = 0;
	void* reponse = malloc(len);
	if (reponse) {
		final_lenght = sceHttpReadData(req->reqId, reponse, (unsigned int)len);
	}
	else {
		printf("Network: Unable to malloc !\n");
	}

	printf("Network: Downloaded: %i bytes\n", final_lenght);
	*dl_lenght = (size_t)final_lenght;

	CleanupRequest(req);

	return reponse;
}