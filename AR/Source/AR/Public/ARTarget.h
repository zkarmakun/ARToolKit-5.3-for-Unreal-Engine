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
	UTexture2D* targetTexture;
	FString prevAcceptTargetName;
	UTexture2D* defaultTexture;
#endif

public:

	AARTarget();

	int32 ID = -1;
	ARdouble filterCutoffFrequency;
	ARdouble filterSampleRate;
	bool validPrev;
	bool valid;
	ARdouble trans[3][4];
	ARFilterTransMatInfo* ftmi;


	//vars
	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		USceneComponent* targetPivot;

	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		UStaticMeshComponent* plane;

	UPROPERTY(Category = "ARToolKit", VisibleDefaultsOnly)
		UStaticMeshComponent* shadowPlane;

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

	UFUNCTION(BlueprintImplementableEvent, Category = "ARToolKit")
		void OnTargetFound();

	UFUNCTION(BlueprintImplementableEvent, Category = "ARToolKit")
		void OnTargetLost();

	FVector2D sizeMM = FVector2D(400, 400);
	FVector offset;
	
	FRotator getTargetRotation();
	void setTargetActorsHiddenInGame(bool bHidden);

};