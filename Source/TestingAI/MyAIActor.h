// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "MyAIActor.generated.h"

UCLASS()
class TESTINGAI_API AMyAIActor : public APawn
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyAIActor();

	// Pointer to the AI controller component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	AAIController* AIController;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
