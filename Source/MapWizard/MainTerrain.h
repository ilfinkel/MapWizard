#pragma once
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include <MapWizard/AllGeometry.h>
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ProceduralObjectMeshActor.h"
#include "Algo/Reverse.h"
#include "Components/PrimitiveComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HAL/Runnable.h"

#include "MainTerrain.generated.h"

UENUM(BlueprintType)
enum class ECityPlan : uint8
{
	radial UMETA(DisplayName = "radial"),
	radial_circle UMETA(DisplayName = "radial-circle"),
	rectangular UMETA(DisplayName = "rectangular"),
	combined UMETA(DisplayName = "combined")
};

UENUM(BlueprintType)
enum class EWaterType : uint8
{
	none UMETA(DisplayName = "none"),
	river UMETA(DisplayName = "river")
};


UENUM(BlueprintType)
enum class EDrawStage : uint8
{
	river UMETA(DisplayName = "river(1)"),
	create_guiding_roads UMETA(DisplayName = "create_guiding_roads(2)"),
	create_usual_roads UMETA(DisplayName = "create_usual_roads(3)"),
	shrink_roads UMETA(DisplayName = "shrink_roads(4)"),
	process_districts UMETA(DisplayName = "process_blocks(5)"),
	process_houses UMETA(DisplayName = "process_houses(6)")
};

USTRUCT(BlueprintType)
struct FMapParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double av_river_length = 80;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double max_river_length = 150;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double min_new_road_length = 45;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double min_road_length = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double av_road_length = 70;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double max_road_length = 95;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double river_road_distance = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double main_road_width = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double road_width = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City Plan")
	ECityPlan city_plan = ECityPlan::radial_circle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drawing Stage")
	EDrawStage draw_stage = EDrawStage::process_houses;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water Type")
	EWaterType water_type = EWaterType::river;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double x_size = 4000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double y_size = 4000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double road_left_chance = 42;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double road_forward_chance = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double road_right_chance = 80;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool is_initialized = false;
	FVector center;
	double av_distance;
	UPROPERTY(BlueprintReadOnly, Category = "Folders")
	TArray<FString> FolderNames = GetSubfoldersInDirectory("/Game/Packs/Pack1");
	
	void update_me()
	{
		center = FVector(x_size / 2, y_size / 2, 0);
		av_distance = (x_size + y_size) / 4;
	}

private:
	TArray<FString> GetSubfoldersInDirectory(const FString& DirectoryPath)
	{
		TArray<FString> Subfolders;
		FString FullPath = FPaths::ConvertRelativePathToFull(DirectoryPath);

		IFileManager& FileManager = IFileManager::Get();
		FileManager.FindFiles(Subfolders, *(FullPath / TEXT("*")), false, true); // false = no files, true = directories

		return Subfolders;
	}
	void UpdateFolderList(const FString& DirectoryPath)
	{
		FolderNames = GetSubfoldersInDirectory(DirectoryPath);

		// Выводим список в лог для проверки
		for (const FString& Folder : FolderNames)
		{
			UE_LOG(LogTemp, Log, TEXT("Found folder: %s"), *Folder)
		}
	}
};

USTRUCT(BlueprintType)
struct FDebugParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool draw_usual_roads = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool draw_walls = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool draw_main = true;
};

UCLASS()
class MAPWIZARD_API AMainTerrain : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMainTerrain();
	UPROPERTY()
	bool bIsEverythingLoaded = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMapParams MapParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDebugParams DebugParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> VerticesRemembered;

	UFUNCTION(BlueprintCallable, Category = "Custom")
	void RedrawAll(bool is_2d);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void ReinitializeActor(FMapParams& map_params, FDebugParams& debug_params);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool is_2d = true;

	UProceduralMeshComponent* BaseComponent;

	


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void initialize_all();
private:
	UMaterialInterface* BaseMaterial;
	UMaterialInterface* WaterMaterial;
	UMaterialInterface* DocsMaterial;
	UMaterialInterface* RoyalMaterial;
	UMaterialInterface* ResidentialMaterial;
	UMaterialInterface* LuxuryMaterial;
	UMaterialInterface* SlumsMaterial;
	UMaterialInterface* BuildingMaterial;
	UMaterialInterface* RoadMaterial;
	UMaterialInterface* MainRoadMaterial;
	UMaterialInterface* WallMaterial;
	UMaterialInterface* load_material(const FString& TexturePack, const FString& MaterialName);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
	                    float ExtrusionHeight);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight,
	                    float ExtrusionHeight);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight,
	                    float ExtrusionHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight);
	void draw_all();
	void get_cursor_hit_location();
	TArray<TSharedPtr<Node>> map_borders_array{};
	TArray<District> figures_array{};
	TArray<Street> streets_array{};
	TArray<FVector> debug_points_array{};
	TArray<TSharedPtr<Node>> roads{};
	District river_figure;
};

