// Fill out your copyright notice in the Description page of Project Settings.


#include "OrthographicCameraActor.h"

// Sets default values
AOrthographicCameraActor::AOrthographicCameraActor()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	RootComponent = CameraComponent;

	// Настраиваем камеру в ортографическом режиме
	CameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;

	// Устанавливаем ширину ортографической камеры (подбирается под ваш проект)
	CameraComponent->OrthoWidth = 2048.0f;

	// Дополнительные параметры камеры (по желанию)
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f)); // Расположим камеру выше сцены
	CameraComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // Смотрим вниз
}
