
#include "ARPrivatePCH.h"
#include "trackingSub.h"

DEFINE_LOG_CATEGORY_STATIC(LogARToolKitThread, Log, All);

typedef struct {
	KpmHandle              *kpmHandle;      // KPM-related data.
	ARUint8                *imagePtr;       // Pointer to image being tracked.
	int                     imageSize;      // Bytes per image.
	float                   trans[3][4];    // Transform containing pose of tracked image.
	int                     page;           // Assigned page number of tracked image.
	int                     flag;           // Tracked successfully.
} TrackingInitHandle;

static void *trackingInitMain(THREAD_HANDLE_T *threadHandle);


int trackingInitQuit(THREAD_HANDLE_T **threadHandle_p)
{
	TrackingInitHandle  *trackingInitHandle;

	if (!threadHandle_p) {
		UE_LOG(LogARToolKitThread, Error, TEXT("trackingInitQuit(): Error: NULL threadHandle_p.\n"));
		return (-1);
	}
	if (!*threadHandle_p) return 0;

	threadWaitQuit(*threadHandle_p);
	trackingInitHandle = (TrackingInitHandle *)threadGetArg(*threadHandle_p);
	if (trackingInitHandle) {
		free(trackingInitHandle->imagePtr);
		free(trackingInitHandle);
	}
	threadFree(threadHandle_p);
	return 0;
}

THREAD_HANDLE_T *trackingInitInit(KpmHandle *kpmHandle)
{
	TrackingInitHandle  *trackingInitHandle;
	THREAD_HANDLE_T     *threadHandle;

	if (!kpmHandle) {
		UE_LOG(LogARToolKitThread, Error, TEXT("trackingInitInit(): Error: NULL KpmHandle.\n"));
		return (NULL);
	}

	trackingInitHandle = (TrackingInitHandle *)malloc(sizeof(TrackingInitHandle));
	if (trackingInitHandle == NULL) return NULL;
	trackingInitHandle->kpmHandle = kpmHandle;
	trackingInitHandle->imageSize = kpmHandleGetXSize(kpmHandle) * kpmHandleGetYSize(kpmHandle) * arUtilGetPixelSize(kpmHandleGetPixelFormat(kpmHandle));
	trackingInitHandle->imagePtr = (ARUint8 *)malloc(trackingInitHandle->imageSize);
	trackingInitHandle->flag = 0;

	threadHandle = threadInit(0, trackingInitHandle, trackingInitMain);
	return threadHandle;
}

int trackingInitStart(THREAD_HANDLE_T *threadHandle, ARUint8 *imagePtr)
{
	TrackingInitHandle     *trackingInitHandle;

	if (!threadHandle || !imagePtr) {
		UE_LOG(LogARToolKitThread, Error, TEXT("trackingInitStart(): Error: NULL threadHandle or imagePtr.\n"));
		ARLOGe("trackingInitStart(): Error: NULL threadHandle or imagePtr.\n");
		return (-1);
	}

	trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
	if (!trackingInitHandle) {
		UE_LOG(LogARToolKitThread, Error, TEXT("trackingInitStart(): Error: NULL trackingInitHandle.\n"));
		ARLOGe("trackingInitStart(): Error: NULL trackingInitHandle.\n");
		return (-1);
	}
	memcpy(trackingInitHandle->imagePtr, imagePtr, trackingInitHandle->imageSize);
	threadStartSignal(threadHandle);

	return 0;
}

int trackingInitGetResult(THREAD_HANDLE_T *threadHandle, float trans[3][4], int *page)
{
	TrackingInitHandle     *trackingInitHandle;
	int  i, j;

	if (!threadHandle || !trans || !page) {
		UE_LOG(LogARToolKitThread, Error, TEXT("trackingInitGetResult(): Error: NULL threadHandle or trans or page.\n"));
		ARLOGe("trackingInitGetResult(): Error: NULL threadHandle or trans or page.\n");
		return (-1);
	}

	if (threadGetStatus(threadHandle) == 0) return 0;
	threadEndWait(threadHandle);
	trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
	if (!trackingInitHandle) return (-1);
	if (trackingInitHandle->flag) {
		for (j = 0; j < 3; j++) for (i = 0; i < 4; i++) trans[j][i] = trackingInitHandle->trans[j][i];
		*page = trackingInitHandle->page;
		return 1;
	}

	return -1;
}

static void *trackingInitMain(THREAD_HANDLE_T *threadHandle)
{
	TrackingInitHandle     *trackingInitHandle;
	KpmHandle              *kpmHandle;
	KpmResult              *kpmResult = NULL;
	int                     kpmResultNum;
	ARUint8                *imagePtr;
	float                  err;
	int                    i, j, k;

	if (!threadHandle) {
		UE_LOG(LogARToolKitThread, Error, TEXT("Error starting tracking thread: empty THREAD_HANDLE_T.\n"));
		ARLOGe("Error starting tracking thread: empty THREAD_HANDLE_T.\n");
		return (NULL);
	}
	trackingInitHandle = (TrackingInitHandle *)threadGetArg(threadHandle);
	if (!threadHandle) {
		UE_LOG(LogARToolKitThread, Error, TEXT("Error starting tracking thread: empty trackingInitHandle.\n"));
		ARLOGe("Error starting tracking thread: empty trackingInitHandle.\n");
		return (NULL);
	}
	kpmHandle = trackingInitHandle->kpmHandle;
	imagePtr = trackingInitHandle->imagePtr;
	if (!kpmHandle || !imagePtr) {
		UE_LOG(LogARToolKitThread, Error, TEXT("Error starting tracking thread: empty kpmHandle/imagePtr.\n"));
		ARLOGe("Error starting tracking thread: empty kpmHandle/imagePtr.\n");
		return (NULL);
	}
	ARLOGi("Start tracking thread.\n");

	kpmGetResult(kpmHandle, &kpmResult, &kpmResultNum);

	for (;;) {
		if (threadStartWait(threadHandle) < 0) break;

		kpmMatching(kpmHandle, imagePtr);
		trackingInitHandle->flag = 0;
		for (i = 0; i < kpmResultNum; i++) {
			if (kpmResult[i].camPoseF != 0) continue;
			ARLOGd("kpmGetPose OK.\n");
			if (trackingInitHandle->flag == 0 || err > kpmResult[i].error) { // Take the first or best result.
				trackingInitHandle->flag = 1;
				trackingInitHandle->page = kpmResult[i].pageNo;
				for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) trackingInitHandle->trans[j][k] = kpmResult[i].camPose[j][k];
				err = kpmResult[i].error;
			}
		}

		threadEndSignal(threadHandle);
	}

	ARLOGi("End tracking thread.\n");
	return (NULL);
}
