/*
*  ARTarget.h
*  ARToolKit5
*
*  This file generate a special actor who defines a NFT target
*  File descriptions: ARTarget contains very important features like size of the target,
*  the offset calcultation for the NFT track point, a plane to display the target just for editor mode
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
#include "ARToolKit.h"
#include "ARTarget.generated.h"

UCLASS()
class AARTarget : public AActor
{
	GENERATED_BODY()

private:

#if WITH_EDITOR
	/** Texture to display target in editor*/
	UTexture2D* targetTexture;

	/** Store default name*/
	FString prevAcceptTargetName;

	/** if no target load display sample from Plugin content*/
	UTexture2D* defaultTexture;
#endif

public:

	AARTarget();


	//-- NFT parameter---------------------
	//unique ID
	int32 ID = -1;

	//filter for detect frecuency
	ARdouble filterCutoffFrequency;

	//filter rate
	ARdouble filterSampleRate;

	//previous frame valid
	bool validPrev;

	//store if is valid, meaning was detected
	bool valid;

	//store matrix pose
	ARdouble trans[3][4];

	//filter for matrix
	ARFilterTransMatInfo* ftmi;


	//-- COMPONENTS ------------------------------
	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		USceneComponent* targetPivot;

	/** Whenever a target was loaded, this plane show that texture*/
	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		UStaticMeshComponent* plane;

	/** if set allow shadow plane, this plane represent a basic plane with texture
		*You can use Shadow Plane for mobile content, but is heavy, recommed use Mobile Dynamic Shadows instead cascade shadows
		*Because UE is deferred rendering you will have gab in the corners of the visualzation shadow plane, because mix between diffuse and self ilumination
		*to avoid gap, HLSL shader is needed
	*/
	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		UStaticMeshComponent* shadowPlane;

	//-- BASIC OVERRIDES
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "set scale if your meshes are bigger or smaller"))
		float scale = 1;

	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "Type new target name, must be inside AR folder in the content"))
		FString targetName = "pinball";

	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "include actor here if you need track some movable, for example a skeletal mesh"))
		TArray<AActor*> movableObjectList;

	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "if you want see shadows interacting with the camera the active this, take care with performance"))
		bool AllowShadowPlane = false;

	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "set new scale for the shadow, try to covert all the space for your meshes"))
		float shadowPlaneScale = 400;

	/** This event fires when Target is found by detection
		*You can use this event to trigger some animation or sound
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "ARToolKit")
		void OnTargetFound();

	/** This event fires when Target is Lost by detection
		*You can use this event to stop some animation or sound
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "ARToolKit")
		void OnTargetLost();

	/** Store pixel size for this target
		*set by ARToolKit.cpp
	*/
	FVector2D sizeMM = FVector2D(400, 400);

	/** Store offsets, this is calculate from ARToolKit cpp
		*set by ARToolKit.cpp
	*/
	FVector offset;
	
	/** return this actor rotation*/
	FRotator getTargetRotation();

	/** toogle Actors visibility binded to this Target
		*Call by ARToolKit.cpp when found or lost target
	*/
	void setTargetActorsHiddenInGame(bool bHidden);

};