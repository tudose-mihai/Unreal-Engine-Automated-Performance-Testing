// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAIActor.h"

// Sets default values
AMyAIActor::AMyAIActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyAIActor::BeginPlay()
{
	Super::BeginPlay();
	
	UWorld* World = GetWorld();

	if (World) {
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.Instigator = GetInstigator();
		FVector SpawnLocation = GetActorLocation();
		FRotator SpawnRotation = GetActorRotation();

		AIController = World->SpawnActor<AAIController>(AAIController::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);

		if (AIController) {
			AIController->Possess(this);
		}

	}
}

// Called every frame
void AMyAIActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

