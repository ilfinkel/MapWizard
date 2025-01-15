
#include "ProceduralObjectMeshActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"


AProceduralBlockMeshActor::AProceduralBlockMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponentName = TEXT("ProceduralMesh");

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(*MeshComponentName);
	RootComponent = ProceduralMesh;
	// ProceduralMesh->SetupAttachment(RootComponent);
	// ProceduralMesh->RegisterComponent();
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

	ProceduralMesh->SetMaterial(NULL, DefaultMaterial);
	// Привязка события клика
	ProceduralMesh->OnClicked.AddDynamic(this, &AProceduralBlockMeshActor::OnMeshClicked);
	ProceduralMesh->OnBeginCursorOver.AddDynamic(this, &AProceduralBlockMeshActor::OnMouseOver);
	ProceduralMesh->OnEndCursorOver.AddDynamic(this, &AProceduralBlockMeshActor::OnMouseOut);
}

void AProceduralBlockMeshActor::OnMeshClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (ButtonPressed == EKeys::LeftMouseButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("Vertices for %s:"), *GetName());
		for (const FVector& Vertex : Vertices)
		{
			UE_LOG(LogTemp, Warning, TEXT("Vertex: %s"), *Vertex.ToString());
		}
	}
}
void AProceduralBlockMeshActor::OnMouseOver(UPrimitiveComponent* Component)
{
	if (Material)
	{
		Component->SetMaterial(0, DefaultMaterial);
	}
}
void AProceduralBlockMeshActor::OnMouseOut(UPrimitiveComponent* Component)
{
	if (Component && Material)
	{
		Component->SetMaterial(0, Material);
	}
}
