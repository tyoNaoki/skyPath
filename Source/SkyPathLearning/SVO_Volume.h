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
	//コンストラクタ
	ASVO_Volume();

	// ゲームが開始したときに呼ばれる関数
	virtual void BeginPlay() override;

	//billboardコンポーネント
	UPROPERTY()
		UBillboardComponent* billBoard;

	//生成フラグ
	UPROPERTY()
		bool isGenerate = false;

	//生成するボクセルの分割レベル(1~6)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVO", meta = (DisplayName = "Generate Level", ClampMin = 0, ClampMax = 6))
		int generateLevel = 6;

	//SVOのセーブデータ名
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVO", meta = (DisplayName = "SaveDeta Name"))
		FString saveName = "None";

	//障害物を登録するための衝突判定タイプ
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SVO", meta = (DisplayName = "collision ToRegistObstacles"))
		TEnumAsByte<ETraceTypeQuery> traceType = ETraceTypeQuery::TraceTypeQuery1;

	// SVOのセーブデータ
	UPROPERTY()
		USVO_SaveGame* saveData;

	// 八分木(SVO)を生成する関数
	UFUNCTION(Category = "SVO", meta = (CallInEditor = "true"))
		void Generate();

	// 全ての障害物を描画する関数
	UFUNCTION(Category = "SVO", meta = (CallInEditor = "true"))
		void DrawAllObstacles();

	// SVOのセーブデータを読み込む関数
	UFUNCTION()
		bool LoadData();

	// SVOのセーブデータを削除する関数
	UFUNCTION(Category = "SVO", meta = (CallInEditor = "true"))
		void DeleteData();

	// 障害物を登録する関数
	UFUNCTION(BlueprintCallable, Category = Obstacle)
		void RegistObstacle(const FVector& obstacleLocation, const FVector& obstacleScale);

	// 特定のアクターを障害物登録する関数
	UFUNCTION(BlueprintCallable, Category = regist)
		void RegistObstacleOnActor(AActor* target);

	// アクターのバウンディングボックスを取得する関数
	UFUNCTION(BlueprintCallable, Category = GetActorBoundingBox)
		void GetActorBoundingBox(AActor* BoundingTarget, FVector& targetCenter, FVector& targetSide);
};
