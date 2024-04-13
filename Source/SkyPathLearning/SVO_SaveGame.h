// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SVO_SaveGame.generated.h"

class ASVO_Volume;

//ファンネルアルゴリズム//
//ボクセルの各面
UENUM(BlueprintType)
enum class EVoxelSurface : uint8 {
	VS_left     UMETA(DisplayName = "left"),
	VS_right    UMETA(DisplayName = "right"),
	VS_forward  UMETA(DisplayName = "forward"),
	VS_back     UMETA(DisplayName = "back"),
	VS_top      UMETA(DisplayName = "top"),
	VS_down     UMETA(DisplayName = "down"),
	VS_none     UMETA(DisplayName = "none"),
};

//未確定軸
UENUM(BlueprintType)
enum class ESkipAxis : uint8 {
	SA_xy    UMETA(DisplayName = "Skip_xy"),
	SA_xz    UMETA(DisplayName = "Skip_xz"),
	SA_yz    UMETA(DisplayName = "Skip_yz"),
	SA_none     UMETA(DisplayName = "Skip_none"),
};

//ファンネルアルゴリズム曲線データ
USTRUCT(BlueprintType)
struct FCellEdge
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = left)
		FVector2D left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = right)
		FVector2D right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = mortonCode)
		int32 mortonCode;

	FCellEdge(FVector2D left, FVector2D right, int32 mortonCode) {
		this->left = left;
		this->right = right;
		this->mortonCode = mortonCode;
	}

	FCellEdge() {
		this->left = FVector2D();
		this->right = FVector2D();
		this->mortonCode = -1;
	}
};

//ボクセルデータ
USTRUCT(BlueprintType)
struct FFaceInfo
{
	GENERATED_USTRUCT_BODY();

	FFaceInfo() {
		mortonCode = -1;
		center = FVector();
		xyLeft = FVector2D();
		xyRight = FVector2D();
		xzLeft = FVector2D();
		xzRight = FVector2D();
		yzLeft = FVector2D();
		yzRight = FVector2D();
	};

	FFaceInfo(int32 mortonCode, FVector center) {
		this->mortonCode = mortonCode;
		this->center = center;
		xyLeft = FVector2D(center.X, center.Y);
		xyRight = FVector2D(center.X, center.Y);
		xzLeft = FVector2D(center.X, center.Z);
		xzRight = FVector2D(center.X, center.Z);
		yzLeft = FVector2D(center.Y, center.Z);
		yzRight = FVector2D(center.Y, center.Z);
	}

	bool operator ==(FFaceInfo& faceInfo) {
		return this->mortonCode == faceInfo.mortonCode;
	}

	bool operator !=(FFaceInfo& faceInfo) {
		return this->mortonCode != faceInfo.mortonCode;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = mortonCode)
		int32 mortonCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = center)
		FVector center;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = xyLeft)
		FVector2D xyLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = xyRight)
		FVector2D xyRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = xzLeft)
		FVector2D xzLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = xzRight)
		FVector2D xzRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = yzLeft)
		FVector2D yzLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = yzRight)
		FVector2D yzRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = skipAxis)
		ESkipAxis skipAxis = ESkipAxis::SA_none;
};

//隣接ノード
USTRUCT(BlueprintType)
struct FAdjacentNodes
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
		TArray<FFaceInfo>adjacentFaces;
};

/**
 * 
 */
UCLASS()
class SKYPATHLEARNING_API USVO_SaveGame : public USaveGame
{
	GENERATED_BODY()


	//生成フラグ
	UPROPERTY()
		bool isGenerate = false;

	//SVO環境生成フラグ
	UPROPERTY()
		bool isDoneGenerateFramework = false;

	//ボクセルの合計数
	UPROPERTY()
		int32 dwCellNum;

	//SVOの変数
	UPROPERTY()
		ASVO_Volume* _svo_Volume;

	// 最下位レベル
	UPROPERTY()
		unsigned int uiLevel;

	//　設定できる配列の大きさ(配列のための変数なのでここからー１)
	static const int MAX_INDEX = 7;

	//レベルごとの一個のボクセルの大きさ
	UPROPERTY()
		FVector units[MAX_INDEX + 1];

	//べき乗配列
	//f(x) = X_n-1 * 8
	//[1,8,64,512,,,,]
	UPROPERTY()
		int32 iPow[MAX_INDEX + 1];

	//指定したレベルまでの全てのVoxelを含めた合計数
	//f(x) = X_n-1 + iPow[X] 
	//[1,9,]
	UPROPERTY()
		int32 totalOfVoxels[MAX_INDEX + 1];

	//障害物
	UPROPERTY()
		TArray<int32>Obstacles;

	//探索ノード
	//UPROPERTY()
	//	TArray<int32>searchNodes;

	//voxelを64bitで収納
	UPROPERTY()
		TArray<uint64>bricks;

	//SVO
	//<空間番号(モートンコード),隣接した空間のリストを入れ子にした構造体>
	UPROPERTY()
		TMap<int32, FAdjacentNodes>svo_Nodes;

	

public:
	USVO_SaveGame();

	// 領域の幅
	UPROPERTY(BlueprintReadOnly, Category = svo)
		FVector unit_W;

	// 領域の最小値
	UPROPERTY(BlueprintReadOnly, Category = svo)
		FVector rgnMin;

	// 領域の最大値
	UPROPERTY(BlueprintReadOnly, Category = svo)
		FVector rgnMax;

private:
	//X軸に1加算
	int32 incMortonCodeX32(int32 code);

	//Y軸に1加算
	int32 incMortonCodeY32(int32 code);

	//Z軸に1加算
	int32 incMortonCodeZ32(int32 code);

	//X軸に1減算
	int32 decMortonCodeX32(int32 code);

	//Y軸に1減算
	int32 decMortonCodeY32(int32 code);

	//Z軸に1減算
	int32 decMortonCodeZ32(int32 code);

	//ノードを対象のノード分加算（ずらす）
	int32 addMortonCode32(int32 code, int32 add);

	//3bit毎に間隔を開ける関数
	int32 BitSeparateFor3D(int32 n);

	//八分木モートン順序算出関数
	int32 Get3DMortonNumber(int32 x, int32 y, int32 z);

	//点の座標からモートンコードを取得する関数
	int32 GetPointElem(FVector& p);

	//モートンコードから座標変換
	void fromMortonCode32(int32 code, float& xRet, float& yRet, float& zRet);

	//モートンコードからベクトル変換
	void fromMortonCode32(int32 code, FVector& TargetVector);

	//グローバルからローカルモートンコードに変換
	int32 GetLocalMortonCode(int32 globalMortonCode, int32 level);

	//ローカルからグローバルモートンコードに変換
	int32 GetGlovalMortonCode(int32 localMortonCode, int level);

public:
	
	//SVOの環境生成する関数
	bool GenerateFrameworkOfSVO(ASVO_Volume* svoVolume, int Level);

	//対象のモートンコードに対応するボクセルが障害物ではないかを判定する関数
	UFUNCTION()
		bool IsNotObstacle(int32 glovalMortonCode);

	//全ての障害物を登録する関数
	bool RegistAllObstacles(UObject* WorldContextObject, ETraceTypeQuery traceType, const TArray<AActor*>& ActorsToIgnore);

	//座標から障害物登録する関数
	void RegistObstacleFromLocation(FVector obstacleLocation, FVector obstacleScale);

	//モートンコードから障害物登録する関数
	bool RegistObstacleFromNumber(int32 glovalMortonCode);

	//対象のボクセルを描画する関数
	UFUNCTION()
		void DrawVoxelFromNumber(int glovalNumber, FColor color, float duration, float thickness);

	//障害物ボクセルを描画する関数
	UFUNCTION()
		void DrawObstacleVoxels(FColor color, float duration, float thickness);

	//ノードをすべて描画する関数
	UFUNCTION()
		void DrawNodes(FColor color, float duration, float thickness);

	//対象のノードに隣接したノードをすべて描画する関数
	UFUNCTION()
		void DrawAdjacentNodes(int32 mortonCode, FColor color, float duration, float thickness);

	//対象のノードの各面を描画する関数
	UFUNCTION()
		void DrawFaces(int32 mortonCode, float size, FColor color, float duration);

	//登録されている障害物を削除する関数
	UFUNCTION()
		void ClearObstacle();

	//SVO生成関数
	bool GenerateSparseVoxelOctree();

	//グローバルモートン番号のレベル取得する関数
	UFUNCTION()
		int GetLevel(int32 glovalMortonCode);

	//座標からグローバルモートンコードを取得関数
	UFUNCTION()
		int32 GetMortonNumber(FVector Min, FVector Max);

	//キャラクター専用のモートンコード取得関数
	UFUNCTION()
		int32 GetMortonNumberToCharacter(FVector center, FVector side);

	//一点からその座標を含むボクセルのモートン番号取得
	UFUNCTION()
		int32 GetMortonNumberFromPoint(FVector targetLocation);

	//隣接したボクセルのモートンコードを取得する関数
	//引数:SVOノード内のモートンコード
	UFUNCTION()
		FAdjacentNodes &GetAdjacentNodes(int32 mortonCode);

	//障害物無しの隣接したボクセルのモートンコード取得する関数
	//引数:障害物ありのモートンコード
	UFUNCTION()
		TArray<int32> GetNearEmptyVoxelNumbers(int32 glovalMortonCode);

	//グローバルモートンコードから座標取得
	UFUNCTION()
		FVector GetLocationFromMortonCode(int32 globalMortonCode, int level);

	//ローカルモートンコードに対応するボクセルデータ取得
	UFUNCTION()
		FVector GetExtentOfVoxel(int32 mortonCode, int level);

	//レベルからボクセル空間数取得
	UFUNCTION()
		int32 GetSizeOfVoxel(int differrenceOfLevel);

	//対象のモートンコードに対応するSVOノード内のモートンコードを取得する関数
	UFUNCTION()
		bool GetMortonCodeInNode(int32& MortonCode, int level);

	//子のモートン番号取得する関数
	UFUNCTION()
		int32 GetChildMortonNumber(int32 parentMortonCodeOfGloval);

	//親のモートン番号取得
	UFUNCTION()
		int32 GetParentMortonNumber(int32 childMortonCodeOfGloval);

private:
	//ブリックを生成する関数
	void GenerateBricks();

	//ノードを生成する関数
	void GenerateNodes();

	//再帰関数
	//障害物付近の空間を最低レベル(uiLevel)まで回して、隣接した空間を追加していく
	void AddSVOLinkNearObstacle(TArray<FFaceInfo>& faces,const EVoxelSurface & surface,int32 glovalMortonCode,unsigned int levelCount);

	//SVノード
	void registNotObstaclesInSVONodes(int32 glovalMortonCode, int levelCount, int& count);

	//ボクセルの分割レベルが同じ場合か隣接ボクセルのレベルが元より大きい場合
	FFaceInfo calcFaceInformation(const int32 glovalMortonCode,const int level, const EVoxelSurface& surface,const bool isChild = false);

	//隣接しているか判定する関数
	bool IsNeighbor(const EVoxelSurface& surface, int32 localMortonCode);

	//一旦レベル6の先頭モートン番号を割り出す関数
	int32 GetFirstMortonCodeOfMaxLevel(int32 glovalMortonCode,unsigned int currentLevel);

	//Bricksにモートンコードを登録する関数
	void RegistMortonCodeToBricks(int32 registGlovalMortonCode);

	//障害物が見つかるまで親を繰り上げる(最大L2まで(1~8))
//返り値はグローバルモートンコード
	int32 getIsNotObstacleCodeInMaxLevel(int32 glovalMortonCode);

	//対象のモートンコードは親のモートンコードに省略されているかどうか判定する関数
	bool IsSameOnMortonCode(int32 glovalMortonCode);
	
};
