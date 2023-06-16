// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PerformanceTrackingBpLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TESTINGAI_API UPerformanceTrackingBpLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static FString testPrint(UWorld* World, FString LevelName);

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static TMap<FString, float> GetPerformanceMetrics();

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static bool UpdatePerformancePointsMap();

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static FString startChart(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static FString stopChart(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static bool SaveTestResults(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static FString SaveLevelResults(UWorld* World, FString LevelName);

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static float CalculateScore();

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static FString HandleEndOfLevel(UWorld* World, FString CurrentLevel);

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static int32 HandleStartOfLevel(UWorld* World, FString CurrentLevel, UClass* BP_NPC);

	UFUNCTION(BlueprintCallable, Category = "ConsoleFunction")
	static FString getConsoleVariableValue(FString variableName);

	UFUNCTION(BlueprintCallable, Category = "ConsoleFunction")
	static int32 ApplyTestValues(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "ConsoleFunction")
	static bool GenerateTestValues();

	UFUNCTION(BlueprintCallable, Category = "TestingBPFunction")
	static int32 GetNumberOfAI();

};

TArray<TArray<FString>> generateCombinations(TArray<FString> array, TMap<FString, TArray<FString>> settingsMap, TArray<FString> keys);

TSharedPtr<FJsonObject> ReadJsonFile(FString FilePath);

TSharedPtr<FJsonObject> GetLevelResultsJson(TArray<FString> settings, TArray<FString> performance, float settingsScore, float performanceScore);


FBox GetNavMeshBoundsVolumeBox(UWorld* World);


bool WriteJsonFile(FString FilePath, TSharedPtr<FJsonObject> JsonObject);

bool ReadTxtFileToArray(const FString& FilePath, TArray<FString>& OutArray);

TArray<FString> FilterArrayByColon(const TArray<FString>& InArray);

TArray<FString> FilterArrayByKey(TArray<FString> FileLines, TArray<int32> keys);

TPair<FString, float> GetPairFromLine(FString input);

FString GetLastCreatedFolder(FString filepath);

FString GetFileName(FString Ffilepath, FString extension);

float extractFPS(FString input);

float GetUsedPhysicalMemory();
float GetUsedVirtualMemory();

