// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestingAIGameMode.h"
#include "TestingAICharacter.h"
#include "UObject/ConstructorHelpers.h"

ATestingAIGameMode::ATestingAIGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
