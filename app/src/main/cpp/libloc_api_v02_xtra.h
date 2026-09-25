//
// Created by daniel on 9/24/26.
//

#ifndef GPS_XTRA_LIBLOC_API_V02_XTRA_H
#define GPS_XTRA_LIBLOC_API_V02_XTRA_H

#include <stdint.h>

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

#endif //GPS_XTRA_LIBLOC_API_V02_XTRA_H
