#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <jni.h>

#include "KotlinHelpers.h"
#include "libloc_api_v02_xtra.h"

static void printingEventCallback(LocClientHandle clientHandle, uint32_t eventId, const void* payload, void* cookie)
{
	if(!cookie)
	{
		return;
	}

	char message[64];
	snprintf(message, sizeof(message), "event callback got code %d\n", eventId);
	sendToKotlin((KotlinInfo*)cookie, message);
}

static void printingResponseCallback(LocClientHandle clientHandle, uint32_t responseId, const void* payload, uint32_t payloadSize, void* cookie)
{
	if(!cookie)
	{
		return;
	}

	const uint32_t BUFFER_SIZE = 1024;
	char bigbuffer[BUFFER_SIZE] = {0};
	char header[64] = {0};
	snprintf(header, sizeof(header), "response callback got %d of size %d\nraw payload: []", responseId, payloadSize);
	snprintf(bigbuffer, BUFFER_SIZE, "%s", header);
	uint32_t offset = strlen(header);

	uint8_t* raw = (uint8_t*)payload;
	for(uint32_t i=0; i<payloadSize; i++)
	{
		snprintf(bigbuffer+offset, BUFFER_SIZE - offset, "%02X ", raw[i]);
		offset = offset + 3; // 3 from %02X(space): ##_
	}
	snprintf(bigbuffer+offset, BUFFER_SIZE - offset, "%s", "]\n");
	sendToKotlin((KotlinInfo*)cookie, bigbuffer);
}

static void printingErrorCallback(LocClientHandle LocClientHandle, uint32_t errorId, void* cookie)
{
	if(!cookie)
	{
		return;
	}

	char message[64];
	snprintf(message, sizeof(message), "something bad happened: %d\n", errorId);
	sendToKotlin((KotlinInfo*)cookie, message);
}

JNIEXPORT void JNICALL
Java_dt_gpsxtra_LibLocAPI2Service_inject(JNIEnv* env, jobject self, jstring xtrbin_path)
{
	jclass counterpart = (*env)->GetObjectClass(env, self);
	jmethodID debugFromC = (*env)->GetMethodID(env, counterpart, "debugFromC", "(Ljava/lang/String;)V");
	JavaVM* jvm = NULL;
	(*env)->GetJavaVM(env, &jvm);
	jobject globalSelf = (*env)->NewGlobalRef(env, self);
	KotlinInfo* kotlinInfo = malloc(sizeof(KotlinInfo));
	kotlinInfo->javaVM = jvm;
	kotlinInfo->globalSelf = self;
	kotlinInfo->methodId = debugFromC;

	const char* cpath = (*env)->GetStringUTFChars(env, xtrbin_path, JNI_FALSE);

	if(XTRA_REQ_EXPECTED_SIZE != sizeof(XtraRequest))
	{
		char error[64] = {0};
		snprintf(error, sizeof(error), "XtraRequest is size %d but should be %d\n", sizeof(XtraRequest), XTRA_REQ_EXPECTED_SIZE);
		sendToKotlin(kotlinInfo, error);
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	FILE* xtraFile = fopen(cpath, "rb");
	if(xtraFile == NULL)
	{
		sendToKotlin(kotlinInfo, "can't open xtra download\n");
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	fseek(xtraFile, 0, SEEK_END);
	const long xtraSize = ftell(xtraFile);
	if(xtraSize == -1)
	{
		sendToKotlin(kotlinInfo, "can't skip to the end of the file\n");
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	char msgXtraSize[64] = {0};
	snprintf(msgXtraSize, sizeof(msgXtraSize), "xtra file size %d\n", xtraSize);
	sendToKotlin(kotlinInfo, msgXtraSize);
	if(xtraSize > XTRA_SIZE_MAX)
	{
		sendToKotlin(kotlinInfo, "xtra download over maximum file size uint32\n");
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}
	rewind(xtraFile);

	const uint32_t totalParts = (xtraSize + (QMI_LOC_MAX_XTRA_PART_LEN_V02-1)) / QMI_LOC_MAX_XTRA_PART_LEN_V02; // ceiling function
	char msgTotalParts[64] = {0};
	snprintf(msgTotalParts, sizeof(msgTotalParts), "total parts: %d\n", totalParts);
	sendToKotlin(kotlinInfo, msgTotalParts);
	if(totalParts > UINT16_MAX)
	{
		sendToKotlin(kotlinInfo, "total parts more than a uint16 can represent\n");
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	void* libloc_api_v02 = dlopen("/vendor/lib64/libloc_api_v02.so", RTLD_NOW);
	if(libloc_api_v02 == NULL)
	{
		sendToKotlin(kotlinInfo, "failed to open vendor library\n");
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	LocClientOpen openHandle = dlsym(libloc_api_v02, "locClientOpen");
	LocClientSendReq sendReq = dlsym(libloc_api_v02, "locClientSendReq");
	LocClientClose closeHandle = dlsym(libloc_api_v02, "locClientClose");
	char dlsyms[128] = {0};
	snprintf(dlsyms, sizeof(dlsyms), "openHandle %p, sendHandle %p, closeHandle%p\n", openHandle, sendReq, closeHandle);
	sendToKotlin(kotlinInfo, dlsyms);
	if(openHandle == 0 || sendReq == 0 || closeHandle == 0)
	{
		sendToKotlin(kotlinInfo, "failed to open one of the 3 functions");
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	LocClientCallbacks callbacks = {
			sizeof(LocClientCallbacks),
			printingEventCallback,
			printingResponseCallback,
			printingErrorCallback
	};
	LocClientHandle handle = NULL;
	const uint32_t openStatus = openHandle(EVENTS_NONE, &callbacks, &handle, &kotlinInfo);
	char openStatusMsg[64] = {0};
	snprintf(openStatusMsg, sizeof(openStatusMsg), "open status %d, loc client handle %p\n", openStatus, handle);
	sendToKotlin(kotlinInfo, openStatusMsg);
	if(openStatus != 0 || handle == NULL)
	{
		sendToKotlin(kotlinInfo, "failed to open a lib loc client handle\n");
		fclose(xtraFile);
		dlclose(libloc_api_v02);
		(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
		free(kotlinInfo);
		return;
	}

	uint32_t offset = 0;
	for(int part=1; part<=totalParts; part++)
	{
		XtraRequest req;
		const uint32_t  remainder = xtraSize - offset;
		memset(req.partData, 0, QMI_LOC_MAX_XTRA_PART_LEN_V02 + sizeof(req.formatType_valid) + sizeof(req.padding));
		req.totalSize = xtraSize;
		req.totalParts = (uint16_t)totalParts;
		req.partNum = part;
		req.partData_len = QMI_LOC_MAX_XTRA_PART_LEN_V02 < remainder ? QMI_LOC_MAX_XTRA_PART_LEN_V02 : remainder;
		const uint32_t bytesRead = fread(req.partData, 1, req.partData_len, xtraFile); // for binary files use unit size = 1, legnth = actual length to get an accurate bytes read
		req.formatType_valid = 1;
		req.formatType = eQMI_LOC_XTRA_DATA_V02;
		if(bytesRead != req.partData_len)
		{
			char readError[64] = {0};
			snprintf(readError, sizeof(readError), "expected to read %d but actually read %d bytes\n", req.partData_len, bytesRead);
			sendToKotlin(kotlinInfo, readError);
			fclose(xtraFile);
			dlclose(libloc_api_v02);
			(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
			free(kotlinInfo);
			return;
		}

		char requestInfo[128] = {0};
		snprintf(requestInfo,sizeof (requestInfo), "XtraRequest{totalSize: %d, totalParts: %d, partNum: %d, partData_len: %d}\n", req.totalSize, req.totalParts, req.partNum, req.partData_len);
		sendToKotlin(kotlinInfo, requestInfo);
		const uint32_t sendStatus = sendReq(handle, QMI_LOC_INJECT_XTRA_DATA_REQ_V02, &req);
		if(sendStatus != 0)
		{
			char sendFail[64] = {0};
			snprintf(sendFail, sizeof(sendFail), "expected send return of 0 but got %d\n", sendStatus);
			sendToKotlin(kotlinInfo, sendFail);
			fclose(xtraFile);
			dlclose(libloc_api_v02);
			(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
			free(kotlinInfo);
			return;
		}

		sendToKotlin(kotlinInfo, "successfully sent\n");
		offset = offset + req.partData_len;
	}

	const uint32_t closeStatus = closeHandle(&handle);
	dlclose(libloc_api_v02);
	fclose(xtraFile);
	char closeMsg[64] = {0};
	snprintf(closeMsg, sizeof(closeMsg), "closed handle exit with %d\n", closeStatus);
	sendToKotlin(kotlinInfo, closeMsg);
	(*env)->ReleaseStringUTFChars(env, xtrbin_path, cpath);
	free(kotlinInfo);
}