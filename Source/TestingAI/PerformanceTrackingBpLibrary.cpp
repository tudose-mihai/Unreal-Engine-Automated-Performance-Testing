// Fill out your copyright notice in the Description page of Project Settings.


#include "PerformanceTrackingBpLibrary.h"
#include "GenericPlatform/GenericPlatformMemory.h" 
#include "ProfilingDebugging/CSVProfiler.h"


#include "Engine/Engine.h"
#include "Misc/FrameRate.h" 
#include "ProfilingDebugging/HealthSnapshot.h" 
#include "ChartCreation.h" 
#include <Json.h>
#include <JsonUtilities.h>
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Trace/Analysis.h"
#include "Stats/StatsFile.h"
#include "Misc/FileHelper.h"
#include <filesystem>
#include "MyAIActor.h"
#include "EngineUtils.h"
#include "NavMesh/NavMeshBoundsVolume.h"

FPerformanceTrackingChart perfChart = FPerformanceTrackingChart();
TMap < FString, double> settingsPointsMap;
TMap < FString, TArray < TPair < float, float>>> performancePointsMap;
FString currentTestName;
int32 currentSettingsScore = -2;
int32 numberOfAiActors = 0;
TArray<FString> currentValues;
TArray<AActor*> NPCs;
bool lastTest = false;
FString JsonFilepath = "C:/Users/Mihai/Documents/Unreal Projects/TestingAI/Content/JsonData/";



FBox GetNavMeshBoundsVolumeBox(UWorld* World)
{

	if (World)
	{
		// Iterate through all the actors in the world
		for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
		{
			ANavMeshBoundsVolume* NavMeshBoundsVolume = *It;
			if (NavMeshBoundsVolume)
			{
				FBox WorldBox = NavMeshBoundsVolume->GetComponentsBoundingBox(true);

				return WorldBox;
			}
		}
	}
	return FBox();
}



TSharedPtr<FJsonObject> CreateFJsonObject(const TMap<FString, TArray<FString>>& InputMap);

FString					UPerformanceTrackingBpLibrary::testPrint(UWorld* World, FString LevelName)
{
	
	return "lvlname: " +  LevelName;


}

FString					UPerformanceTrackingBpLibrary::HandleEndOfLevel(UWorld* World, FString CurrentLevel) {
	TSharedPtr<FJsonObject> SessionData = ReadJsonFile(JsonFilepath + "SessionData.json");
	TArray<TSharedPtr<FJsonValue>> levels = SessionData->GetArrayField("levels");
	FString finalLevel = levels[levels.Num() - 1]->AsString();
	FString nextLevel;
	if (finalLevel == CurrentLevel) {
		nextLevel = levels[0]->AsString();
	}
	else {
		for (int i = 0; i < levels.Num(); i++) {
			if (levels[i]->AsString() == CurrentLevel)
			{
				nextLevel = levels[i + 1]->AsString();
				break;
			}
		}
	}
	stopChart(World);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("" + CurrentLevel + " followed by: " + nextLevel));

	
	return nextLevel;

}

int32					UPerformanceTrackingBpLibrary::GetNumberOfAI()
{
	return numberOfAiActors;
}

FString					UPerformanceTrackingBpLibrary::SaveLevelResults(UWorld* World, FString LevelName) {
	TMap<FString, float> metricsMap = GetPerformanceMetrics();
	TArray<FString> metrics, keys;
	metricsMap.GetKeys(keys);
	UpdatePerformancePointsMap();
	for (FString key : keys)
	{
		// Check if the key is not in performancePointsMap
		if (!performancePointsMap.Contains(key))
		{
			// Remove the key and value from metricsMap
			metricsMap.Remove(key);
		}
	}
	metricsMap.GetKeys(keys);
	for (FString key : keys) {
		FString metric = key + " " + FString::Printf(TEXT("%.2f"), metricsMap[key]);
		metrics.Add(metric);
	}

	TSharedPtr<FJsonObject> testJson = ReadJsonFile(JsonFilepath + "CurrentTest.json");

	TSharedPtr<FJsonObject> jsonObject = GetLevelResultsJson(currentValues, metrics, currentSettingsScore, CalculateScore());
	TSharedPtr<FJsonObject> SessionData = ReadJsonFile(JsonFilepath + "SessionData.json");
	TArray<TSharedPtr<FJsonValue>> levels = SessionData->GetArrayField("levels");
	FString previousLevel;
	for (int i = 0; i < levels.Num(); i++) {
		if (levels[i]->AsString() == LevelName)
		{
			if (i == 0) {
				previousLevel = levels[levels.Num() - 1]->AsString();
			}
			else {
				previousLevel = levels[(i - 1) % levels.Num()]->AsString();
			}
			break;
		}
	}
	testJson->SetObjectField(previousLevel, jsonObject);

	WriteJsonFile(JsonFilepath + "CurrentTest.json", testJson);

	return "shouldnt see this";
}

bool					UPerformanceTrackingBpLibrary::SaveTestResults(UWorld* World) {
	TSharedPtr<FJsonObject> testJson		= ReadJsonFile(JsonFilepath + "CurrentTest.json");
	TSharedPtr<FJsonObject> allTestsJson	= ReadJsonFile(JsonFilepath + "Results.json");
	if (currentTestName.IsEmpty()) {
		return false;
	}
	allTestsJson->SetObjectField(currentTestName, testJson);
	WriteJsonFile(JsonFilepath + "Results.json", allTestsJson);
	testJson = MakeShareable(new FJsonObject());
	WriteJsonFile(JsonFilepath + "CurrentTest.json", testJson);

	return true;
}

int32					UPerformanceTrackingBpLibrary::HandleStartOfLevel(UWorld* World, FString CurrentLevel, UClass* BP_NPC) {
	TSharedPtr<FJsonObject> SessionData = ReadJsonFile(JsonFilepath + "SessionData.json");
	TArray<TSharedPtr<FJsonValue>> levels = SessionData->GetArrayField("levels");
	FString firstLevel = levels[0]->AsString();
	FString currentLevel = World->GetName();

	TSharedPtr<FJsonObject> GeneratedTests = ReadJsonFile(JsonFilepath + "GeneratedTests.json");
	TSharedPtr<FJsonObject> CurrentTest = ReadJsonFile(JsonFilepath + "CurrentTest.json");
	if (GeneratedTests->Values.Num() == 0 && firstLevel == currentLevel) {
		if (CurrentTest->Values.Num() > 0) {
			SaveLevelResults(World, currentLevel);
			SaveTestResults(World);
		}
		numberOfAiActors = 0;
			return 0;
	}

	startChart(World);
	if (currentSettingsScore == -2) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("First time applying values"));
		currentSettingsScore = ApplyTestValues(World);
		// should clear json files
		return 1;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SaveLevelResults"));
	SaveLevelResults(World, currentLevel);
	
	if (firstLevel == currentLevel) {
		
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ApplyTestValues " + CurrentLevel));
		if (CurrentTest->Values.Num() > 0) {
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("SaveTestResult"));
			SaveTestResults(World);
		}
		currentSettingsScore = ApplyTestValues(World);

		return 2;
	}



	return 3;
}

bool					UPerformanceTrackingBpLibrary::UpdatePerformancePointsMap() {
	TSharedPtr<FJsonObject> JsonObject = ReadJsonFile(JsonFilepath + "InputPerformance.json");
	TMap<FString, TSharedPtr<FJsonValue>> map = JsonObject->Values;
	TArray<FString> keys;
	map.GetKeys(keys);


	for (FString key : keys) {
		TArray<TSharedPtr<FJsonValue>> valuesAndPoints = JsonObject->GetArrayField(key);
		TArray<TSharedPtr<FJsonValue>> values = valuesAndPoints[0]->AsArray();
		TArray<TSharedPtr<FJsonValue>> points = valuesAndPoints[1]->AsArray();
		TArray<TPair<float, float>> infoArray;
		for (int i = 0; i < values.Num(); i++) {
			TPair<float, float> infoPair;
			infoPair.Key = values[i]->AsNumber();
			infoPair.Value = points[i]->AsNumber();
			
			infoArray.Add(infoPair);
		}
		performancePointsMap.Add(key, infoArray);
	}
	performancePointsMap.GetKeys(keys);

	return true;
}

bool					UPerformanceTrackingBpLibrary::GenerateTestValues() {
	TSharedPtr<FJsonObject> JsonObject = ReadJsonFile(JsonFilepath + "InputSettings.json");
	TMap<FString, TSharedPtr<FJsonValue>> map = JsonObject->Values;
	TArray<FString> keys;
	map.GetKeys(keys);

	TMap<FString, TArray<FString>> settingsValuesMap;

	for (FString key : keys) {
		TArray<FString> valueArray;
		TArray<TSharedPtr<FJsonValue>> valuesAndPoints = JsonObject->GetArrayField(key);
		TArray<TSharedPtr<FJsonValue>> values = valuesAndPoints[0]->AsArray();
		TArray<TSharedPtr<FJsonValue>> points = valuesAndPoints[1]->AsArray();
		for (int i = 0; i < values.Num(); i++) {
			FString name = values[i]->AsString();
			double score = points[i]->AsNumber();
			valueArray.Add(name);
			settingsPointsMap.Add(FString(key + " " + name), score);
		}
		settingsValuesMap.Add(key, valueArray);
	}
	TArray<FString> array;
	settingsValuesMap.GetKeys(keys);
	TArray<TArray<FString>> result = generateCombinations(array, settingsValuesMap, keys);

	TMap<FString, TArray<FString>> InputMap;
	int32 testIndex = 0;
	for (const TArray<FString>& a : result) {
		InputMap.Add("test" + FString::FromInt(testIndex), a);
		testIndex++;
	}
	TSharedPtr<FJsonObject> testsJsonObject = CreateFJsonObject(InputMap);
	WriteJsonFile(JsonFilepath + "GeneratedTests.json", testsJsonObject);

	return true;
}

TMap<FString, float>	UPerformanceTrackingBpLibrary::GetPerformanceMetrics() {
	FString FPSChartsFilePath = "C:\\Users\\Mihai\\Documents\\Unreal Projects\\TestingAI\\Saved\\Profiling\\FPSChartStats";
	FString LatestChartFilePath = GetLastCreatedFolder(FPSChartsFilePath);
	FString TxtFilePath = GetFileName(LatestChartFilePath, ".log");

	TArray<FString> FileContentByLines, Metrics;
	ReadTxtFileToArray(TxtFilePath, FileContentByLines);
	TArray<int32> keys = { 21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,51,52,53,54,55,56,57,59,60,61,62,63,64 };
	Metrics = FilterArrayByKey(FileContentByLines, keys);
	
	
	TMap<FString, float> MetricsMap;

	MetricsMap.Add("FPS average", extractFPS(FileContentByLines[20]));

	for (FString metric : Metrics)
		MetricsMap.Add(GetPairFromLine(metric));

	MetricsMap.Add("Used Physical memory",	GetUsedPhysicalMemory());
	MetricsMap.Add("Used Virtual memory",	GetUsedVirtualMemory());

	return MetricsMap;
}

FString					UPerformanceTrackingBpLibrary::startChart(UWorld* World)
{
	//UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("startfpschart"));
	if (GEngine)
		GEngine->StartFPSChart("Testing testing", false);
	return FString();
}

FString					UPerformanceTrackingBpLibrary::stopChart(UWorld* World)
{
	UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("t.FPSChart.OpenFolderOnDump 0"));
	UKismetSystemLibrary::ExecuteConsoleCommand(World, TEXT("stopfpschart -NoOpen"));
	//if (GEngine)
	//	GEngine->StopFPSChart("testlevel");
	return FString();

}

FString					UPerformanceTrackingBpLibrary::getConsoleVariableValue(FString variableName)
{
	
	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(*variableName);
	FString Value = CVar->GetString();
	return Value;
}

int32					UPerformanceTrackingBpLibrary::ApplyTestValues(UWorld* World) {
	// update points map
	TSharedPtr<FJsonObject> settingsMapJsonObject = ReadJsonFile(JsonFilepath + "InputSettings.json");
	TMap<FString, TSharedPtr<FJsonValue>> settingsMapmap = settingsMapJsonObject->Values;
	TArray<FString> settingsMapkeys;
	settingsMapmap.GetKeys(settingsMapkeys);

	TMap<FString, TArray<FString>> settingsValuesMap;

	for (FString key : settingsMapkeys) {
		TArray<FString> valueArray;
		TArray<TSharedPtr<FJsonValue>> valuesAndPoints = settingsMapJsonObject->GetArrayField(key);
		TArray<TSharedPtr<FJsonValue>> values = valuesAndPoints[0]->AsArray();
		TArray<TSharedPtr<FJsonValue>> points = valuesAndPoints[1]->AsArray();
		for (int i = 0; i < values.Num(); i++) {
			FString name = values[i]->AsString();
			double score = points[i]->AsNumber();
			valueArray.Add(name);
			settingsPointsMap.Add(FString(key + " " + name), score);
		}
		settingsValuesMap.Add(key, valueArray);
	}


	//

	TSharedPtr<FJsonObject> JsonObject = ReadJsonFile(JsonFilepath + "GeneratedTests.json");
	TMap<FString, TSharedPtr<FJsonValue>> map = JsonObject->Values;
	TArray<FString> keys;
	int32 finalScore = 0;
	map.GetKeys(keys);
	currentValues.Empty();
	TArray<TSharedPtr<FJsonValue>> values;
	if (keys.Num() > 0) {
		currentTestName = keys[0];
		values = JsonObject->TryGetField(keys[0])->AsArray();
		JsonObject->RemoveField(keys[0]);
		if (keys.Num() == 0) {
			lastTest = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("No tests in GeneratedTests.json"));
		return -1;
	}

	for (int i = 0; i < values.Num(); i++) {
		FString name = values[i]->AsString();
		currentValues.Add(name);

		TArray<FString> parts;
		name.ParseIntoArray(parts, TEXT(" "), true);
		if (parts.Num() == 2 && FName(*parts[0]) == "numberOfAiActors") {
			numberOfAiActors = FCString::Atoi(*parts[1]);
		}
		else {
			UKismetSystemLibrary::ExecuteConsoleCommand(World, name);
		}
		if(settingsPointsMap.Contains(name))
			finalScore += settingsPointsMap[name];

	}

	WriteJsonFile(JsonFilepath + "GeneratedTests.json", JsonObject);


	return finalScore;
}

float					UPerformanceTrackingBpLibrary::CalculateScore()
{
	TMap<FString, float> metricsMap;
	metricsMap = GetPerformanceMetrics();
	UpdatePerformancePointsMap();


	float totalScore = 0.0f;
	TArray<FString> keys;

	performancePointsMap.GetKeys(keys);
	for (auto& key : keys)
	{		
		if (metricsMap.Contains(key))
		{			
			float metricValue = metricsMap[key];
			TArray<TPair<float, float>> valueArray = performancePointsMap[key];
			for (int i = 0; i < valueArray.Num(); i++)
			{
				TPair<float, float> pair = valueArray[i];
				if (metricValue < pair.Key)
					totalScore += pair.Value;
			}
		}

	}

	return totalScore;
}
	
float					GetUsedPhysicalMemory()
{
	FWindowsPlatformMemory memoryStats = FWindowsPlatformMemory();
	FPlatformMemoryStats MemoryStats = memoryStats.GetStats();
	uint64 usedPhysicalMemory = MemoryStats.UsedPhysical / 1024 / 1024;

	return usedPhysicalMemory;
}

float					GetUsedVirtualMemory()
{
	FWindowsPlatformMemory memoryStats = FWindowsPlatformMemory();
	FPlatformMemoryStats MemoryStats = memoryStats.GetStats();
	uint64 usedVirtual = MemoryStats.UsedVirtual / 1024 / 1024;

	return usedVirtual;
}

float					extractFPS(FString input) {
	float result;
	int32 index = input.Find(TEXT(" FPS"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	result = FCString::Atof(*input.Mid(index - 5, 5));


	return result;
}

TPair<FString,float>	GetPairFromLine(FString input) {
	TPair<FString, float> infoPair;
	FString left, right;
	input.Split(TEXT(":"), &left, &right);
	left = left.TrimStartAndEnd();
	right = right.TrimStartAndEnd();
	// Only interested in percentages, marked in parantheses
	int index = right.Find(TEXT("("));
	if (index != -1) {
		right = right.RightChop(index+1);
	}
	float number = FCString::Atof(*right);
	infoPair.Key = left;
	infoPair.Value = number;
	return infoPair;
}

TSharedPtr<FJsonObject> CreateFJsonObject(const TMap<FString, TArray<FString>>& InputMap)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	for (const auto& Pair : InputMap) {
		FString Key = Pair.Key;
		TArray<FString> Value = Pair.Value;

		TArray<TSharedPtr<FJsonValue>> JsonValueArray;
		for (const FString& String : Value)
		{
			JsonValueArray.Add(MakeShareable(new FJsonValueString(String)));
		}
		JsonObject->SetArrayField(Key, JsonValueArray);
	}
	return JsonObject;
}

TArray<TArray<FString>> generateCombinations(TArray<FString> array, TMap<FString, TArray<FString>> settingsMap, TArray<FString> keys) {
	TArray<TArray<FString>> result;

	if (keys.Num() == 0)
	{
		result.AddUnique(array);
		return result;
	}

	FString key = keys[0];
	keys.RemoveAt(0);

	if (settingsMap.Contains(key))
	{
		TArray<FString> valueArray = settingsMap[key];

		for (const FString& element : valueArray)
		{
			TArray<FString> newArray = array;
			FString settingValuePair = key + " " + element;
			newArray.AddUnique(settingValuePair);

			TArray<TArray<FString>> subResult = generateCombinations(newArray, settingsMap, keys);

			result.Append(subResult);
		}
	}

	return result;
}

TSharedPtr<FJsonObject> GetLevelResultsJson(TArray<FString> settings, TArray<FString> metrics, float settingsScore, float performanceScore) {
	TSharedPtr<FJsonObject> levelObject = MakeShareable(new FJsonObject());

	TArray<TSharedPtr<FJsonValue>> settingsJson;
	TArray<TSharedPtr<FJsonValue>> metricsJson;

	for (FString setting : settings)
	{
		TSharedPtr<FJsonValueString> settingJson = MakeShareable(new FJsonValueString(setting));
		settingsJson.Add(settingJson);
	}
	for (FString metric : metrics)
	{
		TSharedPtr<FJsonValueString> metricJson = MakeShareable(new FJsonValueString(metric));
		metricsJson.Add(metricJson);
	}


	levelObject->SetArrayField("settings", settingsJson);
	levelObject->SetNumberField("settingsScore", settingsScore);
	levelObject->SetArrayField("performance", metricsJson);
	levelObject->SetNumberField("performanceScore", performanceScore);

	return levelObject;
}

TSharedPtr<FJsonObject> GetTestResults() {
	TSharedPtr<FJsonObject> testObject = MakeShareable(new FJsonObject());
	return testObject;
}

TSharedPtr<FJsonObject> ReadJsonFile(FString FilePath) {
	TSharedPtr<FJsonObject> JsonObject = nullptr;
	FString FileContents;

	if (FFileHelper::LoadFileToString(FileContents, *FilePath)) {
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(FileContents);
		if (FJsonSerializer::Deserialize(JsonReader.Get(), JsonObject)) {

			return JsonObject;
		}
	}
	return JsonObject;

}

bool					WriteJsonFile(FString FilePath, TSharedPtr<FJsonObject> JsonObject) {
	FString FileContents;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&FileContents);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter)) {
		if (FFileHelper::SaveStringToFile(FileContents, *FilePath)) {
			return true;
		}
	}

	return false;
}

bool					ReadTxtFileToArray(const FString& FilePath, TArray<FString>& OutArray)
{
	// Clear the output array
	OutArray.Empty();

	// Check if the file exists
	if (FPaths::FileExists(FilePath))
	{
		// Read the file contents as a string
		FString FileContents;
		if (FFileHelper::LoadFileToString(FileContents, *FilePath))
		{
			// Split the string by newline characters and store the result in the output array
			FileContents.ParseIntoArrayLines(OutArray);
			return true;
		}
	}

	// Return false if the file does not exist or cannot be read
	return false;
}

TArray<FString>			FilterArrayByColon(const TArray<FString>& InArray)
{
	// Create an empty output array
	TArray<FString> OutArray;

	// Loop through the input array
	for (const FString& Element : InArray)
	{
		// Check if the element contains a ":" character
		if (Element.Contains(TEXT(":")))
		{
			// Add the element to the output array
			OutArray.Add(Element);
		}
	}

	// Return the output array
	return OutArray;
}

TArray<FString>			FilterArrayByKey(TArray<FString> FileLines, TArray<int32> keys) {
	TArray<FString> result;
	for (int i = 0; i < keys.Num(); i++) {
		int index = keys[i];
		if (index >= 0 && index < FileLines.Num()) {
			FString line = FileLines[index];
			result.Add(line);
		}
	}
	// return the result array
	return result;
}

FString					GetLastCreatedFolder(FString filepath) {
	std::string utf8String = TCHAR_TO_UTF8(*filepath);
	std::filesystem::path filePath = std::filesystem::u8path(utf8String);
	std::filesystem::path folderPath = filePath;
	std::filesystem::path lastFolderAdded;
	std::filesystem::file_time_type latestTime;

	// Iterate over the subdirectories of folderPath
	for (auto it = std::filesystem::directory_iterator(folderPath); it != std::filesystem::directory_iterator(); ++it)
	{
		if (it->is_directory())
		{
			auto currentTime = it->last_write_time();

			if (currentTime > latestTime)
			{
				latestTime = currentTime;
				lastFolderAdded = it->path();
			}
		}
	}
	std::string pathStringUTF8 = lastFolderAdded.u8string();
	const char* lastFilepath = pathStringUTF8.c_str();
	return lastFilepath;
}

FString					GetFileName(FString Ffilepath, FString extension) {
	std::string utf8String = TCHAR_TO_UTF8(*Ffilepath);
	std::filesystem::path FfilePath = std::filesystem::u8path(utf8String);
	std::filesystem::path folderPath = FfilePath;
	std::filesystem::path lastFolderAdded;
	std::filesystem::path textFilePath;

	for (auto it = std::filesystem::directory_iterator(folderPath); it != std::filesystem::directory_iterator(); ++it)
	{
		if (it->is_regular_file())
		{
			auto filePath = it->path();
			if (filePath.extension() == TCHAR_TO_UTF8(*extension))
			{
				textFilePath = filePath;
				break;
			}
		}
	}

	std::string pathStringUTF8 = textFilePath.u8string();
	const char* txtFilePath = pathStringUTF8.c_str();
	return txtFilePath;
}


/*

UI/UX:
Menu:
	-Start/stop tests
	-Test generation
	-Control over final result: minimal (scores only), expanded (+ settings and performance metrics results)

CSV creation, CSVtoSVG tool

Display current test information:
	Level name         + Settings being tested	+ score
	Sequence of tests  + Estimated time left
	Sequence of levels + settingsScore			+ performanceScore

Better control of input:
	way of knowing all possible performance metrics
	way of knowing common graphical settings

More demos to stress different systems

Some way to leverage the Stats system
	-Stat StartFile/StopFile
	-CsvProfile Start/Stop

code readability
project readability



//

Administrativ:
	se stie perioada examenului dincolo de 26 iunie - 09 iulie?
	restanta din iarna ?


*/