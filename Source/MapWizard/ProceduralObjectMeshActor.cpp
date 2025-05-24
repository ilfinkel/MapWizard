
#include "ProceduralObjectMeshActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"


AProceduralBlockMeshActor::AProceduralBlockMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponentName = TEXT("ProceduralMesh");

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(*MeshComponentName);
	RootComponent = ProceduralMesh;
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Включаем коллизии
	ProceduralMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic); // Устанавливаем тип объекта
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Ignore); // Игнорируем все каналы
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Visibility,
												  ECR_Block); // Разрешаем пересечение с каналом видимости

	ProceduralMesh->bUseAsyncCooking = true; // Включаем асинхронное создание коллизий
	ProceduralMesh->SetGenerateOverlapEvents(true); // Включаем события перекрытия
	ProceduralMesh->bSelectable = true; // Делаем компонент интерактивным
}

void AProceduralBlockMeshActor::BeginPlay()
{
	Super::BeginPlay();

	ProceduralMesh->OnClicked.AddDynamic(this, &AProceduralBlockMeshActor::OnMeshClicked);
	ProceduralMesh->OnBeginCursorOver.AddDynamic(this, &AProceduralBlockMeshActor::OnMouseOver);
	ProceduralMesh->OnEndCursorOver.AddDynamic(this, &AProceduralBlockMeshActor::OnMouseOut);
}
void AProceduralBlockMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProceduralBlockMeshActor::OnMeshClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (object.IsValid() && !object->is_selected())
	{
		object->select();
		// selected_object->Add(object);
		UE_LOG(LogTemp, Warning, TEXT("mesh selected %p"), object.Get())
		// TouchedComponent->SetMaterial(0, DefaultMaterial);
	}
	else if (object.IsValid() && object->is_selected())
	{
		object->unselect();
		// selected_object->RemoveAll([this]( TSharedPtr<DynamicObject> obj)
		// {
		// 	return obj = object;
		// });
		UE_LOG(LogTemp, Warning, TEXT("mesh unselected"))
		// TouchedComponent->SetMaterial(0, Material);
	}
}
void AProceduralBlockMeshActor::OnMouseOver(UPrimitiveComponent* Component)
{
	// if (Material)
	// {
	// 	Component->SetMaterial(0, DefaultMaterial);
	// }
}
void AProceduralBlockMeshActor::OnMouseOut(UPrimitiveComponent* Component)
{
	// if (Component && Material && !(district.IsValid() && district->is_selected()))
	// {
	// 	Component->SetMaterial(0, Material);
	// }
}
