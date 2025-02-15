#include "MainTerrain.h"
// #include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "OrthographicCameraPawn.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "ProceduralObjectMeshActor.h"
#include "TerrainGen.h"

void DrawingObject::create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
                                   float ExtrusionHeight)
{
	int32 NumVertices = BaseVertices.Num();
	if (NumVertices < 3)
	{
		return; // Нужно хотя бы 3 вершины для создания полигона
	}

	TArray<FVector> Vertices;
	TArray<FVector> Base;
	TArray<int32> Triangles;

	// Добавляем вершины нижней стороны
	for (auto Vertex : BaseVertices)
	{
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().InverseTransformPosition(Vertex);
		Base.Add(local_vertex + FVector(0, 0, StarterHeight));
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight));
	}

	// Добавляем вершины верхней стороны, сдвинутые вверх на ExtrudeHeight
	for (auto Vertex : BaseVertices)
	{
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().InverseTransformPosition(Vertex);
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight) + FVector(0, 0, ExtrusionHeight));
	}

	// Используем триангуляцию для нижней стороны
	TArray<int32> BaseTriangles;
	AllGeometry::TriangulatePolygon(Base, BaseTriangles);

	// Добавляем триангуляцию для нижней стороны
	for (int32 i = 0; i < BaseTriangles.Num(); i += 3)
	{
		Triangles.Add(BaseTriangles[i]);
		Triangles.Add(BaseTriangles[i + 1]);
		Triangles.Add(BaseTriangles[i + 2]);
	}

	// Добавляем триангуляцию для верхней стороны (с учетом смещения вершин)
	int32 Offset = NumVertices;
	for (int32 i = 0; i < BaseTriangles.Num(); i += 3)
	{
		Triangles.Add(BaseTriangles[i] + Offset);
		Triangles.Add(BaseTriangles[i + 2] + Offset);
		Triangles.Add(BaseTriangles[i + 1] + Offset);
	}

	// Создаем боковые стороны
	for (int32 i = 0; i < NumVertices; i++)
	{
		int32 NextIndex = (i + 1) % NumVertices;

		// Боковая сторона, первый треугольник
		Triangles.Add(NextIndex);
		Triangles.Add(i);
		Triangles.Add(NumVertices + i);

		// Боковая сторона, второй треугольник
		Triangles.Add(NumVertices + NextIndex);
		Triangles.Add(NextIndex);
		Triangles.Add(NumVertices + i);
	}

	// Создаем пустые массивы для нормалей, UV-координат и тангенсов
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// Создаем меш
	Mesh->ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents,
		true);
}

void DrawingObject::create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
                                   float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_FVector());
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}
void DrawingObject::create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
                                   float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->point);
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}

void DrawingObject::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight)
{
	int32 NumVertices = BaseVertices.Num();

	if (NumVertices < 3)
	{
		return; // Нужно хотя бы 3 вершины для создания полигона
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;

	// Добавляем вершины нижней стороны
	for (auto Vertex : BaseVertices)
	{
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().InverseTransformPosition(Vertex);
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight));
		// Vertices.Add(Vertex + FVector(0, 0, StarterHeight));
	}

	// Используем триангуляцию для нижней стороны
	TArray<int32> BaseTriangles;
	AllGeometry::TriangulatePolygon(Vertices, BaseTriangles);

	for (int32 i = 0; i < BaseTriangles.Num(); i += 3)
	{
		Triangles.Add(BaseTriangles[i]);
		Triangles.Add(BaseTriangles[i + 2]);
		Triangles.Add(BaseTriangles[i + 1]);
	}

	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	if (Vertices.Num() == 4)
	{
		UVs = {
			FVector2D(1, 0),
			FVector2D(1, 1),
			FVector2D(0, 1),
			FVector2D(0, 0)
		};

		Normals = {
			FVector(0, 0, 1),
			FVector(0, 0, 1),
			FVector(0, 0, 1),
			FVector(0, 0, 1)
		};
	}
	Mesh->Vertices = Vertices;
	Mesh->Triangles = Triangles;
	// Создаем пустые массивы для нормалей, UV-координат и тангенсов
	// TArray<FVector> Normals;
	// TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	// VerticesRemembered = Vertices;

	// Создаем меш
	Mesh->ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}
void DrawingObject::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
                                   float StarterHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_FVector());
	}
	create_mesh_2d(Mesh, vertices, StarterHeight);
}
void DrawingObject::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
                                   float StarterHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		FVector aa = BaseVertex->point;
		vertices.Add(aa);
	}
	create_mesh_2d(Mesh, vertices, StarterHeight);
}
AMainTerrain::AMainTerrain() : BaseMaterial(nullptr)
                             , WaterMaterial(nullptr)
                             , DocsMaterial(nullptr)
                             , RoyalMaterial(nullptr)
                             , ResidentialMaterial(nullptr)
                             , LuxuryMaterial(nullptr)
                             , SlumsMaterial(nullptr)
                             , BuildingMaterial(nullptr)
                             , RoadMaterial(nullptr)
                             , MainRoadMaterial(nullptr)
                             , WallMaterial(nullptr)
                             , MapParams()
                             , BaseComponent(nullptr)
{
	// PrimaryActorTick.bCanEverTick = false;
}

void AMainTerrain::RedrawAll(bool is_2d_)
{
	is_2d = is_2d_;
	draw_all();
}
void AMainTerrain::ReinitializeActor(FMapParams& map_params, FDebugParams& debug_params)
{
	roads.Empty();
	figures_array.Empty();
	streets_array.Empty();
	map_borders_array.Empty();
	debug_points_array.Empty();
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	// Список акторов, которые нужно оставить
	TArray<AActor*> ActorsToKeep;
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController)
	{
		ActorsToKeep.Add(PlayerController);
		if (AActor* PlayerPawn = PlayerController->GetPawn())
		{
			ActorsToKeep.Add(PlayerPawn);
		}
	}

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;

		// Пропускаем обязательные и системные акторы
		if (!ActorsToKeep.Contains(Actor) && Actor)
		{
			Actor->Destroy();
		}
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOrthographicCameraPawn::StaticClass(), FoundActors);
	MapParams.update_me();

	if (FoundActors.Num() > 0)
	{
		AActor* OrthographicCamera = FoundActors[0];

		FVector NewLocation = FVector(MapParams.x_size / 2, MapParams.y_size / 2, (MapParams.x_size + MapParams.y_size) / 2);
		OrthographicCamera->SetActorLocation(NewLocation);
		FRotator DownwardRotation = FRotator(0.00, -90.00, 0.00);
		OrthographicCamera->SetActorRotation(DownwardRotation);

		TerrainGen gen(MapParams);
		gen.create_terrain(roads, figures_array, streets_array, river_figures, map_borders_array, debug_points_array);
		gen.empty_all();
		draw_all();
		AActor* ViewTarget = PlayerController->GetViewTarget();
		if (ViewTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"), *ViewTarget->GetName())
		}
	}
}
void AMainTerrain::ClearAll(FMapParams& map_params, FDebugParams& debug_params)
{
	clear_all();
}
void AMainTerrain::AttachDistricts()
{
	TArray<TSharedPtr<District>> districts_to_attach;
	TArray<TSharedPtr<District>> districts_to_remove;
	for (int i = 0; i < figures_array.Num(); i++)
	{
		if (figures_array[i]->is_selected())
		{
			districts_to_attach.Add(figures_array[i]);
		}
	}
	for (int i = 1; i < districts_to_attach.Num(); i++)
	{
		if (districts_to_attach[0]->attach_district(districts_to_attach[i]))
		{
			districts_to_remove.Add(districts_to_attach[i]);
			districts_to_attach[0]->self_figure = districts_to_attach[0]->shrink_figure_with_roads(districts_to_attach[0]->figure, MapParams.road_width, MapParams.main_road_width);
		}
	}
	districts_to_attach.Remove(nullptr);
	figures_array.RemoveAll([&](TSharedPtr<District> dist)
	{
		return districts_to_remove.Contains(dist);
	});
	RedrawAll(is_2d);
}
void AMainTerrain::clear_all()
{
	// TSubclassOf<AProceduralBlockMeshActor> ActorClass;

	// if (!GetWorld() && ActorClass)
	// {
	// 	return;
	// }

	TArray<AActor*> ActorsToDestroy;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProceduralBlockMeshActor::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		AProceduralBlockMeshActor* MyActor = Cast<AProceduralBlockMeshActor>(Actor);
		if (MyActor)
		{
			ActorsToDestroy.Add(MyActor);
		}
	}
	for (AActor* Actor : ActorsToDestroy)
	{
		Actor->Destroy();
	}

	FlushPersistentDebugLines(GetWorld());
}
void AMainTerrain::BeginPlay()
{
	Super::BeginPlay();
	initialize_all();
}

// Called every frame
void AMainTerrain::Tick(float DeltaTime)
{
	
}
inline void AMainTerrain::initialize_all()
{

	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
	MapParams.update_me();
	BaseMaterial = load_material("Pack1", "MaterialBase");
	WaterMaterial = load_material("Pack1", "MaterialWater");
	DocsMaterial = load_material("Pack1", "MaterialDocks");
	RoyalMaterial = load_material("Pack1", "MaterialRoyal");
	ResidentialMaterial = load_material("Pack1", "MaterialResidential");
	LuxuryMaterial = load_material("Pack1", "MaterialLuxury");
	SlumsMaterial = load_material("Pack1", "MaterialSlums");
	BuildingMaterial = load_material("Pack1", "MaterialBuilding");
	RoadMaterial = load_material("Pack1", "MaterialRoad");
	MainRoadMaterial = load_material("Pack1", "MaterialMainRoad");
	WallMaterial = load_material("Pack1", "MaterialWall");
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOrthographicCameraPawn::StaticClass(), FoundActors);
	AOrthographicCameraPawn* OrthographicCamera;
	if (FoundActors.Num() > 0)
	{
		OrthographicCamera = Cast<AOrthographicCameraPawn>(FoundActors[0]);
		if (OrthographicCamera)
		{
			// Теперь OrthographicCamera доступна как объект вашего класса
			UE_LOG(LogTemp, Warning, TEXT("Orthographic camera found: %s"), *OrthographicCamera->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No orthographic cameras found!"));
		return;
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && OrthographicCamera)
	{
		PlayerController->Possess(OrthographicCamera);
	}
	// AActor* OrthographicCamera = FoundActors[0];

	FVector NewLocation = FVector(MapParams.x_size / 2, MapParams.y_size / 2, (MapParams.x_size + MapParams.y_size) / 2);
	OrthographicCamera->SetActorLocation(NewLocation);
	FRotator DownwardRotation = FRotator(-90.00, 0.0, 0.0);
	OrthographicCamera->SetActorRotation(DownwardRotation);
	if (PlayerController)
	{
		PlayerController->bShowMouseCursor = true; // Показываем курсор
		PlayerController->bEnableClickEvents = true; // Включаем обработку событий кликов
		PlayerController->bEnableMouseOverEvents = true; // Включаем обработку событий наведения
		PlayerController->SetViewTargetWithBlend(OrthographicCamera);
	}

	// PrimaryActorTick.bCanEverTick = true;
	// Super::BeginPlay();

	TerrainGen gen(MapParams);
	gen.create_terrain(roads, figures_array, streets_array, river_figures, map_borders_array, debug_points_array);
	gen.empty_all();
	draw_all();
	AActor* ViewTarget = PlayerController->GetViewTarget();
	if (ViewTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"), *ViewTarget->GetName())
	}
}
UMaterialInterface* AMainTerrain::load_material(const FString& TexturePack, const FString& MaterialName)
{
	FString MaterialPath = FString::Printf(TEXT("Material'/Game/Packs/%s/%s.%s'"), *TexturePack, *MaterialName, *MaterialName);

	// Загружаем материал как UMaterialInterface
	UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *MaterialPath));

	// Проверяем, удалось ли загрузить материал
	if (!MaterialInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load material: %s"), *MaterialPath)
	}

	return MaterialInterface;
}


void AMainTerrain::draw_all()
{
	clear_all();
	int ind = 0;
	for (auto b : roads)
	{
		b->debug_ind_ = ind;
		ind++;
	}
	// for (auto b : roads)
	// {
	// 	for (auto bconn : b->conn)
	// 	{
	// 		if (b->get_type() == wall && bconn->node->get_type() == wall && DebugParams.draw_walls)
	// 		{
	// 			auto start_point = b->get_FVector();
	// 			auto end_point = bconn->node->get_FVector();
	// 			start_point.Z = 12;
	// 			end_point.Z = 12;
	// 			DrawDebugLine(GetWorld(), start_point, end_point, FColor::Black, true, -1, 0, 10);
	// 		}
	// 		if (b->get_type() == main_road && bconn->node->get_type() == main_road && DebugParams.draw_main)
	// 		{
	// 			auto start_point = b->get_FVector();
	// 			auto end_point = bconn->node->get_FVector();
	// 			start_point.Z = 1.2f;
	// 			end_point.Z = 1.2f;
	// 			DrawDebugLine(GetWorld(), start_point, end_point, FColor::Red, true, -1, 0, 10);
	// 		}
	// 		if (DebugParams.draw_usual_roads)
	// 		{
	// 			auto start_point = b->get_FVector();
	// 			auto end_point = bconn->node->get_FVector();
	// 			start_point.Z = 1.0f;
	// 			end_point.Z = 1.0f;
	// 			DrawDebugLine(GetWorld(), start_point, end_point, FColor::White, true, -1, 0, 4);
	// 		}
	// 	}
	// }
	AProceduralBlockMeshActor* Base =
	GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());

	// Создаем физическое тело для коллизии
	DrawingObject obj;
	obj.create_mesh_2d(Base, map_borders_array, 0);


	static int32 ActorCounter = 0;
	for (auto& r : figures_array)
	{
		// FColor color;
		// int thickness = 1;

		TArray<TSharedPtr<Point>> figure_to_print;
		if (!r->self_figure.IsEmpty())
		{
			for (auto& p : r->self_figure)
			{
				figure_to_print.Add(MakeShared<Point>(p));
			}
		}
		else
		{
			continue;
		}
		FString ActorName;
		if (r->get_type() == water)
		{
			ActorName = FString::Printf(TEXT("DistrictWater_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, WaterMaterial);
			MeshComponent2->Material = WaterMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
			// create_mesh_2d(MeshComponent2, figure_to_print, 0.02);
		}
		else if (r->get_type() == luxury)
		{
			ActorName = FString::Printf(TEXT("DistrictLuxury_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, LuxuryMaterial);
			MeshComponent2->Material = LuxuryMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}
		else if (r->get_type() == dock)
		{
			ActorName = FString::Printf(TEXT("DistrictDocks_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, DocsMaterial);
			MeshComponent2->Material = DocsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}
		else if (r->get_type() == royal)
		{
			ActorName = FString::Printf(TEXT("DistrictRoyal_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, RoyalMaterial);
			MeshComponent2->Material = RoyalMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}
		else if (r->get_type() == slums)
		{
			ActorName = FString::Printf(TEXT("DistrictSlums_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, SlumsMaterial);
			MeshComponent2->Material = SlumsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}
		else if (r->get_type() == residential)
		{
			ActorName = FString::Printf(TEXT("DistrictResidence_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, ResidentialMaterial);
			MeshComponent2->Material = ResidentialMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}
		else if (r->get_type() == tower)
		{
			ActorName = FString::Printf(TEXT("DistrictResidence_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, MainRoadMaterial);
			MeshComponent2->Material = MainRoadMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}
		else
		{
			ActorName = FString::Printf(TEXT("DistrictUnknown_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, BaseMaterial);
			MeshComponent2->Material = BaseMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.01));
		}

		int house_count = 0;
		for (auto& p : r->houses)
		{
			FString HouseName = FString::Printf(TEXT("%s_House_%d"), *ActorName, ++house_count);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(HouseName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, BuildingMaterial);
			MeshComponent2->Material = BuildingMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			MeshComponent2->SetDistrict(r);

			drawing_houses.Add(DrawingHouse(p, MeshComponent2, 0.02, is_2d));
		}
	}
	// {
	// if (!river_figures.IsEmpty())
	// {
	// 	for (auto& river_figure : river_figures)
	// 	{
	// 		// Algo::Reverse(river_figure.figure);
	//
	// 		FString ActorName = FString::Printf(TEXT("River_%d"), ++ActorCounter);
	// 		AProceduralBlockMeshActor* MeshComponent2 =
	// 		GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
	// 		MeshComponent2->SetActorLabel(ActorName);
	// 		MeshComponent2->ProceduralMesh->SetMaterial(NULL, WaterMaterial);
	// 		MeshComponent2->Material = WaterMaterial;
	// 		MeshComponent2->DefaultMaterial = BaseMaterial;
	// 		create_mesh_2d(MeshComponent2, river_figure->figure, 150);
	// 	}
	// }

	for (auto street : streets_array)
	{
		if (street->type == road)
		{
			FString ActorName = FString::Printf(TEXT("Street_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, RoadMaterial);
			MeshComponent2->Material = RoadMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			drawing_streets.Add(DrawingStreet(street, MeshComponent2, 0.19, false, is_2d));
			// create_mesh_2d(MeshComponent2, street->street_vertexes, 0.19);
		}
		else if (street->type == main_road)
		{
			FString ActorName = FString::Printf(TEXT("StreetMain_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, MainRoadMaterial);
			MeshComponent2->Material = MainRoadMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			drawing_streets.Add(DrawingStreet(street, MeshComponent2, 0.19, false, is_2d));
			// create_mesh_2d(MeshComponent2, street.street_vertexes, 0.021);
		}
		else if (street->type == river)
		{
			FString ActorName = FString::Printf(TEXT("StreetMain_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, WaterMaterial);
			MeshComponent2->Material = WaterMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			drawing_streets.Add(DrawingStreet(street, MeshComponent2, 0.21, false, is_2d));
			// create_mesh_2d(MeshComponent2, street.street_vertexes, 0.021);
		}
		else if (street->type == wall)
		{
			FString ActorName = FString::Printf(TEXT("StreetWall_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, WallMaterial);
			MeshComponent2->Material = WallMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			drawing_streets.Add(DrawingStreet(street, MeshComponent2, 0.022, true, is_2d));
			// if (is_2d)
			// {
			// 	create_mesh_2d(MeshComponent2, street.street_vertexes, 0.022);
			// }
			// else
			// {
			// 	create_mesh_3d(MeshComponent2, street.street_vertexes, 0.022, 10);
			// }
		}
		else
		{
			FString ActorName = FString::Printf(TEXT("StreetUndefined_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, BuildingMaterial);
			MeshComponent2->Material = BuildingMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;

			drawing_streets.Add(DrawingStreet(street, MeshComponent2, 0.24, false, is_2d));
			// create_mesh_2d(MeshComponent2, street.street_vertexes, 0.024);
		}
	}
}
void AMainTerrain::get_cursor_hit_location()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	// Получаем координаты мыши на экране
	float MouseX, MouseY;
	if (PlayerController->GetMousePosition(MouseX, MouseY))
	{
		FVector WorldLocation, WorldDirection;

		// Преобразуем координаты мыши в направление в мире
		if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
		{
			FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
			FVector CameraForwardVector = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
			FVector Start = CameraLocation; // Начальная точка линии (например, от камеры)
			FVector End = Start + (CameraForwardVector * 10000.01); // Конечная точка линии

			// FVector HitLocation = HitResult.Location;
			// FVector HitWatch = HitLocation;
			// DrawDebugString(GetWorld(), HitWatch, HitLocation.ToString(), nullptr, FColor::Red, 50.01,
			// 				true);
			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this); // Игнорировать самого себя
 
			bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End,
				ECC_Visibility // Канал трассировки
			);

			if (bHit && HitResult.Component == BaseComponent)
			{
				// Логируем координаты попадания
				FVector HitLocation = HitResult.Location;
				FVector HitWatch = HitLocation;
				HitWatch.Z += 200;
			}
		}
	}
}
