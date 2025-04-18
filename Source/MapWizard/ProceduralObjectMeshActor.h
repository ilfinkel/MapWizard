﻿#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "AllGeometry.h"

#include "ProceduralObjectMeshActor.generated.h"

UCLASS(Blueprintable)
class MAPWIZARD_API AProceduralBlockMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AProceduralBlockMeshActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<FVector> Vertices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<int32> Triangles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* DefaultMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* Material;
	UFUNCTION()
	void OnMeshClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);
	UFUNCTION()
	void OnMouseOver(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOut(UPrimitiveComponent* Component);
	FString MeshComponentName;
	void SetMeshComponentName(const FString& NewName)
	{
		MeshComponentName = NewName;
	}
	void SetDistrict(TSharedPtr<District> distr)
	{
		district = distr;
	}
	TSharedPtr<District> district;
};
