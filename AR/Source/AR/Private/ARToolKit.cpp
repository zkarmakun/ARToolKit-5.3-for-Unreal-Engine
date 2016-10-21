/*
*  ARToolKit.cpp
*  ARToolKit5
*
*  This file encapsulate the core funcionallity
*  File description: ARToolKit class exposes all the basic funcionallity for AR
*  this never call directly, is just a unique TSharedRef<> inside the sigleton module (IAR.cpp)
*
*  ARToolKit is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  ARToolKit is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
*
*  As a special exception, the copyright holders of this library give you
*  permission to link this library with independent modules to produce an
*  executable, regardless of the license terms of these independent modules, and to
*  copy and distribute the resulting executable under terms of your choice,
*  provided that you also meet, for each linked independent module, the terms and
*  conditions of the license of that module. An independent module is a module
*  which is neither derived from nor based on this library. If you modify this
*  library, you may extend this exception to your version of the library, but you
*  are not obligated to do so. If you do not wish to do so, delete this exception
*  statement from your version.
*
*  Copyright 2015 Daqri, LLC.
*  Copyright 2010-2015 ARToolworks, Inc.
*
*  Author(s): Philip Lamb, Julian Looser.
*
*  Plugin Author(s): Jorge CR (AKA karmakun)
*
*  Special Thanks: tgraupmann (your android knowledge helps a lot)
*/

#include "ARPrivatePCH.h"
#include "ARToolKit.h"
#include "ARTarget.h"
#include "ARPawn.h"

//-- DEBUG MACROS-----------------------------------
/**  Use just for debug purposes, anyway shipping build disable it*/
DEFINE_LOG_CATEGORY_STATIC(LogARToolKit, Log, All);
#define print(txt) GEngine->AddOnScreenDebugMessage(-1,10,FColor::Green, txt)
#define error(txt) GEngine->AddOnScreenDebugMessage(-1,10,FColor::Red, txt)
#define warning(txt) GEngine->AddOnScreenDebugMessage(-1,10,FColor::Yellow, txt)

//--- ANDROID JAVA CUSTOM CALLS-------------------------
#if PLATFORM_ANDROID
#include <android/log.h>
/** Start camera from java code, from UPL system, only default resolution value, meaning poor quality for camera device*/
extern void AndroidThunkCpp_startCamera();
/** Stop camera from java code, from UPL system*/
extern void AndroidThunkCpp_stopCamera();
/** Check if file exist just android, DEPRECATED*/
extern bool AndroidThunkCpp_AFileExist(const FString& path);
/** Whenever a new frame is arrived and need to be procedded*/
extern bool newFrame;
/** Get Raw Data camera info, by instance YUV420*/
extern unsigned char* rawDataAndroid;
#endif


//---- PER PLATFORM MACROS------------------
#if PLATFORM_WINDOWS
#define _WIN_
#elif PLATFORM_MAC
#define _MAC_
#elif PLATFORM_ANDROID
#define _ANDROID_
#elif PLATFORM_IOS
#define _IOS_
#endif

/** Start only in Shared Ref module sinleton*/
ARToolKit::ARToolKit() :
	pixelFormat(AR_PIXEL_FORMAT_INVALID),
	threadHandle(NULL),
	ar2Handle(NULL),
	kpmHandle(NULL),
	surfaceSetCount(0)
{
	for (int i = 0; i < PAGES_MAX; i++) surfaceSet[i] = NULL;
}

ARToolKit::~ARToolKit()
{
	shutdownAR();
}


int ARToolKit::setupCamera(FString camera_config, FString camera_parameter)
{
	ARParam			cparam;
#if defined _WIN_ || defined _MAC_ || defined _IOS_
	//try to access the camera
	if (arVideoOpen(TCHAR_TO_ANSI(*camera_config)))
	{
        error("initializeAR(): Unable to open connection to camera");
		UE_LOG(LogARToolKit, Warning, TEXT("initializeAR(): Unable to open connection to camera"));
		return 0;
	}

	//get width and height of this camera
	if (arVideoGetSize(&width, &height) < 0) {
		error("initializeAR():  Unable to determine camera frame size.");
		UE_LOG(LogARToolKit, Warning, TEXT("initializeAR():  Unable to determine camera frame size."));
		arVideoClose();
		return 0;
	}

	UE_LOG(LogARToolKit, Log, TEXT("Camera image size (x,y) = %i , %i"), width, height);

	// Get the format in which the camera is returning pixels.
	pixelFormat = arVideoGetPixelFormat();
	if (pixelFormat == AR_PIXEL_FORMAT_INVALID) {
		error("initializeAR(): Camera is using unsupported pixel format");
		UE_LOG(LogARToolKit, Warning, TEXT("initializeAR(): Camera is using unsupported pixel format"));
		arVideoClose();
		return 0;
	}
#endif

	if (arParamLoad(TCHAR_TO_UTF8(*camera_parameter), 1, &cparam) < 0) {
		error("initializeAR(): Error loading parameter file for camera");
		UE_LOG(LogARToolKit, Warning, TEXT("initializeAR(): Error loading parameter file for camera"));
		return 0;
	}
	
#if defined _ANDROID_
	pixelFormat = AR_PIXEL_FORMAT_NV21;
	width = 320;
	height = 240;
#endif

	arParamChangeSize(&cparam, width, height, &cparam);

	if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
		error("initializeAR():  Error: arParamLTCreate.");
		UE_LOG(LogARToolKit, Warning, TEXT("initializeAR():  Error: arParamLTCreate."));
		return 0;
	}

	return 1;
}

// Modifies globals: kpmHandle, ar2Handle.
int ARToolKit::initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat)
{
	
	UE_LOG(LogARToolKit, Log, TEXT("Initialising NFT."));
	//
	// NFT init.
	
	// KPM init.
	kpmHandle = kpmCreateHandle(cparamLT, pixelFormat);
	if (!kpmHandle) {
		error("initNFT() Error: kpmCreateHandle.");
		UE_LOG(LogARToolKit, Error, TEXT("initNFT() Error: kpmCreateHandle."));
		return 0;
	}

	// AR2 init.
	if ((ar2Handle = ar2CreateHandle(cparamLT, pixelFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL) {
		error("initNFT() Error: ar2CreateHandle.");
		UE_LOG(LogARToolKit, Error, TEXT("initNFT() Error: ar2CreateHandle."));
		kpmDeleteHandle(&kpmHandle);
		return 0;
	}
	if (threadGetCPU() <= 1) {
		UE_LOG(LogARToolKit, Log, TEXT("Using NFT tracking settings for a single CPU."));
		ar2SetTrackingThresh(ar2Handle, 5.0);
		ar2SetSimThresh(ar2Handle, 0.50);
		ar2SetSearchFeatureNum(ar2Handle, 16);
		ar2SetSearchSize(ar2Handle, 6);
		ar2SetTemplateSize1(ar2Handle, 6);
		ar2SetTemplateSize2(ar2Handle, 6);
	}
	else {
		UE_LOG(LogARToolKit, Log, TEXT("Using NFT tracking settings for more than one CPU."));
		ar2SetTrackingThresh(ar2Handle, 5.0);
		ar2SetSimThresh(ar2Handle, 0.50);
		ar2SetSearchFeatureNum(ar2Handle, 16);
		ar2SetSearchSize(ar2Handle, 12);
		ar2SetTemplateSize1(ar2Handle, 6);
		ar2SetTemplateSize2(ar2Handle, 6);
	}
	// NFT dataset loading will happen later.
	nftMultiMode = true;
	kpmRequired = true;
	kpmBusy = false;
	return 1;
}

void ARToolKit::unloadNFT()
{
	//free tracking handle
	int i, j;
	if (threadHandle)
	{
		UE_LOG(LogARToolKit, Warning, TEXT("Stopping NFT2 tracking thread."));
		trackingInitQuit(&threadHandle);
		kpmBusy = false;
		threadHandle = nullptr;
	}

	//free NFT data, and reset vars
	j = 0;
	for (i = 0; i < surfaceSetCount; i++) {
		if (j == 0) UE_LOG(LogARToolKit, Log, TEXT("Unloading NFT tracking surfaces."));
		ar2FreeSurfaceSet(&surfaceSet[i]); // Also sets surfaceSet[i] to NULL.
		surfaceSet[i] = NULL;
		j++;
	}
	if (j > 0) UE_LOG(LogARToolKit, Log, TEXT("Unloaded %d NFT tracking surfaces."), j);
	surfaceSetCount = 0;
	kpmRequired = true;
}

int ARToolKit::loadNFT(TArray<AARTarget*> targetsFound)
{
	
	
	// If data was already loaded, stop KPM tracking thread and unload previously loaded data.
	if (threadHandle)
	{
		warning("Reloading NFT data.");
		UE_LOG(LogARToolKit, Log, TEXT("Reloading NFT data."));
		unloadNFT();
	}
	KpmRefDataSet *refDataSet = NULL;
	refDataSet = NULL;

	//if we found some targets in world, process
	if (targetsFound.Num() > 0)
	{
		for (auto& target : targetsFound)
		{
			//discard same name
			if(targets.Num() > 0)
			{
				bool repeat = false;
				for(auto& prevTarget : targets)
				{
					if(prevTarget->targetName == target->targetName)
					{
						repeat = true;
						break;
					}
				}
				if (repeat)
				{
					UE_LOG(LogARToolKit, Warning, TEXT("this target is load before, so this target will be disard, %s"), *target->targetName);
					continue;
				}
			}			
			
			KpmRefDataSet  *refDataSet2;
			FString fullName = contentDirectory + "/" + target->targetName;
			
			//load fset3 data from path
			if (kpmLoadRefDataSet(TCHAR_TO_ANSI(*fullName), "fset3", &refDataSet2) < 0)
			{
				error("Error reading data fset3");
				UE_LOG(LogARToolKit, Error, TEXT("Error reading data fset3: %s"), *target->targetName);
				continue;
			}
			
			//new reference page for KPM
			if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0)
			{
				error("Error: kpmChangePageNoOfRefDataSet");
				UE_LOG(LogARToolKit, Error, TEXT("Error: kpmChangePageNoOfRefDataSet"));
				continue;
			}
			
			//get the width and height from this target before marge all Kpm data, otherway is destroy and gone forever-ever-ever-eeer!!!
			int32 referenceWidth = refDataSet2->pageInfo->imageInfo->width;
			int32 referenceHeight = refDataSet2->pageInfo->imageInfo->height;

			//marge old reference with this new one
			if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
				error("Error: kpmMergeRefDataSet");
				UE_LOG(LogARToolKit, Error, TEXT("Error: kpmMergeRefDataSet"));
				continue;
			}

			
			// Load AR2 data.
			if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(TCHAR_TO_ANSI(*fullName), "fset", NULL)) == NULL) {
				error("Error reading data from %s.fset");
				UE_LOG(LogARToolKit, Error, TEXT("Error reading data from %s.fset"));
			}

			// load new target NFT success
			target->ID = surfaceSetCount;
			target->filterSampleRate = AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT;
			target->filterCutoffFrequency = AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT;
			target->ftmi = arFilterTransMatInit(target->filterSampleRate, target->filterCutoffFrequency);
			target->valid = false;
			target->validPrev = false;

			surfaceSetCount++;
			targets.Add(target);

			//calculate a offset because NFT track in coord 0,0 from the target size, so we need the middle in both x and y coord
			float halftargetSizeX = pixels2millimeter(referenceWidth) / 2.f;
			float halftargetSizeY = pixels2millimeter(referenceHeight) / 2.f;
			target->offset = FVector(halftargetSizeX,-halftargetSizeY,0);
			
			UE_LOG(LogARToolKit, Log, TEXT("Load new target index: %i , name: %s"), target->ID, *target->targetName);

			if (surfaceSetCount == PAGES_MAX)
			{
				break;
			}
		}
	
		if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0)
		{
			error("Error: kpmSetRefDataSet");
			UE_LOG(LogARToolKit, Error, TEXT("Error: kpmSetRefDataSet"));
			return 0;
		}
		kpmDeleteRefDataSet(&refDataSet);

		// Start the KPM tracking thread.
		threadHandle = trackingInitInit(kpmHandle);
		if (!threadHandle)
		{
			error("no tracking handle available, unloading Targets");
			UE_LOG(LogARToolKit, Error, TEXT("no tracking handle available, unloading Targets"));
			return 0;
		}
		else
		{
			UE_LOG(LogARToolKit, Warning, TEXT("thread handle success!"));
		}

		//if return true, means almost one target has been loaded success!
		return 1;
	}
	else
	{
		warning("not targets found in this world");
		UE_LOG(LogARToolKit, Warning, TEXT("not targets found in this world"));
		return 0;
	}
}

void ARToolKit::initializeAR(ECameraSelection cameraSelection, AARPawn* newPawn, TArray<AARTarget*> targetFound)
{
	//base content directory for windows and mac
	contentDirectory = FPaths::GameContentDir() + "/AR";

#if defined _ANDROID_
	//content directory for Android, GExternialFilePath is a global var inside UE pointing to content, PCH hijack as extern variable
	contentDirectory = GExternalFilePath;
#elif defined _IOS_
	//content directory for IOS
    contentDirectory = FPaths::ConvertRelativePathToFull(ConvertToIOSPath(FString::Printf(TEXT("%s"), FApp::GetGameName()).ToLower() + FString("/content/AR"), 0));
#endif

	//-- Choose camera index process by switch
	int32 index = 1;
	FString indexM = "back";

#if defined _WIN_ || defined _MAC_
	index = (int32)cameraSelection;
	index++;
#elif defined _IOS_
	switch (cameraSelection)
	{
	case ECameraSelection::DEFAULT:
		indexM = "back";
		break;
	case ECameraSelection::FRONTAL_CAMERA:
		indexM = "front";
		break;
	case ECameraSelection::BACK_CAMERA:
		indexM = "back";
		break;
	default:
		break;
	}
#endif


	// set current pawn
	currentPawn = newPawn;

	// set camera string configurator, based windows
	FString camera_config = "-device=WinDS -devNum=" + FString::FromInt(index) + " -flipV";
    
#if defined _MAC_
	/ set camera string configurator, based mac
    camera_config = "-width=640 -height=480 -nodialog";
#elif defined _IOS_
	/ set camera string configurator, based IOS
    camera_config = "-device=iPhone -format=BGRA -preset=cif -position=" + indexM;
#endif

	
	//camera instrisics file path
	FString camera_parameters = contentDirectory + "/camera_para.dat";

	//setup camera
	if (!setupCamera(camera_config, camera_parameters))
	{
		UE_LOG(LogARToolKit, Error, TEXT("Unable to setup camera!"));
		//if not camera available return dummy
		texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("/AR/icon128")));
		texture->UpdateResource();
		rawData.Init(FColor(255, 0, 0, 255), 2*2);
		return;
	}

	//if camera open at this point, then plugin become valid
	isValid = true;

	//init NFT
	if (!initNFT(gCparamLT, pixelFormat))
	{
		error("Unable to init NFT.");
		return;
	}

	//load NFT
	if (!loadNFT(targetFound))
	{
		error("Unable to load NFT.");
		return;
	}

	//create webcam or device texture
	texture = UTexture2D::CreateTransient(width, height, PF_B8G8R8A8);
	echoUpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, width, height);
	texture->UpdateResource();
	rawData.Init(FColor(255, 0, 0, 255), width*height);

}

void ARToolKit::shutdownAR()
{

    UE_LOG(LogARToolKit, Log, TEXT("camera shutdown!"));
	//reset target array
	targets.Empty();

	//unload and destroy all Handles
	unloadNFT();
	ar2DeleteHandle(&ar2Handle);
	ar2Handle = NULL;
	kpmDeleteHandle(&kpmHandle);
	kpmHandle = NULL;
	//arParamLTFree(&gCparamLT);
	//gCparamLT = NULL;
	
	//reset raw data array
	rawData.Empty();

	//become invalid
	isValid = false;

	//stop all camera streams
#if defined _WIN_ || defined _MAC_ || defined _IOS_
	arVideoCapStop();
	arVideoClose();
#elif defined _ANDROID_
	AndroidThunkCpp_stopCamera();
#endif

	UE_LOG(LogARToolKit, Log, TEXT("ARToolKit shutdown!"));
}

UTexture2D* ARToolKit::getTexture()
{
	return texture;
}

FTransform ARToolKit::getTargetTransformations()
{
	return targetTrans;
}

int32 ARToolKit::getWidth()
{
	return width;
}

int32 ARToolKit::getheight()
{
	return height;
}

void ARToolKit::playPreview()
{
	//video stream only start if is valid and we have no null pointer texture
	if (isValid)
	{
		if (texture)
		{
#if defined _WIN_ || defined _MAC_ || defined _IOS_
			if (arVideoCapStart() != 0)
			{
				error("playPreview() Unable to begin camera data capture");
				UE_LOG(LogARToolKit, Error, TEXT("playPreview() Unable to begin camera data capture"));
			}
			//
#elif defined _ANDROID_
			AndroidThunkCpp_startCamera();
#endif
			isValid = true;
			UE_LOG(LogARToolKit, Log, TEXT("playPreview() camera begin capture!!!"));
		}

	}
}

void ARToolKit::detect(unsigned char* data)
{
	if (data == NULL) return;

	if (targets.Num() > 0)
	{

		if (threadHandle)
		{
			
			float err;
			float trackingTrans[3][4];
			if (kpmRequired && pageNo == -1)
			{
				if (!kpmBusy)
				{
					trackingInitStart(threadHandle, data);
					kpmBusy = true;
				}
				else
				{
					int ret;
					ret = trackingInitGetResult(threadHandle, trackingTrans, &pageNo);
					if (ret != 0)
					{
						kpmBusy = false;
						if (ret == 1)
						{
							if (pageNo >= 0 && pageNo < PAGES_MAX)
							{
								//Target become visible
								if (surfaceSet[pageNo]->contNum < 1)
								{
									currentPawn->execOnTargetFound(targets[pageNo]->ID, targets[pageNo]->targetName);
									targets[pageNo]->setTargetActorsHiddenInGame(false);
									targets[pageNo]->valid = true;
									targets[pageNo]->OnTargetFound();
									UE_LOG(LogARToolKit, Log, TEXT("target found with ID: %i, Name: %s"), targets[pageNo]->ID, *targets[pageNo]->targetName);
									ar2SetInitTrans(surfaceSet[pageNo], trackingTrans);
								}
							}
						}
					}

				}
			}

			if (pageNo != -1)
			{
				//Target lost
				if (ar2Tracking(ar2Handle, surfaceSet[pageNo], data, trackingTrans, &err) < 0)
				{
					currentPawn->execOnTargetLost(targets[pageNo]->ID, targets[pageNo]->targetName);
					targets[pageNo]->setTargetActorsHiddenInGame(true);
					targets[pageNo]->OnTargetLost();
					UE_LOG(LogARToolKit, Log, TEXT("target lost ID: %i, Name: %s"), targets[pageNo]->ID, *targets[pageNo]->targetName);
					pageNo = -1;
				}
				else
				{
					for (int j = 0; j < 3; j++) for (int k = 0; k < 4; k++) targets[pageNo]->trans[j][k] = trackingTrans[j][k];
					targets[pageNo]->validPrev = targets[pageNo]->valid;
					//filter pose estimate
					if (targets[pageNo]->ftmi)
					{
						if (arFilterTransMat(targets[pageNo]->ftmi, targets[pageNo]->trans, !targets[pageNo]->validPrev) < 0)
						{
							UE_LOG(LogARToolKit, Error, TEXT("arFilterTransMat error with marker %i, %s"), targets[pageNo]->ID, *targets[pageNo]->targetName);
						}
					}
					//we have new position lets pass to camera transformation!
					ARdouble trans[3][4];
					ARdouble transInverted[3][4];
					ARdouble quaternion[4];
					ARdouble position[3];

					memcpy(trans, targets[pageNo]->trans, sizeof(ARdouble) * 3 * 4);

					arUtilMatInv(trans, transInverted);
					arUtilMat2QuatPos(transInverted, quaternion, position);

					//NFT position
					FVector pos = FVector(trans[0][3], trans[1][3], -trans[2][3]);

					//NFT rotations
					FRotator raw = FQuat(quaternion[0], quaternion[1], quaternion[2], quaternion[3]).Rotator();
					FRotator rot;
					rot.Yaw = raw.Yaw;
					rot.Pitch = -raw.Pitch;
					rot.Roll = 180 - raw.Roll;

					//Add NFT offset because NFT track in 0, 0 meaning left down corner of the image
					pos += rot.RotateVector(FVector(targets[pageNo]->offset.X, targets[pageNo]->offset.Y, 0));

					//add scale to the target camera
					float targetScale = targets[pageNo]->GetActorScale().X;
					pos *= targetScale;

					//target trans from unreal real object
					ATargetTrans = FTransform(targets[pageNo]->GetActorRotation(), targets[pageNo]->GetActorLocation(), targets[pageNo]->GetActorScale());

					//target trans from ARTOOLKIT
					targetTrans = FTransform(rot, pos, FVector(1, 1, 1));
				}
			}
			kpmRequired = (pageNo < 0);
		}

		
	}
}

void ARToolKit::setTracking(bool bAllowTrack)
{
	//enable or disable camera stream
	if (bAllowTrack)
	{
		isValid = true;
		playPreview();
		UE_LOG(LogARToolKit, Log, TEXT("ARToolKit continue tracking!"));
	}
	else
	{
		isValid = false;
#if defined _WIN_ || defined _MAC_ || defined _IOS_
		arVideoCapStop();
#elif defined _ANDROID_
		AndroidThunkCpp_stopCamera();
#endif

		UE_LOG(LogARToolKit, Log, TEXT("ARToolKit pause traclking!"));
	}
}


void ARToolKit::updateAR(float DeltaTime)
{
    //try to get new camera raw data, and if is valid, process texture and track
	if (texture && isValid)
	{
		void* image = NULL;
#if defined _WIN_ || defined _MAC_ || defined _IOS_
        image = (void*)arVideoGetImage();
#endif
        
#if defined _ANDROID_
        if (newFrame == true)
        {
			image = (void*)rawDataAndroid;
		}
#endif
        if (image != NULL)
        {
			updateTexture(image);
			detect((unsigned char*)image);
#if defined _ANDROID_
			newFrame = false;
#endif
        }
	}
}

FTransform ARToolKit::getAATargetTransformations()
{
	return ATargetTrans;
}

TArray<AARTarget*> ARToolKit::getTargets()
{
	return targets;
}

void ARToolKit::updateTexture(void* data)
{
	
	if (data == NULL) return;

	// fill my Texture Region data
	FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;
	RegionData->Texture2DResource = (FTexture2DResource*)texture->Resource;
	RegionData->MipIndex = 0;
	RegionData->NumRegions = 1;
	RegionData->Regions = echoUpdateTextureRegion;
	RegionData->SrcPitch = (uint32)(4 * width);
	RegionData->SrcBpp = 4;
    
    int32 stepSize = 3;
#if defined _MAC_ || defined _IOS_
    stepSize = 4;
#endif
    
	
#if defined _WIN_ || defined _MAC_ || defined _IOS_
	unsigned char* thisData = (unsigned char*)data;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int i = x + (y * width);
			rawData[i].B = thisData[stepSize * i + 0];
			rawData[i].G = thisData[stepSize * i + 1];
			rawData[i].R = thisData[stepSize * i + 2];
		}
		RegionData->SrcData = (uint8*)rawData.GetData();
	}

#elif defined _ANDROID_
	//convert YUV420 to bgra
	char* yuv420sp = (char*)data;
	int* rgb = new int[width * height];
	if (!rawDataAndroid) return;

	int size = width*height;
	int offset = size;

	int u, v, y1, y2, y3, y4;

	for (int i = 0, k = 0; i < size; i += 2, k += 2) {
		y1 = yuv420sp[i] & 0xff;
		y2 = yuv420sp[i + 1] & 0xff;
		y3 = yuv420sp[width + i] & 0xff;
		y4 = yuv420sp[width + i + 1] & 0xff;

		u = yuv420sp[offset + k] & 0xff;
		v = yuv420sp[offset + k + 1] & 0xff;
		u = u - 128;
		v = v - 128;

		rgb[i] = ConvertYUVtoRGB(y1, u, v);
		rgb[i + 1] = ConvertYUVtoRGB(y2, u, v);
		rgb[width + i] = ConvertYUVtoRGB(y3, u, v);
		rgb[width + i + 1] = ConvertYUVtoRGB(y4, u, v);

		if (i != 0 && (i + 2) % width == 0)
			i += width;
	}

	RegionData->SrcData = (uint8*)rgb;
	//__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "color SRCDATA %d", RegionData->SrcData[0]);

#endif
	

	//provitional fix
	FTexture2DMipMap& Mip = texture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, RegionData->SrcData, width * height * 4);
	Mip.BulkData.Unlock();
	texture->UpdateResource();

	//UE MUST FIX THIS, 4.13.1 breaks everything
	/*
	bool bFreeData = false;

	// call the RHIUpdateTexture2D to refresh the texture with new info
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdateTextureRegionsData,
		FUpdateTextureRegionsData*, RegionData, RegionData,
		bool, bFreeData, bFreeData,
		{
			for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
			{
				int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
				if (RegionData->MipIndex >= CurrentFirstMip)
				{
					RHIUpdateTexture2D(
						RegionData->Texture2DResource->GetTexture2DRHI(),
						RegionData->MipIndex - CurrentFirstMip,
						RegionData->Regions[RegionIndex],
						RegionData->SrcPitch,
						RegionData->SrcData
						+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
						+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
						);
				}
			}
	if (bFreeData)
	{
		FMemory::Free(RegionData->Regions);
		FMemory::Free(RegionData->SrcData);
	}
	delete RegionData;
		});

	*/
#if defined _ANDROID_
	delete[] rgb;
#endif
}






