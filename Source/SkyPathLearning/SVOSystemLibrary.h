// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SVOSystemLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SKYPATHLEARNING_API USVOSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//��Q���o�^(���I)
	static bool BoxOverlapActors(
		UObject* WorldContextObject,
		const FVector BoxPos,
		FVector BoxExtent,
		FRotator BoxRotation,
		const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes,
		UClass* ActorClassFilter,
		const TArray<AActor*>& ActorsToIgnore,
		TArray<AActor*>& OutActors
	);

	//��Q���o�^(�ÓI)
	static bool BoxOverlapComponents(
		UObject* WorldContextObject,
		const FVector BoxPos,
		FVector BoxExtent,
		FRotator BoxRotation,
		const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes,
		UClass* ComponentClassFilter,
		const TArray<AActor*>& ActorsToIgnore,
		TArray<UPrimitiveComponent*>& OutComponents
	);

};
