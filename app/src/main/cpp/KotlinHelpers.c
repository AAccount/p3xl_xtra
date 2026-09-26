//
// Created by daniel on 9/24/26.
//
#include <stdbool.h>
#include "KotlinHelpers.h"

void sendToKotlin(KotlinInfo* kotlinInfo, const char* message)
{
	JNIEnv* env = NULL;
	JavaVM* jvm = kotlinInfo->javaVM;
	const int getEnvResult = (*jvm)->GetEnv(jvm, (void**)(&env), JNI_VERSION_1_6);
	bool needDetach = false;

	if(getEnvResult == JNI_EDETACHED)
	{
		if((*jvm)->AttachCurrentThread(jvm, &env, NULL) != JNI_OK)
		{
			return;
		}
		needDetach = true;
	}
	else if(getEnvResult != JNI_OK)
	{
		return;
	}


	jstring jmessage = (*env)->NewStringUTF(env, message);

	(*env)->CallVoidMethod(env, kotlinInfo->globalSelf, kotlinInfo->methodId, jmessage);
	(*env)->DeleteLocalRef(env, jmessage);
	if(needDetach)
	{
		(*jvm)->DetachCurrentThread(jvm);
	}
}