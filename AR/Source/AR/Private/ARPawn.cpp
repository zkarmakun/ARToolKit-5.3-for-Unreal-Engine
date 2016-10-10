/*
*  ARPawn.cpp
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


#include "ARPrivatePCH.h"
#include "ARPawn.h"
#include "ARToolKit.h"

#define pprint(txt) GEngine->AddOnScreenDebugMessage(-1,5,FColor::Green, txt)

AARPawn::AARPawn()
{
	pivot = CreateDefaultSubobject<USceneComponent>("Pivot");
	RootComponent = pivot;

	camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	camera->SetupAttachment(pivot);
	plane = CreateDefaultSubobject<UStaticMeshComponent>("TexturePlane");
	plane->SetupAttachment(camera);

	plane->SetRelativeLocation(FVector(656, 0, 0));
	plane->SetRelativeScale3D(FVector(1, 640, 480));
	camera->SetRelativeRotation(FQuat(FRotator(-90, 0, -90)));

	UStaticMesh* myMeshPlane = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/AR/Plane")));
	if (myMeshPlane != NULL)
	{
		plane->SetStaticMesh(myMeshPlane);
	}

	targetHelper = CreateDefaultSubobject<USceneComponent>("Target Helper");
	targetHelper->SetupAttachment(pivot);
	cameraHelper = CreateDefaultSubobject<USceneComponent>("Camera Helper");
	cameraHelper->SetupAttachment(targetHelper);

}


void AARPawn::BeginPlay()
{
	
	//find all AARTarget* in the scene!
	TArray<AARTarget*> targets;
	for (TActorIterator<AARTarget> It(GetWorld(), AARTarget::StaticClass()); It; ++It)
	{
		AARTarget* target = *It;
		if (!target->IsPendingKill())
		{
			targets.Add(target);
			if (bAllowTracking)
			{
				target->setTargetActorsHiddenInGame(true);
			}
		}
	}
	
	//initialize the ar interface
	IARModule::Get().GetARToolKit()->initializeAR(cameraSelection, this, targets);
	SetTracking(bAllowTracking);

	//create material instance dynamic for the plane
	materialInstance = plane->CreateDynamicMaterialInstance(0);

	materialInstance->SetTextureParameterValue("tex", getCameraTexture());
	if (getCameraTexture() != NULL)
	{
		pixelAspectRatioTexture = (float)(getCameraTexture()->GetSizeX() / (getCameraTexture()->GetSizeY() * 1.00f));
		camera->AspectRatio = pixelAspectRatioTexture;
		camera->FieldOfView = hFOV;
		setBackground();
	}

	//in order to call in some whatever actor begin play the texture, we must be sure call initialization before UE begin play
	Super::BeginPlay();
}

void AARPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	//call shutdown interface for AR
	IARModule::Get().GetARToolKit()->shutdownAR();
}

void AARPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bAllowTracking)
	{
		IARModule::Get().GetARToolKit()->updateAR(DeltaTime);
		setCameraTransformations();
	}
}

UTexture2D* AARPawn::getCameraTexture()
{
	return IARModule::Get().GetARToolKit()->getTexture();
}

FTransform AARPawn::getCameraTrans()
{
	//set target transforms to target fake
	targetHelper->SetWorldTransform(IARModule::Get().GetARToolKit()->getTargetTransformations());

	//set zero camera
	cameraHelper->SetWorldLocationAndRotation(FVector(0, 0, 0), FRotator(-90, 0, -90));

	//now gets inverse to know the camera locations and not the target locations
	FVector relativeLoc = cameraHelper->GetRelativeTransform().GetLocation();
	FQuat relativeRot = cameraHelper->GetRelativeTransform().GetRotation();

	FTransform result = FTransform(relativeRot, relativeLoc, FVector(1, 1, 1));
	return result;
}


void AARPawn::execOnTargetFound(int32 id, FString targetName)
{
	setBackground();
	OnTargetFound(id, FName(*targetName));
}

void AARPawn::execOnTargetLost(int32 id, FString targetName)
{
	OnTargetLost(id, FName(*targetName));
}

void AARPawn::setCameraTransformations()
{
	pivot->SetWorldRotation(FRotator(0, 0, 0));
	SetActorLocation(IARModule::Get().GetARToolKit()->getAATargetTransformations().GetLocation());
	camera->SetRelativeTransform(getCameraTrans());

	//if target was Rotate the pass that rotation to the camera
	float targetYAW = IARModule::Get().GetARToolKit()->getAATargetTransformations().GetRotation().Rotator().Yaw;
	pivot->SetWorldRotation(FRotator(0, 90 + targetYAW, 0));
}

void AARPawn::setBackground()
{
	//calculate a new distance of the camera plane based on the target scale
	float scale = IARModule::Get().GetARToolKit()->getAATargetTransformations().GetScale3D().X;
	float fixScale =  3 * scale;
	FVector2D resolution = FVector2D(getCameraTexture()->GetSizeX() * fixScale, getCameraTexture()->GetSizeY() * fixScale);
	//with a fov you calculate a distance based in tangents
	float distanceAwayFromCamera = (resolution.X / 2.f) / FMath::Tan(PI / 180.f * (hFOV / 2.f));
	plane->SetRelativeScale3D(FVector(1, resolution.X, resolution.Y));
	plane->SetRelativeLocation(FVector(distanceAwayFromCamera, 0, 0));
}

void AARPawn::SetTracking(bool on)
{
	if (plane != nullptr)
	{
		plane->SetVisibility(on);
	}
	bAllowTracking = on;
	TArray<AARTarget*> targets = IARModule::Get().GetARToolKit()->getTargets();
	if (bAllowTracking)
	{
		for (auto target : targets)
		{
			target->setTargetActorsHiddenInGame(true);
		}
		IARModule::Get().GetARToolKit()->setTracking(bAllowTracking);
	}
	else
	{
		for (auto target : targets)
		{
			target->setTargetActorsHiddenInGame(false);
		}
		IARModule::Get().GetARToolKit()->setTracking(bAllowTracking);
	}
}