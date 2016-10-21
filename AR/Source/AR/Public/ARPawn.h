/*
*  ARPawn.h
*  ARToolKit5
*
*  This file generate a special kind of pawn, who contain a basic camera and a plane with the camera texture
*  File description: ARPawn have a camera with some features like the correct FOV, this automanage the background plane,
*  based of the NFT target scale, also this have some funcionallity to transform the found NFT matrix to the camera coordinates
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
#include "ARTarget.h"
#include "Utils.h"
#include "Engine.h"
#include "ARPawn.generated.h"

UCLASS()
class AARPawn : public APawn
{
	GENERATED_BODY()

private:
	/** aspect ratio for camera device resolution*/
	float pixelAspectRatioTexture = 0;
	UMaterialInstanceDynamic* materialInstance;
	//const float fov = 70.580002f;
	//const horizontal FOV provide for camera intrinsics in files
	const float hFOV = 52.00f;

	float vFOV;
	
	/** get Camera Transformations*/
	FTransform getCameraTrans();
	void setCameraTransformations();
	void setBackground();

public:

	AARPawn();

	//-- COMPONENTS ----------------------------------
	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		USceneComponent* pivot;

	UPROPERTY(Category = "ARToolKit", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* camera;

	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		UStaticMeshComponent* plane;


	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		USceneComponent* targetHelper;

	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		USceneComponent* cameraHelper;

	//--- VARS ---------------------------------------------

	/** Access camera
		*Blueprint programmer can choose camera index, or camera position in mobile case
	*/
	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "DEFAULT in mobile mean the back camera, in desktop is the first, and in desktop works as ++ index if you need another webcam"))
		ECameraSelection cameraSelection;

	/** Initial tracking state*/
	UPROPERTY(EditAnyWhere, Category = "ARToolKit", meta = (ToolTip = "by default is true, so you start your pawn instance tracking"))
		bool bAllowTracking = true;

	//-- BASIC OVERRIDES ----------------------------------------------

	virtual void BeginPlay();
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	//-- METHODS ---------------------------------------------------

	// get texture pointer for pawn
	UFUNCTION(BlueprintPure, Category = "ARToolKit")
		UTexture2D* getCameraTexture();

	// Implementable event on target found, this triggers when found some target
	UFUNCTION(BlueprintImplementableEvent, Category = "ARToolKit")
		void OnTargetFound(int32 targetID,const FName& targetName);

	// Implementable event on target lost, this triggers when lost some target
	UFUNCTION(BlueprintImplementableEvent, Category = "ARToolKit")
		void OnTargetLost(int32 targetID, const FName& targetName);

	//c++ style method to call within ARToolKit.cpp
	void execOnTargetFound(int32 id, FString targetName);
	void execOnTargetLost(int32 id, FString targetName);

	// allow Enable or Disable Tracking to save resources if you do not needed anymore
	UFUNCTION(BlueprintCallable, Category = "ARToolKit")
		void SetTracking(bool on);


};