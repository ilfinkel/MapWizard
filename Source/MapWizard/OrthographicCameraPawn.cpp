#include "OrthographicCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AOrthographicCameraPawn::AOrthographicCameraPawn()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	RootComponent = CameraComponent;

	// Настраиваем камеру в ортографическом режиме
	CameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;

	// // Устанавливаем ширину ортографической камеры (подбирается под ваш проект)
	// CameraComponent->OrthoWidth = 4000.0f;
	//
	// // Дополнительные параметры камеры (по желанию)
	// CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f)); // Расположим камеру выше сцены
}
void AOrthographicCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	// CameraComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // Смотрим вниз

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		EnableInput(PlayerController);
	}
}

void AOrthographicCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Обновляем положение камеры
	if (!MovementInput.IsZero())
	{
		FVector NewLocation = GetActorLocation() + MovementInput * CameraSpeed * DeltaTime;
		SetActorLocation(NewLocation);
	}
	// Обновляем масштаб ортографической камеры
	if (!FMath::IsNearlyZero(ZoomInput))
	{
		float NewOrthoWidth = FMath::Clamp(CameraComponent->OrthoWidth - ZoomInput * ZoomSpeed * DeltaTime, MinOrthoWidth, MaxOrthoWidth);
		CameraComponent->OrthoWidth = NewOrthoWidth;
	}
}

void AOrthographicCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Привязываем оси ввода
	PlayerInputComponent->BindAxis("MoveForward", this, &AOrthographicCameraPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AOrthographicCameraPawn::MoveRight);
	PlayerInputComponent->BindAxis("Zoom", this, &AOrthographicCameraPawn::Zoom);
}

// Управление перемещением
void AOrthographicCameraPawn::MoveForward(float Value)
{
	MovementInput.X = Value;
}

void AOrthographicCameraPawn::MoveRight(float Value)
{
	MovementInput.Y = Value;
}

// Управление зумом
void AOrthographicCameraPawn::Zoom(float Value)
{
	ZoomInput = Value;
}
