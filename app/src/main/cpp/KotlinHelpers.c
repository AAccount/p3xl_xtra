//
// Created by daniel on 9/24/26.
//
#include "KotlinHelpers.h"

void sendToKotlin(KotlinInfo* kotlinInfo, const char* message)
{
	JNIEnv* env = NULL;
	JavaVM* jvm = kotlinInfo->javaVM;
	if((*jvm)->GetEnv(jvm, (void**)(&env), JNI_VERSION_1_6) != JNI_OK)
	{
		return;
	}

	if((*jvm)->AttachCurrentThread(jvm, &env, NULL) != JNI_OK)
	{
		return;
	}

	jstring jmessage = (*env)->NewStringUTF(env, message);
	(*env)->CallVoidMethod(kotlinInfo->globalSelf, kotlinInfo->methodId, jmessage);
	(*env)->DeleteLocalRef(env, jmessage);
}