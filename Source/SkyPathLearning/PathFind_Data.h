// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PathFind_Data.generated.h"

class USVO_SaveGame;
/**
 * 
 */
UCLASS()
class SKYPATHLEARNING_API UPathFind_Data : public USaveGame
{
	GENERATED_BODY()
	
public:
	

	UFUNCTION()
		void InitializeData(USVO_SaveGame *_svo);

	UFUNCTION()
		bool AddSearchNodes(int32 mortonCode);

	UFUNCTION()
		TArray<int32>& GetSearchNodes();

	UFUNCTION()
		USVO_SaveGame* GetSVO();

private:
	UPROPERTY()
		TArray<int32>searchNodes;
	
	UPROPERTY()
		USVO_SaveGame* svo;
};
