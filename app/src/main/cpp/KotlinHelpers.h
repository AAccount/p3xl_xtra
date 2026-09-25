//
// Created by daniel on 9/24/26.
//

#ifndef GPS_XTRA_KOTLINHELPERS_H
#define GPS_XTRA_KOTLINHELPERS_H

#include <jni.h>

typedef struct
{
	JavaVM* javaVM;
	jobject globalSelf;
	jmethodID methodId;
} KotlinInfo;
void sendToKotlin(KotlinInfo* kotlinInfo, const char* message);
#endif //GPS_XTRA_KOTLINHELPERS_H
