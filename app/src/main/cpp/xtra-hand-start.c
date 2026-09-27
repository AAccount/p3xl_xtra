#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

// Location client handle used to represent a specific client. Negative values are invalid handles.
typedef void* LocClientHandle;

// Callbacks
typedef void (*EventCallback)(LocClientHandle clientHandle, uint32_t eventId, const void* payload, void* cookie);
typedef void (*ResponseCallback)(LocClientHandle clientHandle, uint32_t responseId, const void* payload, uint32_t payloadSize, void* cookie);
typedef void (*ErrorCallback)(LocClientHandle LocClientHandle, uint32_t errorId, void* cookie);

typedef struct
{
	uint32_t size;
	EventCallback eventCallback;
	ResponseCallback responseCallback;
	ErrorCallback errorCallback;
} LocClientCallbacks;

static void printingEventCallback(LocClientHandle clientHandle, uint32_t eventId, const void* payload, void* cookie)
{
	printf("event callback got code %d\n", eventId);
}

static void printingResponseCallback(LocClientHandle clientHandle, uint32_t responseId, const void* payload, uint32_t payloadSize, void* cookie)
{
    flockfile(stdout);
	printf("response callback got %d of size %d\n", responseId, payloadSize);
	printf("raw payload: [");
	uint8_t* raw = (uint8_t*)payload;
	for(uint32_t i=0; i<payloadSize; i++)
	{
		printf("%02X ", raw[i]);
	}
	printf("]\n");
    funlockfile(stdout);
}

static void printingErrorCallback(LocClientHandle LocClientHandle, uint32_t errorId, void* cookie)
{
	printf("something bad happened: %d\n", errorId);
}

// Xtra chunking
const uint32_t eQMI_LOC_XTRA_DATA_V02 = 0;
const uint32_t QMI_LOC_INJECT_XTRA_DATA_REQ_V02 = 0xa7;
const uint32_t XTRA_REQ_EXPECTED_SIZE = 1044; //0x414
const uint32_t XTRA_SIZE_MAX = UINT32_MAX;
const uint32_t QMI_LOC_MAX_XTRA_PART_LEN_V02 = 1024;
typedef struct
{
	uint32_t totalSize;
	uint16_t totalParts;
	uint16_t partNum;
	uint32_t partData_len;
	uint8_t  partData[QMI_LOC_MAX_XTRA_PART_LEN_V02];

	uint8_t  formatType_valid;
	uint8_t  padding[3];
	uint32_t formatType;
} XtraRequest;

// Location client functions
const uint64_t EVENTS_NONE = 0;
typedef uint32_t (*LocClientOpen)(uint64_t eventMask, LocClientCallbacks* callbacks, LocClientHandle* outputHandle, void* cookie);
typedef uint32_t (*LocClientClose)(LocClientHandle* outputHandle);
typedef uint32_t (*LocClientSendReq)(LocClientHandle handle, uint32_t requestId, XtraRequest* request);

FILE* openXtraFile(const char* path)
{
	FILE* xtraFile = fopen(path, "rb");
	if(xtraFile == NULL)
	{
		perror("can't open xtra download\n");
		exit(1);
	}
	return xtraFile;
}

long getXtraSize(FILE* xtraDownload)
{
	fseek(xtraDownload, 0, SEEK_END);
	const long xtraSize = ftell(xtraDownload);
	if(xtraSize == -1)
	{
		printf("can't skip to the end of the file\n");
		exit(1);
	}
	if(xtraSize > XTRA_SIZE_MAX)
	{
		printf("xtra download size %ld over maximum file size %d\n", xtraSize, XTRA_SIZE_MAX);
		exit(1);
	}
	rewind(xtraDownload);
	return xtraSize;
}

int main()
{
	assert(XTRA_REQ_EXPECTED_SIZE == sizeof(XtraRequest));

	const char* path = "/data/local/tmp/xtra.blob";
	FILE* xtraDownload = openXtraFile(path);
	const long xtraSize = getXtraSize(xtraDownload);
	printf("xtra file size %ld\n", xtraSize);

	const long totalParts = (xtraSize + (QMI_LOC_MAX_XTRA_PART_LEN_V02-1)) / QMI_LOC_MAX_XTRA_PART_LEN_V02; // ceiling function
	if(totalParts > UINT16_MAX)
    {
        printf("total parts %ld exceeds maximum of %d %d sized chunks\n", totalParts, UINT16_MAX, QMI_LOC_MAX_XTRA_PART_LEN_V02);
        return 1;
    }
	printf("total parts %ld\n", totalParts);

	void* libloc_api_v02 = dlopen("/vendor/lib64/libloc_api_v02.so", RTLD_NOW);
	printf("libloc handle %p\n", libloc_api_v02);
	if(libloc_api_v02 == NULL)
    {
        printf("failed to open libloc_api_v02\n");
        return 1;
    }

	LocClientOpen openHandle = dlsym(libloc_api_v02, "locClientOpen");
	LocClientSendReq sendReq = dlsym(libloc_api_v02, "locClientSendReq");
	LocClientClose closeHandle = dlsym(libloc_api_v02, "locClientClose");
    printf("open handle %p send request handle %p close handle %p\n", openHandle, sendReq, closeHandle);
	if(openHandle == 0 || sendReq == 0 || closeHandle == 0)
    {
        printf("failed to find one of the 3 functions\n");
        return 1;
    }

	LocClientCallbacks callbacks = {
			sizeof(LocClientCallbacks),
			printingEventCallback,
			printingResponseCallback,
			printingErrorCallback
	};
	LocClientHandle handle = NULL;
	void* cookie = NULL;
	const uint32_t openStatus = openHandle(EVENTS_NONE, &callbacks, &handle, cookie);
	if(openStatus != 0 && handle == NULL)
	{
		printf("failed to get a handle %d\n", openStatus);
		fclose(xtraDownload);
		dlclose(libloc_api_v02);
		return 1;
	}

	int offset = 0;
	for(int part=1; part<=totalParts; part++)
	{
        const long remainder = xtraSize - offset;
		XtraRequest req;
		memset(req.partData, 0, QMI_LOC_MAX_XTRA_PART_LEN_V02 + sizeof(req.formatType_valid) + sizeof(req.padding));
		req.totalSize = xtraSize;
		req.totalParts = totalParts;
		req.partNum = part;
		req.partData_len = QMI_LOC_MAX_XTRA_PART_LEN_V02 < remainder ? QMI_LOC_MAX_XTRA_PART_LEN_V02 : remainder;
		const size_t bytesRead = fread(req.partData, 1, req.partData_len, xtraDownload); // for binary files use unit size = 1, length = actual length to get an accurate bytes read
		req.formatType_valid = 1;
		req.formatType = eQMI_LOC_XTRA_DATA_V02;
		if(bytesRead != req.partData_len)
		{
			printf("expected to read %d but actually read %zu bytes\n", req.partData_len, bytesRead);
			fclose(xtraDownload);
			dlclose(libloc_api_v02);
			return 1;
		}

		printf("XtraRequest{totalSize: %d, totalParts: %d, partNum: %d, partData_len: %d}\n", req.totalSize, req.totalParts, req.partNum, req.partData_len);
		const uint32_t sendStatus = sendReq(handle, QMI_LOC_INJECT_XTRA_DATA_REQ_V02, &req);
		if(sendStatus != 0)
		{
			printf("expected send return of 0 but got %d\n", sendStatus);
			fclose(xtraDownload);
			dlclose(libloc_api_v02);
			return 1;
		}

		printf("successfully sent\n");
		offset = offset + req.partData_len;
	}

	const uint32_t closeStatus = closeHandle(&handle);
	dlclose(libloc_api_v02);
	fclose(xtraDownload);
	printf("closed handle exit with %d\n", closeStatus);
}

// /home/daniel/Desktop/void/android-ndk-r30/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang -g -Wall -Wextra xtra-hand-start.c -ldl -o xtra-hand-start
// curl -L https://xtrapath2.izatcloud.net/xtra3grcej.bin -o /tmp/xtra3grcej.bin