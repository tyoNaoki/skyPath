// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "SVO_Volume.generated.h"

class USVO_SaveGame;
class UBillboardComponent;

/**
 * 
 */
UCLASS()
class SKYPATHLEARNING_API ASVO_Volume : public AVolume
{
	GENERATED_BODY()
	
public:
	//�R���X�g���N�^
	ASVO_Volume();

	// �Q�[�����J�n�����Ƃ��ɌĂ΂��֐�
	virtual void BeginPlay() override;

	//billboard�R���|�[�l���g
	UPROPERTY()
		UBillboardComponent* billBoard;

	//�����t���O
	UPROPERTY()
		bool isGenerate = false;

	//��������{�N�Z���̕������x��(1~6)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVO", meta = (DisplayName = "Generate Level", ClampMin = 0, ClampMax = 6))
		int generateLevel = 6;

	//SVO�̃Z�[�u�f�[�^��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVO", meta = (DisplayName = "SaveDeta Name"))
		FString saveName = "None";

	//��Q����o�^���邽�߂̏Փ˔���^�C�v
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVO", meta = (DisplayName = "collision ToRegistObstacles"))
		TEnumAsByte<ETraceTypeQuery> traceType = ETraceTypeQuery::TraceTypeQuery1;

	// SVO�̃Z�[�u�f�[�^
	UPROPERTY()
		USVO_SaveGame* saveData;

	// ������(SVO)�𐶐�����֐�
	UFUNCTION(Category = "SVO", meta = (CallInEditor = "true"))
		void Generate();

	// �S�Ă̏�Q����`�悷��֐�
	UFUNCTION(Category = "SVO", meta = (CallInEditor = "true"))
		void DrawAllObstacles();

	// SVO�̃Z�[�u�f�[�^��ǂݍ��ފ֐�
	UFUNCTION()
		bool LoadData();

	// SVO�̃Z�[�u�f�[�^���폜����֐�
	UFUNCTION(Category = "SVO", meta = (CallInEditor = "true"))
		void DeleteData();

	// ��Q����o�^����֐�
	UFUNCTION(BlueprintCallable, Category = Obstacle)
		void RegistObstacle(const FVector& obstacleLocation, const FVector& obstacleScale);

	// ����̃A�N�^�[����Q���o�^����֐�
	UFUNCTION(BlueprintCallable, Category = regist)
		void RegistObstacleOnActor(AActor* target);

	// �A�N�^�[�̃o�E���f�B���O�{�b�N�X���擾����֐�
	UFUNCTION(BlueprintCallable, Category = GetActorBoundingBox)
		void GetActorBoundingBox(AActor* BoundingTarget, FVector& targetCenter, FVector& targetSide);
};
