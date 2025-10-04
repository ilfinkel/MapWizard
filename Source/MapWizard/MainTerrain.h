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
	int seed = -1;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double main_road_lights_dist = 25;
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


USTRUCT(BlueprintType)
struct FResidentialHousesParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseY = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseY = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseX = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseX = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseZ = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseZ = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SizeStep = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseSidesRatio = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseSidesRatio = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinSpaceBetweenHouses = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSpaceBetweenHouses = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArea = 90;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HouseChance = 90;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InnerSidewalkWidth = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OuterSidewalkWidth = 3;
};

USTRUCT(BlueprintType)
struct FLuxuryHousesParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseY = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseY = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseX = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseX = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseZ = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseZ = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SizeStep = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseSidesRatio = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseSidesRatio = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArea = 90;
};


USTRUCT(BlueprintType)
struct FSlumsHousesParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseY = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseY = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseX = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseX = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseZ = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseZ = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SizeStep = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinHouseSidesRatio = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHouseSidesRatio = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArea = 90;
};

USTRUCT(BlueprintType)
struct FPointDrawingObject
{
	GENERATED_BODY()
	// FPointDrawingObject(FVector point_, float angle_): point(point_), angle(angle_)
	// {
	// }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector point;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float angle;
};

struct DrawingObject
{
	// ~DrawingObject()
	// {
	// 	delete_mesh();
	// }
	void delete_mesh()
	{
		mesh->Destroy();
	}

	void define_mesh(bool is_mesh_exists)
	{
		name = mesh->GetActorLabel();
		if (is_mesh_exists) material_interface = mesh->ProceduralMesh->GetMaterial(0);
	}

	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
	                    float ExtrusionHeight);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight,
	                    float ExtrusionHeight);
	void create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight,
	                    float ExtrusionHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices, float StarterHeight);
	void create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices, float StarterHeight);
	AProceduralBlockMeshActor* mesh;
	FString name;
	UMaterialInterface* material_interface;
	UMaterialInterface* material;
};

struct DrawingDistrict : DrawingObject
{
	DrawingDistrict(TSharedPtr<District> district_,
	                AProceduralBlockMeshActor* mesh_,
	                double start_height_): district(district_)
	                                       , start_height(start_height_)
	{
		mesh = mesh_;
		define_mesh(true);
	}

	void delete_mesh()
	{
		// district->unselect();
		district.Reset();
		mesh->Destroy();
	}

	void draw_me()
	{
		mesh->SetActorLabel(name);
		mesh->ProceduralMesh->SetMaterial(0, material_interface);
		// mesh->Material = material;
		TArray<FVector> vertices;
		for (auto BaseVertex : district->self_figure)
		{
			FVector item = BaseVertex.point;
			vertices.Add(item);
		}
		create_mesh_2d(mesh, vertices, start_height);
		mesh->SetDynamicObject(district);
	}

	TSharedPtr<District> district;
	double start_height;
};

struct DrawingStreet : DrawingObject
{
	DrawingStreet(TSharedPtr<Street> street_,
	              AProceduralBlockMeshActor* mesh_,
	              double start_height_,
	              bool is_2d_): street(street_)
	                            , is_2d(is_2d_)
	                            , start_height(start_height_)
	{
		if (street->type == point_type::wall)
		{
			is_changing = true;
		}
		mesh = mesh_;
		define_mesh(true);
	}

	void draw_me()
	{
		mesh->SetActorLabel(name);
		mesh->ProceduralMesh->SetMaterial(0, material_interface);
		// mesh->Material = material;
		if (is_2d || !is_changing)
		{
			create_mesh_2d(mesh, street->street_vertexes, start_height);
		}
		else
		{
			create_mesh_3d(mesh, street->street_vertexes, start_height, 10);
		}
	}

	void redraw_me(double width)
	{
		mesh->SetActorLabel(name);
		mesh->ProceduralMesh->SetMaterial(0, material_interface);
		// mesh->Material = material;
		TArray<FVector> vertices;
		for (auto BaseVertex : street->street_vertices)
		{
			vertices.Add(BaseVertex->get_FVector());
		}
		street->street_vertexes = AllGeometry::line_to_polygon(vertices, width);
		if (is_2d || !is_changing)
		{
			create_mesh_2d(mesh, street->street_vertexes, start_height);
		}
		else
		{
			create_mesh_3d(mesh, street->street_vertexes, start_height, 10);
		}
	}

	TSharedPtr<Street> street;
	bool is_changing = false;
	bool is_2d;
	double start_height;
};

struct DrawingHouse : DrawingObject
{
	DrawingHouse(TSharedPtr<House> house_,
	             AProceduralBlockMeshActor* mesh_,
	             double start_height_,
	             bool is_2d_): house(house_)
	                           , is_2d(is_2d_)
	                           , start_height(start_height_)
	{
		mesh = mesh_;
		bool is_mesh_exists = true;
		if (house->get_object_type() == "House") is_mesh_exists = false;
		define_mesh(is_mesh_exists);
	}

	void draw_me()
	{
		mesh->SetActorLabel(name);
		// mesh->ProceduralMesh->SetMaterial(0, material_interface);
		// // mesh->Material = material;
		// if (is_2d)
		if (house->get_object_type() != "House")
		{
			create_mesh_2d(mesh, house->house_figure, start_height);
		}
		// else
		// {
		// 	create_mesh_3d(mesh, house->house_figure, start_height, house->height);
		// }
	}

	TSharedPtr<House> house;
	bool is_2d;
	double start_height;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FResidentialHousesParams ResidentialHousesParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLuxuryHousesParams LuxuryHousesParams;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlumsHousesParams SlumsHousesParams;
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices", meta = (AllowPrivateAccess = "true"))
	// TArray<FVector> VerticesRemembered;

	UFUNCTION(BlueprintCallable, Category = "Custom")
	int GetSeed();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void RedrawAll(bool is_2d);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllDistrictsSelected();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllStreetsSelected();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllHousesSelected();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	AProceduralBlockMeshActor* GetLastSelected();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	AProceduralBlockMeshActor* GetPrevSelected();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllStreets();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<FPointDrawingObject> GetAllPointObjects();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllHouses();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllOjectsOfTypeSelected(FString type_name);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllOjectsOfType(FString type_name);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetAllDistricts();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> UnselectAllStreets();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> UnselectAllHouses();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> UnselectAllDistricts();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	TArray<AProceduralBlockMeshActor*> GetLastTypeSelected();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void ReinitializeActor(FMapParams& map_params, FDebugParams& debug_params);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void ClearAll(FMapParams& map_params, FDebugParams& debug_params);
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void AttachDistricts();
	UFUNCTION(BlueprintCallable, Category = "Custom")
	void DivideDistricts();

	void clear_all();

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
	void draw_all();
	void get_cursor_hit_location();
	TArray<AProceduralBlockMeshActor*> get_all_houses_of_type_selected(FString type_name);
	TArray<AProceduralBlockMeshActor*> get_all_houses_of_type(FString type_name);
	TArray<TSharedPtr<Node>> map_borders_array{};
	TArray<TSharedPtr<District>> figures_array{};
	TArray<TSharedPtr<Street>> streets_array{};
	TArray<TSharedPtr<Street>> segments_array{};
	TArray<TSharedPtr<PointObject>> point_objects_array{};
	TArray<FVector> debug_points_array{};
	TArray<FVector> debug2_points_array{};
	TArray<TSharedPtr<Node>> roads{};
	TArray<TSharedPtr<District>> river_figures;
	TArray<DrawingDistrict> drawing_districts;
	TArray<DrawingStreet> drawing_streets;
	TArray<DrawingHouse> drawing_houses;
	// TArray<DrawingPointObject> drawing_point_objects;
	TSharedPtr<TArray<unsigned int>> selected_objects;
	TSharedPtr<TArray<unsigned int>> prev_selected_objects;

	int seed = -1;
};
