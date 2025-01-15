#include "MainTerrain.h"
// #include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "OrthographicCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "ProceduralObjectMeshActor.h"
#include "TerrainGen.h"


// #include "Async/AsyncWork.h"

AMainTerrain::AMainTerrain() : BaseMaterial(nullptr)
                             , WaterMaterial(nullptr)
                             , DocsMaterial(nullptr)
                             , RoyalMaterial(nullptr)
                             , ResidenceMaterial(nullptr)
                             , LuxuryMaterial(nullptr)
                             , SlumsMaterial(nullptr)
                             , MapParams()
                             , BaseComponent(nullptr)
{
	// PrimaryActorTick.bCanEverTick = false;
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
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOrthographicCameraActor::StaticClass(), FoundActors);
	MapParams.update_me();

	if (FoundActors.Num() > 0)
	{
		AActor* OrthographicCamera = FoundActors[0];

		FVector NewLocation = FVector(MapParams.x_size / 2, MapParams.y_size / 2, (MapParams.x_size + MapParams.y_size) / 2);
		OrthographicCamera->SetActorLocation(NewLocation);
		FRotator DownwardRotation = FRotator(-90.01, 0.01, 0.01);
		OrthographicCamera->SetActorRotation(DownwardRotation);

		// APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		// if (PlayerController)
		// {
		// 	PlayerController->bShowMouseCursor = true; // Показываем курсор
		// 	PlayerController->bEnableClickEvents = true; // Включаем обработку событий кликов
		// 	PlayerController->bEnableMouseOverEvents = true; // Включаем обработку событий наведения
		// 	PlayerController->SetViewTargetWithBlend(OrthographicCamera);
		// }

		// PrimaryActorTick.bCanEverTick = true;
		// Super::BeginPlay();

		TerrainGen gen(MapParams);
		gen.create_terrain(roads, figures_array, streets_array, river_figure, map_borders_array, debug_points_array);
		gen.empty_all();
		draw_all();
		AActor* ViewTarget = PlayerController->GetViewTarget();
		if (ViewTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"), *ViewTarget->GetName())
		}
	}
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
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOrthographicCameraActor::StaticClass(), FoundActors);
	MapParams.update_me();

	if (FoundActors.Num() > 0)
	{
		AActor* OrthographicCamera = FoundActors[0];

		FVector NewLocation = FVector(MapParams.x_size / 2, MapParams.y_size / 2, (MapParams.x_size + MapParams.y_size) / 2);
		OrthographicCamera->SetActorLocation(NewLocation);
		FRotator DownwardRotation = FRotator(-90.01, 0.01, 0.01);
		OrthographicCamera->SetActorRotation(DownwardRotation);

		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
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
		gen.create_terrain(roads, figures_array, streets_array, river_figure, map_borders_array, debug_points_array);
		gen.empty_all();
		draw_all();
		AActor* ViewTarget = PlayerController->GetViewTarget();
		if (ViewTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"), *ViewTarget->GetName())
		}
	}
}

void AMainTerrain::create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight,
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

void AMainTerrain::create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
                                  float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_FVector());
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}
void AMainTerrain::create_mesh_3d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
                                  float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->point);
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}

void AMainTerrain::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<FVector> BaseVertices, float StarterHeight)
{
	
	int32 NumVertices = BaseVertices.Num();
	if (NumVertices < 5)
	{
		UE_LOG(LogTemp, Log, TEXT("aaaa"))
	}

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
	VerticesRemembered = Vertices;

	// Создаем меш
	Mesh->ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	return;
}
void AMainTerrain::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Node>> BaseVertices,
                                  float StarterHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_FVector());
	}
	create_mesh_2d(Mesh, vertices, StarterHeight);
}
void AMainTerrain::create_mesh_2d(AProceduralBlockMeshActor* Mesh, TArray<TSharedPtr<Point>> BaseVertices,
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

void AMainTerrain::draw_all()
{
	FlushPersistentDebugLines(GetWorld());
	int ind = 0;
	for (auto b : roads)
	{
		b->debug_ind_ = ind;
		ind++;
	}
	UE_LOG(LogTemp, Warning, TEXT("улиц - %d"), streets_array.Num())
	UE_LOG(LogTemp, Warning, TEXT("узлов - %d"), roads.Num())
	for (auto b : roads)
	{
		for (auto bconn : b->conn)
		{
			UE_LOG(LogTemp, Warning, TEXT("%d - %d"), b->debug_ind_, bconn->node->debug_ind_)

			if (b->get_type() == wall && bconn->node->get_type() == wall && DebugParams.draw_walls)
			{
				auto start_point = b->get_FVector();
				auto end_point = bconn->node->get_FVector();
				start_point.Z = 12;
				end_point.Z = 12;
				DrawDebugLine(GetWorld(), start_point, end_point, FColor::Black, true, -1, 0, 10);
			}
			if (b->get_type() == main_road && bconn->node->get_type() == main_road && DebugParams.draw_main)
			{
				auto start_point = b->get_FVector();
				auto end_point = bconn->node->get_FVector();
				start_point.Z = 1.2f;
				end_point.Z = 1.2f;
				DrawDebugLine(GetWorld(), start_point, end_point, FColor::Red, true, -1, 0, 10);
			}
			if (DebugParams.draw_usual_roads)
			{
				auto start_point = b->get_FVector();
				auto end_point = bconn->node->get_FVector();
				start_point.Z = 1.0f;
				end_point.Z = 1.0f;
				DrawDebugLine(GetWorld(), start_point, end_point, FColor::White, true, -1, 0, 4);
			}
		}
	}
	AProceduralBlockMeshActor* Base =
	GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());

	// Создаем физическое тело для коллизии
	create_mesh_2d(Base, map_borders_array, 0);


	static int32 ActorCounter = 0;
	for (auto& r : figures_array)
	{
		// FColor color;
		// int thickness = 1;

		TArray<TSharedPtr<Point>> figure_to_print;
		if (!r.self_figure.IsEmpty())
		{
			for (auto& p : r.self_figure)
			{
				figure_to_print.Add(MakeShared<Point>(p));
			}
		}
		else
		{
			continue;
		}
		FString ActorName;
		if (r.get_type() == luxury)
		{
			ActorName = FString::Printf(TEXT("DistrictLuxury_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, LuxuryMaterial);
			MeshComponent2->Material = LuxuryMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		}
		else if (r.get_type() == dock)
		{
			ActorName = FString::Printf(TEXT("DistrictDocks_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, DocsMaterial);
			MeshComponent2->Material = DocsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		}
		else if (r.get_type() == royal)
		{
			ActorName = FString::Printf(TEXT("DistrictRoyal_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, RoyalMaterial);
			MeshComponent2->Material = RoyalMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		}
		else if (r.get_type() == slums)
		{
			ActorName = FString::Printf(TEXT("DistriceSlums_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, SlumsMaterial);
			MeshComponent2->Material = SlumsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		}
		else if (r.get_type() == residential)
		{
			ActorName = FString::Printf(TEXT("DistrictResidence_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, ResidenceMaterial);
			MeshComponent2->Material = ResidenceMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, figure_to_print, 0.01);
		}
		else
		{
			ActorName = FString::Printf(TEXT("Building_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, BuildingMaterial);
			MeshComponent2->Material = BuildingMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			if (is_2d)
			{
				create_mesh_2d(MeshComponent2, figure_to_print, 0.02);
			}
		}

		int house_count = 0;
		for (auto& p : r.houses)
		{
			FString HouseName = FString::Printf(TEXT("%s_House_%d"), *ActorName, ++house_count);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(HouseName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, BuildingMaterial);
			MeshComponent2->Material = BuildingMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			if (is_2d)
			{
				create_mesh_2d(MeshComponent2, p.house_figure, 0.02);
			}
			else
			{
				create_mesh_3d(MeshComponent2, p.house_figure, 0.02, p.height);
			}
		}
	}
	{
		if (!river_figure.figure.IsEmpty())
		{
			Algo::Reverse(river_figure.figure);

			FString ActorName = FString::Printf(TEXT("River_%d"), ++ActorCounter);
			AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetActorLabel(ActorName);
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, WaterMaterial);
			MeshComponent2->Material = WaterMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, river_figure.figure, 0.02);
		}
	}

	for (auto street : streets_array)
	{
		FString ActorName = FString::Printf(TEXT("Street_%d"), ++ActorCounter);
		AProceduralBlockMeshActor* MeshComponent2 =
		GetWorld()->SpawnActor<AProceduralBlockMeshActor>(AProceduralBlockMeshActor::StaticClass());
		MeshComponent2->SetActorLabel(ActorName);
		if (street.type == main_road)
		{
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, RoyalMaterial);
			MeshComponent2->Material = RoyalMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, street.street_vertexes, 0.021);
		}
		else if (street.type == road)
		{
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, LuxuryMaterial);
			MeshComponent2->Material = LuxuryMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, street.street_vertexes, 0.022);
		}
		else if (street.type == wall)
		{
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, SlumsMaterial);
			MeshComponent2->Material = SlumsMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, street.street_vertexes, 0.023);
		}
		else
		{
			MeshComponent2->ProceduralMesh->SetMaterial(NULL, BuildingMaterial);
			MeshComponent2->Material = BuildingMaterial;
			MeshComponent2->DefaultMaterial = BaseMaterial;
			create_mesh_2d(MeshComponent2, street.street_vertexes, 0.024);
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
