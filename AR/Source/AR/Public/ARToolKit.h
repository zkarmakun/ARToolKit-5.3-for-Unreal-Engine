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

//---
//Forward declarations
//---
class AARTarget;
class AARPawn;

//inherit max page from ARToolKit
//DEPRECATED
#define PAGES_MAX 10   

class ARToolKit
{
public:

	ARToolKit();
	~ARToolKit();

	/** Initialize AR type of camera, current Pawn and how many target was found in current map, load NFT data and set enviorment for current level, each level must call this*/
	void initializeAR(ECameraSelection cameraSelection, AARPawn* newPawn, TArray<AARTarget*> targetsFound);

	/** enable and disable Tracking if you don't needed at runtime*/
	void setTracking(bool bAllowTrack);

	/** return available targets list found in the current map*/
	TArray<AARTarget*> getTargets();

	/** turn off AR, must call when you load a new map or when you finisih a game*/
	void shutdownAR();

	/** update AR, current ARPawn is in charge to call every frame*/
	void updateAR(float deltaTime);
	
	/** gets camera texture*/
	UTexture2D* getTexture();

	/** get width or hight from the camera texture*/
	int32 getWidth();
	int32 getheight();

	/** get target matrix results as unreal FTransform*/
	FTransform getTargetTransformations();

	/** get the found AATarget transformations in world space*/
	FTransform getAATargetTransformations();

private:
	//-- Initialize methods ---------------------------

	/** load camara intrinsic file and setup pixel format, and create runtime texture with same characteristics*/
	int setupCamera(FString camera_config, FString camera_parameter);

	/** set parameters for natural filucidal targets*/
	int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat);

	/** load natural filucidal target from all targets found in the current map and sets pixel format for tracking handle*/
	int loadNFT(TArray<AARTarget*> targetFound);

	/** start camera play back*/
	void playPreview();


	//-- Runtime methods -----------------------------

	/** update texture and call detection for each frame*/
	void updateTexture(void* data);

	/** try to detect some image target with the camera data*/
	void detect(unsigned char* data);


	//-- Turn off methods ----------------------------

	/** unload natural filucidal targets, is call form within shutdownAR()*/
	void unloadNFT();
	
	//----- ARTOOLKIT VARS ----------------------------------------------------

	//pixel format for each platform
	AR_PIXEL_FORMAT		pixelFormat;

	//threads handle for detecting
	THREAD_HANDLE_T     *threadHandle;
	AR2HandleT          *ar2Handle;
	KpmHandle           *kpmHandle;

	//number of surface, meaning number of targets found
	int                  surfaceSetCount;

	//pointers to NFT info data
	AR2SurfaceSetT      *surfaceSet[PAGES_MAX];

	//detect parameters
	ARParamLT		   *gCparamLT;

	//allow multu tracking
	bool nftMultiMode;

	//flag for KPM
	bool kpmRequired;
	bool kpmBusy;

	//detected page
	int32 pageNo = -1;


	//-- UE VARS --------------------------------------------

	//Path to content directory, diferent for each platform
	FString contentDirectory;

	//store pointer to Pawn in use
	AARPawn* currentPawn;

	//texture
	UTexture2D* texture = NULL;
	FUpdateTextureRegion2D *echoUpdateTextureRegion;

	//camera Texture data array
	TArray<FColor> rawData;

	//camera size
	int32 width;
	int32 height;

	//list of targets
	TArray<AARTarget*> targets;

	//whenever is singleton valid or not
	bool isValid = false;

	//store raw target transformations
	FTransform targetTrans;

	//store UE target transformations
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