#include "MainTerrain.h"
// #include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "OrthographicCameraPawn.h"
#include "ProceduralObjectMeshActor.h"
#include "TerrainGen.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

void DrawingObject::create_mesh_3d(AProceduralBlockMeshActor* Mesh,
                                   TArray<FVector> BaseVertices,
                                   float StarterHeight,
                                   float ExtrusionHeight)
{
	Mesh->ProceduralMesh->ClearAllMeshSections();
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
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().
		                          InverseTransformPosition(Vertex);
		Base.Add(local_vertex + FVector(0, 0, StarterHeight));
		Vertices.Add(local_vertex + FVector(0, 0, StarterHeight));
	}

	// Добавляем вершины верхней стороны, сдвинутые вверх на ExtrudeHeight
	for (auto Vertex : BaseVertices)
	{
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().
		                          InverseTransformPosition(Vertex);
		Vertices.Add(
			local_vertex + FVector(0, 0, StarterHeight) + FVector(
				0, 0, ExtrusionHeight));
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
	Mesh->ProceduralMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents,
		true);
}

void DrawingObject::create_mesh_3d(AProceduralBlockMeshActor* Mesh,
                                   TArray<TSharedPtr<Node>> BaseVertices,
                                   float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_FVector());
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}

void DrawingObject::create_mesh_3d(AProceduralBlockMeshActor* Mesh,
                                   TArray<TSharedPtr<Point>> BaseVertices,
                                   float StarterHeight, float ExtrusionHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->point);
	}
	create_mesh_3d(Mesh, vertices, StarterHeight, ExtrusionHeight);
}

void DrawingObject::create_mesh_2d(AProceduralBlockMeshActor* Mesh,
                                   TArray<FVector> BaseVertices,
                                   float StarterHeight)
{
	Mesh->ProceduralMesh->ClearAllMeshSections();
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
		auto local_vertex = Mesh->ProceduralMesh->GetComponentTransform().
		                          InverseTransformPosition(Vertex);
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
	Mesh->ProceduralMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}

void DrawingObject::create_mesh_2d(AProceduralBlockMeshActor* Mesh,
                                   TArray<TSharedPtr<Node>> BaseVertices,
                                   float StarterHeight)
{
	TArray<FVector> vertices;
	for (auto BaseVertex : BaseVertices)
	{
		vertices.Add(BaseVertex->get_FVector());
	}
	create_mesh_2d(Mesh, vertices, StarterHeight);
}

void DrawingObject::create_mesh_2d(AProceduralBlockMeshActor* Mesh,
                                   TArray<TSharedPtr<Point>> BaseVertices,
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

AMainTerrain::AMainTerrain() : MapParams()
                               , BaseComponent(nullptr)
// , BaseMaterial(nullptr)
// , WaterMaterial(nullptr)
// , DocsMaterial(nullptr)
// , RoyalMaterial(nullptr)
// , ResidentialMaterial(nullptr)
// , LuxuryMaterial(nullptr)
// , SlumsMaterial(nullptr)
// , BuildingMaterial(nullptr)
// , RoadMaterial(nullptr)
// , MainRoadMaterial(nullptr)
// , WallMaterial(nullptr), PavementMaterial(nullptr)
{
	TArray<unsigned int> empty_arr{};
	selected_objects = MakeShared<TArray<unsigned int>>(empty_arr);
	// PrimaryActorTick.bCanEverTick = false;
}

void AMainTerrain::RedrawAll(bool is_2d_)
{
	is_2d = is_2d_;
	for (auto s : drawing_streets)
	{
		if (s.is_changing)
		{
			s.is_2d = is_2d_;
			// s.delete_mesh();
			s.draw_me();
		}
	}
	for (auto s : drawing_houses)
	{
		s.is_2d = is_2d_;
		// s.delete_mesh();
		s.draw_me();
	}
	// draw_all();
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllDistrictsSelected()
{
	TArray<AProceduralBlockMeshActor*> districts_to_get{};
	for (int i = 0; i < drawing_districts.Num(); i++)
	{
		// UE_LOG(LogTemp, Warning, TEXT("for in %i: %p"), i, drawing_districts[i].district.Get())
		if (drawing_districts[i].district->is_selected())
		{
			districts_to_get.Add(drawing_districts[i].mesh);
		}
	}
	return districts_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllStreetsSelected()
{
	TArray<AProceduralBlockMeshActor*> streets_to_get{};
	for (int i = 0; i < drawing_streets.Num(); i++)
	{
		if (drawing_streets[i].street->is_selected())
		{
			streets_to_get.Add(drawing_streets[i].mesh);
		}
	}
	return streets_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllHousesSelected()
{
	return get_all_houses_of_type_selected("House");
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::get_all_houses_of_type_selected(FString type_name)
{
	TArray<AProceduralBlockMeshActor*> houses_to_get{};
	for (int i = 0; i < drawing_houses.Num(); i++)
	{
		// UE_LOG(LogTemp, Warning, TEXT("for in %i: %p"), i, drawing_districts[i].district.Get())
		if (drawing_houses[i].house->is_selected() && drawing_houses[i].house->get_object_type() == type_name)
		{
			houses_to_get.Add(drawing_houses[i].mesh);
		}
	}
	return houses_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllOjectsOfTypeSelected(FString type_name)
{
	TArray<AProceduralBlockMeshActor*> selected_array;
	if (type_name == "District")
	{
		return GetAllDistrictsSelected();
	}
	else if (type_name == "House" || type_name == "Pavement")
	{
		get_all_houses_of_type_selected(type_name);
	}
	else if (type_name == "Street")
	{
		return GetAllStreetsSelected();
	}
	return {};
}


TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllDistricts()
{
	TArray<AProceduralBlockMeshActor*> districts_to_get{};
	for (int i = 0; i < drawing_districts.Num(); i++)
	{
		districts_to_get.Add(drawing_districts[i].mesh);
	}
	return districts_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllStreets()
{
	TArray<AProceduralBlockMeshActor*> streets_to_get{};
	for (int i = 0; i < drawing_streets.Num(); i++)
	{
		streets_to_get.Add(drawing_streets[i].mesh);
	}
	return streets_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllHouses()
{
	return get_all_houses_of_type("House");
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::get_all_houses_of_type(FString type)
{
	TArray<AProceduralBlockMeshActor*> houses_to_get{};
	for (int i = 0; i < drawing_houses.Num(); i++)
	{
		if (drawing_houses[i].house->get_object_type() == type)
		{
			houses_to_get.Add(drawing_houses[i].mesh);
		}
	}
	return houses_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetAllOjectsOfType(FString type_name)
{
	TArray<AProceduralBlockMeshActor*> selected_array;
	if (type_name == "District")
	{
		return GetAllDistricts();
	}
	else if (type_name == "House" || type_name == "Pavement")
	{
		return get_all_houses_of_type(type_name);
	}
	else if (type_name == "Street")
	{
		return GetAllStreets();
	}
	return {};
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::UnselectAllDistricts()
{
	TArray<AProceduralBlockMeshActor*> districts_to_get{};
	for (int i = 0; i < drawing_districts.Num(); i++)
	{
		drawing_districts[i].district->unselect();
	}
	return districts_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::UnselectAllStreets()
{
	TArray<AProceduralBlockMeshActor*> streets_to_get{};
	for (int i = 0; i < drawing_streets.Num(); i++)
	{
		drawing_streets[i].street->unselect();
	}
	return streets_to_get;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::UnselectAllHouses()
{
	TArray<AProceduralBlockMeshActor*> houses_to_get{};
	for (int i = 0; i < drawing_houses.Num(); i++)
	{
		drawing_houses[i].house->unselect();
	}
	return houses_to_get;
}

AProceduralBlockMeshActor* AMainTerrain::GetLastSelected()
{
	TArray<AProceduralBlockMeshActor*> selected;
	// return selected_objects->Last();
	for (auto distr : GetAllDistrictsSelected())
	{
		if (distr->object->get_id() == selected_objects->Last())
			return distr;
	}
	for (auto street : GetAllStreetsSelected())
	{
		if (street->object->get_id() == selected_objects->Last())
			return street;
	}
	for (auto house : GetAllHousesSelected())
	{
		if (house->object->get_id() == selected_objects->Last())
			return house;
	}
	return nullptr;
}

AProceduralBlockMeshActor* AMainTerrain::GetPrevSelected()
{
	// TArray<AProceduralBlockMeshActor*> selected;
	// return selected_objects->Last();

	TArray<unsigned int> Result;
	for (const unsigned int& Element : *selected_objects)
	{
		if (!prev_selected_objects->Contains(Element))
		{
			Result.Add(Element);
		}
	}

	for (const unsigned int& Element : *prev_selected_objects)
	{
		if (!selected_objects->Contains(Element))
		{
			Result.Add(Element);
		}
	}

	for (auto distr : GetAllDistricts())
	{
		if (distr->object->get_id() == Result.Last())
			return distr;
	}
	for (auto street : GetAllStreets())
	{
		if (street->object->get_id() == Result.Last())
			return street;
	}
	for (auto house : GetAllHouses())
	{
		if (house->object->get_id() == Result.Last())
			return house;
	}
	return nullptr;
}

TArray<AProceduralBlockMeshActor*> AMainTerrain::GetLastTypeSelected()
{
	for (auto distr : GetAllDistrictsSelected())
	{
		if (distr->object->get_id() == selected_objects->Last())
			return GetAllDistrictsSelected();
	}
	for (auto distr : GetAllStreetsSelected())
	{
		if (distr->object->get_id() == selected_objects->Last())
			return GetAllStreetsSelected();
	}
	for (auto distr : GetAllHousesSelected())
	{
		if (distr->object->get_id() == selected_objects->Last())
			return GetAllHousesSelected();
	}
	return {};
}


void AMainTerrain::ReinitializeActor(FMapParams& map_params,
                                     FDebugParams& debug_params)
{
	roads.Empty();
	figures_array.Empty();
	streets_array.Empty();
	map_borders_array.Empty();
	debug_points_array.Empty();
	debug2_points_array.Empty();
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
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
	                                      AOrthographicCameraPawn::StaticClass(),
	                                      FoundActors);
	MapParams.update_me();

	if (FoundActors.Num() > 0)
	{
		// AActor* OrthographicCamera = FoundActors[0];

		// FVector NewLocation = FVector(MapParams.x_size / 2,
		//                               MapParams.y_size / 2,
		//                               (MapParams.x_size + MapParams.y_size) /
		//                               2);
		// OrthographicCamera->SetActorLocation(NewLocation);
		// FRotator DownwardRotation = FRotator(0.00, -90.00, 0.00);
		// OrthographicCamera->SetActorRotation(DownwardRotation);

		TerrainGen gen(MapParams, ResidentialHousesParams);
		gen.create_terrain(roads, figures_array, streets_array, segments_array,
		                   river_figures, map_borders_array,
		                   debug_points_array, debug2_points_array);
		// gen.empty_all(river);
		draw_all();
		AActor* ViewTarget = PlayerController->GetViewTarget();
		if (ViewTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"),
			       *ViewTarget->GetName())
		}
	}
}

void AMainTerrain::ClearAll(FMapParams& map_params, FDebugParams& debug_params)
{
	clear_all();
}

void AMainTerrain::AttachDistricts()
{
	TArray<TSharedPtr<District>> districts_to_attach{};
	TArray<TSharedPtr<District>> districts_to_remove{};
	TArray<TSharedPtr<Street>> segments_to_delete{};
	for (int i = 0; i < drawing_districts.Num(); i++)
	{
		// UE_LOG(LogTemp, Warning, TEXT("for in %i: %p"), i, drawing_districts[i].district.Get())
		if (drawing_districts[i].district->is_selected())
		{
			UE_LOG(LogTemp, Warning, TEXT("selected in loop"))
			districts_to_attach.Add(drawing_districts[i].district);
		}
	}
	if (districts_to_attach.Num() < 2)
	{
		return;
	}
	districts_to_attach.Sort(
		[&](TSharedPtr<District> d1, TSharedPtr<District> d2)
		{
			return d1->is_adjacent(d2);
		});
	for (int i = 1; i < districts_to_attach.Num(); i++)
	{
		TArray<TSharedPtr<Node>> figure1 = districts_to_attach[i - 1]->figure;
		TArray<TSharedPtr<Node>> figure2 = districts_to_attach[i]->figure;
		if (districts_to_attach[i - 1]->
			attach_district(districts_to_attach[i], segments_to_delete) &&
			districts_to_attach[0]->is_adjacent(districts_to_attach[i]))
		{
			districts_to_remove.Add(districts_to_attach[i]);
			districts_to_attach[i - 1]->self_figure = districts_to_attach[0]->
				shrink_figure_with_roads(districts_to_attach[0]->figure,
				                         MapParams.road_width,
				                         MapParams.main_road_width);
			for (auto j = 0; j < figure1.Num(); j++)
			{
				if (districts_to_attach[0]->figure.Contains(figure1[j]))
				{
					for (auto& c : figure1[j]->conn)
					{
						segments_to_delete.AddUnique(c->get_street());
					}
					figure1[j]->delete_me();
				}
			}
			for (auto j = 0; j < figure2.Num(); j++)
			{
				if (districts_to_attach[i - 1]->figure.Contains(figure2[j]))
				{
					for (auto& c : figure2[j]->conn)
					{
						segments_to_delete.AddUnique(c->get_street());
					}
					figure2[j]->delete_me();
				}
			}
		}
	}

	drawing_streets.RemoveAll([&](DrawingStreet& d_street)
	{
		if (segments_to_delete.Contains(d_street.street))
		{
			d_street.delete_mesh();
			d_street.street.Reset();
			return true;
		}
		return false;
	});

	for (auto& dd : drawing_districts)
	{
		if (dd.district->is_selected())
		{
			dd.district->self_figure = dd.district->shrink_figure_with_roads(
				dd.district->figure, MapParams.road_width,
				MapParams.main_road_width);
			dd.draw_me();
			dd.district->unselect();
		}
		if (districts_to_attach[0] == dd.district)
		{
			dd.draw_me();
		}
	}

	drawing_districts.RemoveAll([&](DrawingDistrict& d_district)
	{
		if (districts_to_remove.Contains(d_district.district))
		{
			d_district.delete_mesh();
			d_district.district.Reset();
			return true;
		}
		return false;
	});
}

void AMainTerrain::DivideDistricts()
{
	TArray<int32> IndicesToRemove;
	for (int i = 0; i < drawing_districts.Num(); i++)
	{
		// UE_LOG(LogTemp, Warning, TEXT("for in %i: %p"), i, drawing_districts[i].district.Get())
		if (drawing_districts[i].district->is_selected())
		{
			IndicesToRemove.Add(i);
			AProceduralBlockMeshActor* MeshComponent =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(
					AProceduralBlockMeshActor::StaticClass());
			MeshComponent->SetSelectedObject(selected_objects, prev_selected_objects);
			MeshComponent->SetActorLabel(
				drawing_districts[i].mesh->GetActorLabel());
			// MeshComponent->ProceduralMesh->SetMaterial(
			// 	0, drawing_districts[i].mesh->Material);
			// MeshComponent->Material = drawing_districts[i].mesh->Material;
			// MeshComponent->DefaultMaterial = drawing_districts[i].mesh->DefaultMaterial;
			auto District1 = MakeShared<District>();
			District1->set_district_type(drawing_districts[i].district->get_district_type());

			AProceduralBlockMeshActor* MeshComponent2 =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(
					AProceduralBlockMeshActor::StaticClass());
			MeshComponent2->SetSelectedObject(selected_objects, prev_selected_objects);
			MeshComponent2->SetActorLabel(
				drawing_districts[i].mesh->GetActorLabel());
			// MeshComponent2->ProceduralMesh->SetMaterial(
			// 	0, drawing_districts[i].mesh->Material);
			// MeshComponent2->Material = drawing_districts[i].mesh->Material;
			// MeshComponent2->DefaultMaterial = drawing_districts[i].mesh->DefaultMaterial;
			auto District2 = MakeShared<District>();
			District2->set_district_type(drawing_districts[i].district->get_district_type());

			DrawingDistrict dd1(District1, MeshComponent,
			                    drawing_districts[i].start_height);
			DrawingDistrict dd2(District2, MeshComponent2,
			                    drawing_districts[i].start_height);

			TSharedPtr<Street> street;
			// street->type = point_type::road;
			drawing_districts[i].district->divide_me(
				dd1.district, dd2.district, street);

			dd1.district->self_figure.Empty();
			dd2.district->self_figure.Empty();

			// for (auto& dd1d : dd1.district->figure)
			// {
			// 	dd1.district->self_figure.Add(dd1d->get_FVector());
			// }
			// for (auto& dd2d : dd2.district->figure)
			// {
			// 	dd2.district->self_figure.Add(dd2d->get_FVector());
			// }
			dd1.district->self_figure = dd1.district->shrink_figure_with_roads(
				dd1.district->figure, MapParams.road_width,
				MapParams.main_road_width);
			dd2.district->self_figure = dd2.district->shrink_figure_with_roads(
				dd2.district->figure, MapParams.road_width,
				MapParams.main_road_width);

			drawing_districts.Add(dd1);
			drawing_districts.Add(dd2);
			dd1.district->unselect();
			dd2.district->unselect();
			dd1.draw_me();
			dd2.draw_me();

			// MeshComponent2->ProceduralMesh->SetMaterial(0, RoadMaterial);
			// MeshComponent2->Material = RoadMaterial;
			// MeshComponent2->DefaultMaterial = BaseMaterial;
			// DrawingStreet ds(street, MeshComponent2, 0.03, true);
			// ds.redraw_me(3);
			// drawing_streets.Add(ds);
			drawing_districts[i].delete_mesh();
			// drawing_districts[i].district.Reset();
		}
	}
	for (int32 i = IndicesToRemove.Num() - 1; i >= 0; --i)
	{
		drawing_districts.RemoveAt(IndicesToRemove[i]);
	}
}

void AMainTerrain::clear_all()
{
	TArray<AActor*> ActorsToDestroy;
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(),
	                                      AProceduralBlockMeshActor::StaticClass(),
	                                      FoundActors);
	for (AActor* Actor : FoundActors)
	{
		AProceduralBlockMeshActor* MyActor = Cast<
			AProceduralBlockMeshActor>(Actor);
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
	// Vulkan.WaitForIdleOnSubmit=1;
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
	MapParams.update_me();
	// BaseMaterial = load_material("Pack1", "MaterialBase");
	// WaterMaterial = load_material("Pack1", "MaterialWater");
	// DocsMaterial = load_material("Pack1", "MaterialDocks");
	// RoyalMaterial = load_material("Pack1", "MaterialRoyal");
	// ResidentialMaterial = load_material("Pack1", "MaterialResidential");
	// LuxuryMaterial = load_material("Pack1", "MaterialLuxury");
	// SlumsMaterial = load_material("Pack1", "MaterialSlums");
	// BuildingMaterial = load_material("Pack1", "MaterialBuilding");
	// RoadMaterial = load_material("Pack1", "MaterialRoad");
	// MainRoadMaterial = load_material("Pack1", "MaterialMainRoad");
	// WallMaterial = load_material("Pack1", "MaterialWall");
	// PavementMaterial = load_material("Pack1", "MaterialPavement");
	// TArray<AActor*> FoundActors;
	// UGameplayStatics::GetAllActorsOfClass(GetWorld(),
	//                                       AOrthographicCameraPawn::StaticClass(),
	//                                       FoundActors);
	// AOrthographicCameraPawn* OrthographicCamera;
	// if (FoundActors.Num() > 0)
	// {
	// 	OrthographicCamera = Cast<AOrthographicCameraPawn>(FoundActors[0]);
	// 	if (OrthographicCamera)
	// 	{
	// 		// Теперь OrthographicCamera доступна как объект вашего класса
	// 		UE_LOG(LogTemp, Warning, TEXT("Orthographic camera found: %s"),
	// 		       *OrthographicCamera->GetName());
	// 	}
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("No orthographic cameras found!"));
	// 	return;
	// }
	APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(this, 0);
	// if (PlayerController && OrthographicCamera)
	// {
	// 	PlayerController->Possess(OrthographicCamera);
	// }
	// // AActor* OrthographicCamera = FoundActors[0];
	//
	// FVector NewLocation = FVector(MapParams.x_size / 2, MapParams.y_size / 2,
	//                               (MapParams.x_size + MapParams.y_size) / 2);
	// OrthographicCamera->SetActorLocation(NewLocation);
	// FRotator DownwardRotation = FRotator(-90.00, 0.0, 0.0);
	// OrthographicCamera->SetActorRotation(DownwardRotation);
	// if (PlayerController)
	// {
	// 	PlayerController->bShowMouseCursor = true; // Показываем курсор
	// 	PlayerController->bEnableClickEvents = true;
	// 	// Включаем обработку событий кликов
	// 	PlayerController->bEnableMouseOverEvents = true;
	// 	// Включаем обработку событий наведения
	// 	PlayerController->SetViewTargetWithBlend(OrthographicCamera);
	// }

	// PrimaryActorTick.bCanEverTick = true;
	// Super::BeginPlay();

	TerrainGen gen(MapParams, ResidentialHousesParams);
	gen.create_terrain(roads, figures_array, streets_array, segments_array,
	                   river_figures, map_borders_array, debug_points_array, debug2_points_array);
	// gen.empty_all(river);
	draw_all();
	AActor* ViewTarget = PlayerController->GetViewTarget();
	if (ViewTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("Current View Target: %s"),
		       *ViewTarget->GetName())
	}
}

// UMaterialInterface* AMainTerrain::load_material(const FString& TexturePack,
//                                                 const FString& MaterialName)
// {
// 	FString MaterialPath = FString::Printf(
// 		TEXT("Material Loading'/Game/Packs/%s/%s.%s'"), *TexturePack, *MaterialName,
// 		*MaterialName);
//
// 	// Загружаем материал как UMaterialInterface
// 	UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(
// 		StaticLoadObject(UMaterialInterface::StaticClass(), nullptr,
// 		                 *MaterialPath));
//
// 	// Проверяем, удалось ли загрузить материал
// 	if (!MaterialInterface)
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Failed to load material: %s"),
// 		       *MaterialPath)
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Material loaded: %s"),
// 			   *MaterialPath)
// 	}
//
// 	return MaterialInterface;
// }

void AMainTerrain::draw_all()
{
	clear_all();

	for (auto& p : debug2_points_array)
	{
		FVector Start = p;
		FVector End = p;
		Start.Z = -10;
		End.Z = 10;

		DrawDebugLine(GetWorld(), Start, End, FColor::Red, true, 0.0f, 0, 15.0f);
	}
	for (auto& p : debug_points_array)
	{
		FVector Start = p;
		FVector End = p;
		Start.Z = -10;
		End.Z = 10;

		DrawDebugLine(GetWorld(), Start, End, FColor::Green, true, 0.0f, 0, 15.0f);
	}

	int ind = 0;
	for (auto b : roads)
	{
		b->debug_ind_ = ind;
		ind++;
	}
	AProceduralBlockMeshActor* Base =
		GetWorld()->SpawnActor<AProceduralBlockMeshActor>(
			AProceduralBlockMeshActor::StaticClass());

	DrawingObject obj;
	obj.create_mesh_2d(Base, map_borders_array, 0.01);


	static int32 ActorCounter = 0;
	for (auto& r : figures_array)
	{
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
		AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(
				AProceduralBlockMeshActor::StaticClass());
		MeshComponent2->SetSelectedObject(selected_objects, prev_selected_objects);
		// MeshComponent2->ProceduralMesh->SetMaterial(0, BaseMaterial);
		// MeshComponent2->Material = BaseMaterial;
		if (r->get_district_type() == district_type::water)
		{
			ActorName = FString::Printf(
				TEXT("DistrictWater_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, WaterMaterial);
			// MeshComponent2->Material = WaterMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
			// create_mesh_2d(MeshComponent2, figure_to_print, 0.02);
		}
		else if (r->get_district_type() == district_type::luxury)
		{
			ActorName = FString::Printf(
				TEXT("DistrictLuxury_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, LuxuryMaterial);
			// MeshComponent2->Material = LuxuryMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}
		else if (r->get_district_type() == district_type::dock)
		{
			ActorName = FString::Printf(
				TEXT("DistrictDocks_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, DocsMaterial);
			// MeshComponent2->Material = DocsMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}
		else if (r->get_district_type() == district_type::royal)
		{
			ActorName = FString::Printf(
				TEXT("DistrictRoyal_%d"), ++ActorCounter);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, RoyalMaterial);
			// MeshComponent2->Material = RoyalMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}
		else if (r->get_district_type() == district_type::slums)
		{
			ActorName = FString::Printf(
				TEXT("DistrictSlums_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, SlumsMaterial);
			// MeshComponent2->Material = SlumsMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}
		else if (r->get_district_type() == district_type::residential)
		{
			ActorName = FString::Printf(
				TEXT("DistrictResidence_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, ResidentialMaterial);
			// MeshComponent2->Material = ResidentialMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}
		else if (r->get_district_type() == district_type::tower)
		{
			ActorName = FString::Printf(TEXT("Tower_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, MainRoadMaterial);
			// MeshComponent2->Material = MainRoadMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}
		else
		{
			ActorName = FString::Printf(
				TEXT("DistrictUnknown_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, BaseMaterial);
			// MeshComponent2->Material = BaseMaterial;
			drawing_districts.Add(DrawingDistrict(r, MeshComponent2, 0.02));
		}

		int house_count = 0;
		for (auto& p : r->houses)
		{
			FString type = p->get_object_type();
			FString HouseName = FString::Printf(
				TEXT("%s_%s_%d"), *ActorName, *type, ++house_count);
			AProceduralBlockMeshActor* MeshComponent =
				GetWorld()->SpawnActor<AProceduralBlockMeshActor>(
					AProceduralBlockMeshActor::StaticClass());
			MeshComponent->SetSelectedObject(selected_objects, prev_selected_objects);
			MeshComponent->SetActorLabel(HouseName);

			// if (p->get_object_type() == "House")
			// 	MeshComponent->ProceduralMesh->SetMaterial(0, BuildingMaterial);
			// else if (p->get_object_type() == "Pavement")
			// 	MeshComponent->ProceduralMesh->SetMaterial(0, PavementMaterial);
			// else
			// 	MeshComponent->ProceduralMesh->SetMaterial(0, BaseMaterial);

			// MeshComponent->DefaultMaterial = BaseMaterial;
			MeshComponent->SetDynamicObject(p);

			drawing_houses.Add(DrawingHouse(p, MeshComponent, 0.04, is_2d));
		}
	}

	for (auto street : segments_array)
	{
		AProceduralBlockMeshActor* MeshComponent2 =
			GetWorld()->SpawnActor<AProceduralBlockMeshActor>(
				AProceduralBlockMeshActor::StaticClass());
		MeshComponent2->SetSelectedObject(selected_objects, prev_selected_objects);
		// MeshComponent2->ProceduralMesh->SetMaterial(0, RoadMaterial);
		// MeshComponent2->Material = RoadMaterial;
		// MeshComponent2->DefaultMaterial = BaseMaterial;
		MeshComponent2->SetDynamicObject(street);
		if (street->type == point_type::road)
		{
			FString ActorName = FString::Printf(
				TEXT("Street_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, RoadMaterial);
			// MeshComponent2->Material = RoadMaterial;
			drawing_streets.Add(
				DrawingStreet(street, MeshComponent2, 0.03, is_2d));
			// create_mesh_2d(MeshComponent2, street->street_vertexes, 0.19);
		}
		else if (street->type == point_type::main_road)
		{
			FString ActorName = FString::Printf(
				TEXT("StreetMain_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, MainRoadMaterial);
			// MeshComponent2->Material = MainRoadMaterial;
			drawing_streets.Add(
				DrawingStreet(street, MeshComponent2, 0.031, is_2d));
			// create_mesh_2d(MeshComponent2, street.street_vertexes, 0.021);
		}
		else if (street->type == point_type::river)
		{
			FString ActorName = FString::Printf(
				TEXT("StreetMain_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, WaterMaterial);
			// MeshComponent2->Material = WaterMaterial;
			drawing_streets.Add(
				DrawingStreet(street, MeshComponent2, 0.032, is_2d));
			// create_mesh_2d(MeshComponent2, street.street_vertexes, 0.021);
		}
		else if (street->type == point_type::wall)
		{
			FString ActorName = FString::Printf(
				TEXT("StreetWall_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, WallMaterial);
			// MeshComponent2->Material = WallMaterial;
			drawing_streets.Add(
				DrawingStreet(street, MeshComponent2, 0.033, is_2d));
		}
		else
		{
			FString ActorName = FString::Printf(
				TEXT("StreetUndefined_%d"), ++ActorCounter);
			MeshComponent2->SetActorLabel(ActorName);
			// MeshComponent2->ProceduralMesh->SetMaterial(0, BuildingMaterial);
			// MeshComponent2->Material = BuildingMaterial;
			drawing_streets.Add(
				DrawingStreet(street, MeshComponent2, 0.034, is_2d));
		}
	}
	for (auto a : drawing_streets)
	{
		a.draw_me();
	}
	for (auto a : drawing_houses)
	{
		a.draw_me();
	}
	for (auto a : drawing_districts)
	{
		a.draw_me();
	}
}

void AMainTerrain::get_cursor_hit_location()
{
	APlayerController* PlayerController = GetWorld()->
		GetFirstPlayerController();
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
		if (PlayerController->DeprojectScreenPositionToWorld(
			MouseX, MouseY, WorldLocation, WorldDirection))
		{
			FVector CameraLocation = PlayerController->PlayerCameraManager->
			                                           GetCameraLocation();
			FVector CameraForwardVector = PlayerController->PlayerCameraManager
			                                              ->GetCameraRotation().Vector();
			FVector Start = CameraLocation;
			// Начальная точка линии (например, от камеры)
			FVector End = Start + (CameraForwardVector * 10000.02);
			// Конечная точка линии

			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this); // Игнорировать самого себя

			bool bHit = GetWorld()->LineTraceSingleByChannel(
				HitResult, Start, End,
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
