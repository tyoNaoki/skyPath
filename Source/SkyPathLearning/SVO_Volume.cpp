// Fill out your copyright notice in the Description page of Project Settings.


#include "SVO_Volume.h"

#include "Kismet/KismetMathLibrary.h"

#include "Components/CapsuleComponent.h"
#include "Components/BillboardComponent.h"

#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"

#include "SVO_SaveGame.h"

ASVO_Volume::ASVO_Volume() {
	billBoard = CreateDefaultSubobject<UBillboardComponent>(TEXT("billboard"));
	billBoard->SetupAttachment(RootComponent);

	if (!saveData) { saveData = Cast<USVO_SaveGame>(UGameplayStatics::CreateSaveGameObject(USVO_SaveGame::StaticClass())); }
}

void ASVO_Volume::BeginPlay() {
	Super::BeginPlay();

}

void ASVO_Volume::Generate() {

	if (!LoadData()){
		saveData = Cast<USVO_SaveGame>(UGameplayStatics::CreateSaveGameObject(USVO_SaveGame::StaticClass()));
	}

	if (saveData->GenerateFrameworkOfSVO(this, this->generateLevel)) {
		UE_LOG(LogTemp, Log, TEXT("%s generate FrameworkOfSVO succeced!!"), *saveName);
	}else {
		UE_LOG(LogTemp, Log, TEXT("%s generate FrameworkOfSVO failed!!"), *saveName);
		return;
	}

	TArray<AActor*> IgnoreActors = { this };

	if (saveData->RegistAllObstacles(this,traceType, IgnoreActors)) {
		UE_LOG(LogTemp, Log, TEXT("%s regist all obstacles succeced!!"), *saveName);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("%s regist all obstacles failed!!"), *saveName);
		return;
	}

	if (saveData->GenerateSparseVoxelOctree()) {
		UE_LOG(LogTemp, Log, TEXT("%s generate SparseVoxelOctree succesed!!"), *saveName);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("%s generate SparseVoxelOctree Failed!!"), *saveName);
		return;
	}

	if(!UGameplayStatics::SaveGameToSlot(saveData,*saveName,0)){
		UE_LOG(LogTemp, Error, TEXT("%s save faild."), *saveName);
		return;
	};

	UE_LOG(LogTemp, Log, TEXT("%s All generate succeced!!"), *saveName);
}

void ASVO_Volume::RegistObstacle(const FVector& obstacleLocation, const FVector& obstacleScale) {
	if(!LoadData()){
		return;
	}

	FVector tempMin = obstacleLocation - obstacleScale;
	FVector tempMax = obstacleLocation + obstacleScale;

	int32 tempRegistMortonCode = saveData->GetMortonNumber(tempMin, tempMax);

	saveData->RegistObstacleFromNumber(tempRegistMortonCode);
}

void ASVO_Volume::DrawAllObstacles() {
	if(!LoadData()){
		return;
	}
	saveData->DrawObstacleVoxels(FColor::Red, 1.0f, 20.0f);
}

bool ASVO_Volume::LoadData() {
	if (saveName == "None") {
		UE_LOG(LogTemp, Error, TEXT("saveName is None!! can't Save"));
		return false;
	}

	if (UGameplayStatics::DoesSaveGameExist(saveName, 0)) {
		saveData = Cast<USVO_SaveGame>(UGameplayStatics::LoadGameFromSlot(saveName, 0));
		if (saveData) {
			UE_LOG(LogTemp, Log, TEXT("%s Load succeced!!"), *saveName);
			return true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("%s Load Faild!!"), *saveName);
	return false;
}

void ASVO_Volume::DeleteData()
{
	if (UGameplayStatics::DeleteGameInSlot(saveName, 0)) {
		UE_LOG(LogTemp, Log, TEXT("%s Delete succeced!!"), *saveName);
	}else {
		UE_LOG(LogTemp, Log, TEXT("%s Delete failed!!"), *saveName);
	}
}

void ASVO_Volume::GetActorBoundingBox(AActor* BoundingTarget, FVector& targetCenter, FVector& targetSide) {
	UCapsuleComponent* TargetComp;
	TArray<UCapsuleComponent* >comps;
	BoundingTarget->GetComponents(comps);
	TSubclassOf<UCapsuleComponent>Caps;

	if (!comps[0]) {
		UE_LOG(LogTemp, Error, TEXT("%s : Not Founding CapsuleComponents"), *BoundingTarget->GetName());
	}

	TargetComp = comps[0];

	//if (Comp) {
	//	//UE_LOG(LogTemp, Error, TEXT("TestComps : %s"), *TestComp->GetName());
	//}else {
	//	UE_LOG(LogTemp, Error, TEXT("Comp : Null"));
	//}

	float Scale = 0.0f;

	UKismetSystemLibrary::GetComponentBounds(TargetComp, targetCenter, targetSide, Scale);
}

void ASVO_Volume::RegistObstacleOnActor(AActor* target) {

	if(!LoadData()){
		return;
	}
	FVector tempCenter = FVector();
	FVector tempSide = FVector();
	GetActorBoundingBox(target, tempCenter, tempSide);

	FVector registMin = tempCenter - tempSide;
	FVector registMax = tempCenter + tempSide;
	int32 registMortoncode = saveData->GetMortonNumber(registMin, registMax);

	UE_LOG(LogTemp, Error, TEXT("registMortoncode : %d"), registMortoncode);

	saveData->RegistObstacleFromNumber(registMortoncode);
}


