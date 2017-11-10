/*
*  ARTarget.cpp
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


#include "ARPrivatePCH.h"
#include "ARTarget.h"
#include "FileManagerGeneric.h"

// Fix WinBase.h override
#undef UpdateResource
#define UpdateResource UpdateResource

DEFINE_LOG_CATEGORY_STATIC(LogARToolKits, Log, All);

AARTarget::AARTarget()
{
	//create components
	targetPivot = CreateDefaultSubobject<USceneComponent>("Target Pivot");
	targetPivot->SetMobility(EComponentMobility::Static);
	RootComponent = targetPivot;

#if WITH_EDITOR
	//Editor only: create static mesh component and display plane 
	plane = CreateDefaultSubobject<UStaticMeshComponent>("Target Plane");
	plane->SetupAttachment(targetPivot);
	UMaterial* targetMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), NULL, TEXT("/AR/targetMat")));
	if (targetMat != NULL)
	{
		plane->SetMaterial(0, targetMat);
	}

	// set dummy texture by default
	UStaticMesh* myMeshPlane = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/AR/Plane")));
	if (myMeshPlane != NULL)
	{
		plane->SetStaticMesh(myMeshPlane);
		plane->SetRelativeScale3D(FVector(1, sizeMM.X, sizeMM.Y));
		plane->SetRelativeRotation(FQuat(FRotator(-90, 0, 0)));
		plane->SetMobility(EComponentMobility::Static);
	}

	defaultTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), NULL, TEXT("/AR/icon128")));
#endif

	//create shadow plane component
	shadowPlane = CreateDefaultSubobject<UStaticMeshComponent>("shadow plane");
	shadowPlane->SetupAttachment(RootComponent);
	UStaticMesh* myMeshPlane2 = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/AR/Plane")));
	if (myMeshPlane2 != NULL)
	{
		shadowPlane->SetStaticMesh(myMeshPlane2);
		shadowPlane->SetRelativeScale3D(FVector(1, shadowPlaneScale, shadowPlaneScale));
		shadowPlane->SetRelativeRotation(FQuat(FRotator(-90, 0, 0)));
		shadowPlane->SetRelativeLocation(FVector(0, 0, 1));
		UMaterial* shadowMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), NULL, TEXT("/AR/shadowPlane_mat")));
		if (shadowMat != NULL)
		{
			shadowPlane->SetMaterial(0, shadowMat);
		}
	}
	
}

void AARTarget::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	//set scale based on custom value
	SetActorScale3D(FVector(scale, scale, scale));

#if WITH_EDITOR

	if (targetName != "" && prevAcceptTargetName != targetName)
	{
		//load target from content
		FString thisTarget = FPaths::GameContentDir() + "AR/" + targetName;
		if (FPaths::FileExists(thisTarget + ".iset"))
		{
			AR2ImageSetT* imageSet = ar2ReadImageSet(TCHAR_TO_ANSI(*thisTarget));
			if (imageSet != NULL)
			{
				//int32 halfIndex = imageSet->num / 2;
				int32 halfIndex = 0;
				int32 w = imageSet->scale[halfIndex]->xsize;
				int32 h = imageSet->scale[halfIndex]->ysize;
				int32 dpi = imageSet->scale[0]->dpi;
			
				//UE_LOG(LogARToolKits, Log, TEXT("target size found: %i , %i , %i"), imageSet->scale[i]->xsize, imageSet->scale[i]->ysize, imageSet->scale[i]->dpi);
				unsigned char* RawData = imageSet->scale[halfIndex]->imgBW;

				targetTexture = UTexture2D::CreateTransient(w, h, PF_B8G8R8A8);
				TArray<FColor> rawData;
				rawData.Init(FColor(0, 0, 0, 255), w* h);
				for (int y = 0; y <  h; y++)
				{
					for (int x = 0; x <  w; x++)
					{
						int i = x + (y *  w);
						rawData[i].B = RawData[i];
						rawData[i].G = RawData[i];
						rawData[i].R = RawData[i];
					}
				}

				FTexture2DMipMap& Mip = targetTexture->PlatformData->Mips[0];
				void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(Data,rawData.GetData(), w * h * 4);
				Mip.BulkData.Unlock();
				targetTexture->UpdateResource();

				UMaterialInstanceDynamic* materialInstance = plane->CreateDynamicMaterialInstance(0);
				if (materialInstance != NULL)
				{
					materialInstance->SetTextureParameterValue("tex", targetTexture);
					sizeMM.X = w;
					sizeMM.Y = h;
					UE_LOG(LogARToolKits, Log, TEXT("target found name: %s, pixel size: (w: %i , h: %i), UE real size: (w: %f , h: %f)"), *targetName, w,h,pixels2millimeter(w), pixels2millimeter(h));
					prevAcceptTargetName = targetName;
				}
			}
		}
		else
		{
			UMaterialInstanceDynamic* materialInstance = plane->CreateDynamicMaterialInstance(0);
			if (materialInstance != NULL)
			{
				materialInstance->SetTextureParameterValue("tex", defaultTexture);
				sizeMM.X = defaultTexture->GetSizeX() * 40;
				sizeMM.Y = defaultTexture->GetSizeY() * 40;

				UE_LOG(LogARToolKits, Warning, TEXT("this name is not found in AR content folder, check if really exists!"));
				prevAcceptTargetName = "";
			}
		}
	}

	//set plane size based on image target
	plane->SetRelativeScale3D(FVector(1, pixels2millimeter(sizeMM.X), pixels2millimeter(sizeMM.Y)));

#endif

	//toogle shadow plane 
	shadowPlane->SetVisibility(AllowShadowPlane);

	//set shadow plane sacle
	shadowPlane->SetRelativeScale3D(FVector(1, shadowPlaneScale, shadowPlaneScale));
}

void AARTarget::BeginPlay()
{
	Super::BeginPlay();

	if (!AllowShadowPlane)
	{
		shadowPlane->DestroyComponent();
	}
	else
	{
		//if plane exist set camera texture 
		UMaterialInstanceDynamic* shadowDynamicMaterial = shadowPlane->CreateDynamicMaterialInstance(0);
		if (shadowDynamicMaterial != NULL)
		{
			shadowDynamicMaterial->SetTextureParameterValue("tex", IARModule::Get().GetARToolKit()->getTexture());
		}
	}
	
}

void AARTarget::setTargetActorsHiddenInGame(bool bHidden)
{
	TArray<USceneComponent*> childs;
	targetPivot->GetChildrenComponents(true, childs);
	for (auto& child : childs)
	{
		if (child != NULL && child != plane)
		{
			child->SetHiddenInGame(bHidden);
		}
	}

	for (auto& movable : movableObjectList)
	{
		if (movable != NULL) movable->SetActorHiddenInGame(bHidden);
	}
	plane->SetHiddenInGame(true, false);
}

FRotator AARTarget::getTargetRotation()
{
	return GetActorRotation();
}

