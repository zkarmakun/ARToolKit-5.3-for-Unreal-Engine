#ifndef TRACKING_SUB_H
#define TRACKING_SUB_H
#pragma once
#include <thread_sub.h> 
#include <KPM/kpm.h>
#include "Engine.h"

#ifdef __cplusplus
extern "C" {
#endif

	THREAD_HANDLE_T *trackingInitInit(KpmHandle *kpmHandle);
	int trackingInitStart(THREAD_HANDLE_T *threadHandle, ARUint8 *imagePtr);
	int trackingInitGetResult(THREAD_HANDLE_T *threadHandle, float trans[3][4], int *page);
	int trackingInitQuit(THREAD_HANDLE_T **threadHandle_p);

#ifdef __cplusplus
}
#endif

#endif // !TRACKING_SUB_H