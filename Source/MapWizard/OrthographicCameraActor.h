// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "OrthographicCameraActor.generated.h"

UCLASS()
class MAPWIZARD_API AOrthographicCameraActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AOrthographicCameraActor();

private:
	// Компонент камеры
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* CameraComponent;
};
