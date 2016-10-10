/*
*  ARToolKit.h
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

#pragma once
#include "Engine.h"
#include "AR/ar.h"
#include "AR/arMulti.h"
#include "AR/video.h"
#include "AR/arFilterTransMat.h"
#include "AR2/tracking.h"
#include "KPM/kpm.h"
#include "trackingSub.h"
#include "Utils.h"

//Forward declarations
class AARTarget;
class AARPawn;

#define PAGES_MAX 10   

class ARToolKit
{
public:

	ARToolKit();
	~ARToolKit();

	void initializeAR(ECameraSelection cameraSelection, AARPawn* newPawn, TArray<AARTarget*> targetsFound);
	void setTracking(bool bAllowTrack);
	TArray<AARTarget*> getTargets();
	void shutdownAR();
	void updateAR(float deltaTime);
	

	/*gets camera texture*/
	UTexture2D* getTexture();

	/*get width or hight from the camera texture*/
	int32 getWidth();
	int32 getheight();

	/*get target matrix results as unreal FTransform*/
	FTransform getTargetTransformations();

	/*get the found AATarget transformations in world space*/
	FTransform getAATargetTransformations();

	

private:
	int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat);
	int setupCamera(FString camera_config, FString camera_parameter);
	void unloadNFT();
	int loadNFT(TArray<AARTarget*> targetFound);
	void updateTexture(void* data);
	void detect(unsigned char* data);
	void playPreview();

	FString contentDirectory;
	AR_PIXEL_FORMAT		pixelFormat;
	THREAD_HANDLE_T     *threadHandle;
	AR2HandleT          *ar2Handle;
	KpmHandle           *kpmHandle;
	int                  surfaceSetCount;
	AR2SurfaceSetT      *surfaceSet[PAGES_MAX];
	ARParamLT		   *gCparamLT;
	bool nftMultiMode;
	bool kpmRequired;
	bool kpmBusy;
	int32 pageNo = -1;

	AARPawn* currentPawn;
	UTexture2D* texture = NULL;
	FUpdateTextureRegion2D *echoUpdateTextureRegion;
	TArray<FColor> rawData;
	int32 width;
	int32 height;
	TArray<AARTarget*> targets;
	bool isValid = false;
	FTransform targetTrans;
	FTransform ATargetTrans;
};


// Region Data struct
struct FUpdateTextureRegionsData
{
	FTexture2DResource* Texture2DResource;
	int32 MipIndex;
	uint32 NumRegions;
	FUpdateTextureRegion2D* Regions;
	uint32 SrcPitch;
	uint32 SrcBpp;
	uint8* SrcData;
};