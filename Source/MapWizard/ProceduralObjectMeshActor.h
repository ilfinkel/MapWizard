#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "AllGeometry.h"

#include "ProceduralObjectMeshActor.generated.h"

USTRUCT(BlueprintType)
struct FVectorArrayWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> Points;
};

UCLASS(BlueprintType, Blueprintable, Placeable)
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
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	// UMaterialInterface* Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	int roof_texture_turns = 0;
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void clickMesh();
	UFUNCTION()
	void OnMouseOver(UPrimitiveComponent* Component);
	UFUNCTION()
	void OnMouseOut(UPrimitiveComponent* Component);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	bool IsHovered()
	{
		return object->is_hovered();
	}

	UFUNCTION(BlueprintCallable, Category = "Custom")
	bool IsSelected()
	{
		return object->is_selected();
	}

	void SetMeshComponentName(const FString& NewName)
	{
		MeshComponentName = NewName;
	}

	UFUNCTION(BlueprintCallable, Category = "Custom")
	float GetAngle()
	{
		return object->get_angle();
	}

	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<FVector> GetObjectVertexes()
	{
		return object->get_object_vertexes();
	}
	
	UFUNCTION(BlueprintCallable, Category = "Custom")
	FVector GetCenter()
	{
		return object->get_center();
	}
	
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<FVectorArrayWrapper> SliceHouse(float north, float east, float south, float west)
	{
		TArray<FVectorArrayWrapper> array;
		for (auto& house_part : object->slice_house(north, east, south, west))
		{
			FVectorArrayWrapper wrap;
			wrap.Points = house_part;
			array.Add(wrap);
		}
		return array;
	}

	UFUNCTION(BlueprintCallable, Category = "Custom")
	int GetID()
	{
		return object->get_id();
	}

	UFUNCTION(BlueprintCallable, Category = "Custom")
	FVector GetMeasure()
	{
		return object->get_measure();
	}

	UFUNCTION(BlueprintCallable, Category = "Custom")
	FString GetObjectType()
	{
		return object->get_type1_name();
	}

	void SetDynamicObject(TSharedPtr<SelectableObject> distr)
	{
		object = distr;
	}

	void SetSelectedObject(
		TSharedPtr<TArray<unsigned int>> selected_object_, TSharedPtr<TArray<unsigned int>> prev_selected_object_)
	{
		selected_object = selected_object_;
		prev_selected_object = prev_selected_object_;
	}


	FString MeshComponentName;
	TSharedPtr<SelectableObject> object;
	TSharedPtr<TArray<unsigned int>> selected_object;
	TSharedPtr<TArray<unsigned int>> prev_selected_object;
};
