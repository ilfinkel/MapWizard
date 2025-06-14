#include "ProceduralObjectMeshActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"


AProceduralBlockMeshActor::AProceduralBlockMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponentName = TEXT("ProceduralMesh");

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(
		*MeshComponentName);
	RootComponent = ProceduralMesh;
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// Включаем коллизии
	ProceduralMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	// Устанавливаем тип объекта
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Игнорируем все каналы
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Visibility,
	                                              ECR_Block);
	// Разрешаем пересечение с каналом видимости

	ProceduralMesh->bUseAsyncCooking = true;
	// Включаем асинхронное создание коллизий
	ProceduralMesh->SetGenerateOverlapEvents(true);
	// Включаем события перекрытия
	ProceduralMesh->bSelectable = true; // Делаем компонент интерактивным
}

void AProceduralBlockMeshActor::BeginPlay()
{
	Super::BeginPlay();

	ProceduralMesh->OnClicked.AddDynamic(
		this, &AProceduralBlockMeshActor::OnMeshClicked);
	ProceduralMesh->OnBeginCursorOver.AddDynamic(
		this, &AProceduralBlockMeshActor::OnMouseOver);
	ProceduralMesh->OnEndCursorOver.AddDynamic(
		this, &AProceduralBlockMeshActor::OnMouseOut);
}

void AProceduralBlockMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProceduralBlockMeshActor::OnMeshClicked(
	UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (object.IsValid() && !object->is_selected())
	{
		object->select();
		// selected_object->Add(object);
		unsigned int object_id = object->get_id();
		selected_object->Add(object_id);
		UE_LOG(LogTemp, Warning, TEXT("mesh selected %p, %i"), object.Get(), object_id)
		// TouchedComponent->SetMaterial(0, DefaultMaterial);
	}
	else if (object.IsValid() && object->is_selected())
	{
		object->unselect();
		selected_object->RemoveAll([this]( unsigned int obj)
		{
			unsigned int object_id = object->get_id();
			return obj == object_id;
		});
		UE_LOG(LogTemp, Warning, TEXT("mesh unselected"))
	}
}

void AProceduralBlockMeshActor::OnMouseOver(UPrimitiveComponent* Component)
{
	object->hover();
}

void AProceduralBlockMeshActor::OnMouseOut(UPrimitiveComponent* Component)
{
	object->unhover();
}
