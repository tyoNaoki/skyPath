// Fill out your copyright notice in the Description page of Project Settings.


#include "SVOSystemLibrary.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/KismetSystemLibrary.h"

bool USVOSystemLibrary::BoxOverlapActors(UObject* WorldContextObject, const FVector BoxPos, FVector BoxExtent, FRotator BoxRotation, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, UClass* ActorClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<AActor*>& OutActors) {
	OutActors.Empty();

	TArray<UPrimitiveComponent*> OverlapComponents;
	bool bOverlapped = BoxOverlapComponents(WorldContextObject, BoxPos, BoxExtent, BoxRotation, ObjectTypes, NULL, ActorsToIgnore, OverlapComponents);
	if (bOverlapped) {
		UKismetSystemLibrary::GetActorListFromComponentList(OverlapComponents, ActorClassFilter, OutActors);
	}

	return (OutActors.Num() > 0);
}

bool USVOSystemLibrary::BoxOverlapComponents(UObject* WorldContextObject, const FVector BoxPos, FVector BoxExtent, FRotator BoxRotation, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, UClass* ComponentClassFilter, const TArray<AActor*>& ActorsToIgnore, TArray<UPrimitiveComponent*>& OutComponents) {
	OutComponents.Empty();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BoxOverlapComponents), false);
	Params.AddIgnoredActors(ActorsToIgnore);

	TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjectParams;
	for (auto Iter = ObjectTypes.CreateConstIterator(); Iter; ++Iter) {
		const ECollisionChannel& Channel = UCollisionProfile::Get()->ConvertToCollisionChannel(false, *Iter);
		ObjectParams.AddObjectTypesToQuery(Channel);
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World != nullptr) {
		World->OverlapMultiByObjectType(Overlaps, BoxPos, BoxRotation.Quaternion(), ObjectParams, FCollisionShape::MakeBox(BoxExtent), Params);
	}


	for (int32 OverlapIdx = 0; OverlapIdx < Overlaps.Num(); ++OverlapIdx) {
		FOverlapResult const& O = Overlaps[OverlapIdx];
		if (O.Component.IsValid()) {
			if (!ComponentClassFilter || O.Component.Get()->IsA(ComponentClassFilter)) {
				OutComponents.Add(O.Component.Get());
			}
		}
	}

	return (OutComponents.Num() > 0);
}