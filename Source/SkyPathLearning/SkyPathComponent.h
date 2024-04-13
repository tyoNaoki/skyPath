// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SVO_SaveGame.h"

#include <complex>
#include <numeric>
#include <functional>
#include <iterator>

#include "SkyPathComponent.generated.h"

class ASVO_Volume;
class UPathFind_Data;

//A* algorithm
USTRUCT(BlueprintType)
struct FWaypointD
{
	GENERATED_USTRUCT_BODY();

public:
	int32 parentMortonCode;

	float Score;

	float MoveCost = -1;

	FFaceInfo faceInfo;

	FWaypointD() {
		faceInfo.mortonCode = -1;
		this->parentMortonCode = -1;
	}

	FWaypointD(int32 parentMortonCode, float Score, float MoveCost, FFaceInfo& faceInfo) {
		this->parentMortonCode = parentMortonCode;
		this->Score = Score;
		this->MoveCost = MoveCost;
		this->faceInfo = faceInfo;
	}

	bool operator!() {
		return faceInfo.mortonCode == -1;
	}

	bool operator == (const FWaypointD& data) {
		return faceInfo.mortonCode == data.faceInfo.mortonCode;
	}
};

//FunnelAlgorithm
USTRUCT(BlueprintType)
struct FFunnelAlgorithmAxis {
	GENERATED_USTRUCT_BODY();

public:
	bool isDecideX = false;

	bool isDecideY = false;

	bool isDecideZ = false;

	FVector location;

	FVector widthMax;

	FVector widthMin;

	FFunnelAlgorithmAxis(bool isDecideX, bool isDecideY, bool isDecideZ, FVector location, FVector widthMax, FVector widthMin) {
		this->isDecideX = isDecideX;
		this->isDecideY = isDecideY;
		this->isDecideZ = isDecideZ;
		this->location = location;
		this->widthMax = widthMax;
		this->widthMin = widthMin;
	}

	FFunnelAlgorithmAxis() {
		this->isDecideX = false;
		this->isDecideY = false;
		this->isDecideZ = false;
		this->location = FVector::ZeroVector;
		this->widthMax = FVector::ZeroVector;
		this->widthMin = FVector::ZeroVector;
	}
};


struct FFunnelAlgorithmMath {
	enum class EPointSide {
		LEFT_SIDE = 0,
		RIGHT_SIDE,
		ON_LINE,
	};

	FORCEINLINE static EPointSide ClassifyPoint(FVector2D& A, FVector2D& B, FVector2D& C) {
		FVector2D AB = B - A;
		FVector2D AC = C - A;

		float Value = FVector2D::CrossProduct(AB.GetSafeNormal(), AC.GetSafeNormal());
		static const float eq = 0.001f * 0.001f;

		if (abs(Value) < eq) return EPointSide::ON_LINE;
		if (Value < 0) return EPointSide::LEFT_SIDE;
		else return EPointSide::RIGHT_SIDE;
	}
};


UENUM(BlueprintType)
enum class EFunnelAxis : uint8 {
	FA_XY   UMETA(DisplayName = "XY"),
	FA_XZ     UMETA(DisplayName = "XZ"),
	FA_YZ  UMETA(DisplayName = "YZ"),
};

USTRUCT(BlueprintType)
struct FVoxelData
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = location)
		FVector location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = extent)
		FVector extent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = side)
		FFaceInfo faceInfo;

	FVoxelData() {
		location = FVector();
		extent = FVector();
		faceInfo = FFaceInfo();
	}

	FVoxelData(FVector locaiton, FVector extent, FFaceInfo& faceInfo) {
		this->location = locaiton;
		this->extent = extent;
		//this->mortonCode = mortonCode;
		this->faceInfo = faceInfo;
	}
};

//ClothoidCurve Algorithm
USTRUCT()
struct FSlope {
	GENERATED_USTRUCT_BODY()

	float phi0;
	float phiV;
	float phiU;

	//@0+@1s+@2ss
	float phi(float _phi0, float _phiV, float _phiU, float S) {
		return _phi0 + _phiV * S + _phiU * (S * S);
	}

	std::complex<float> slope_f(float _phi0, float _phiV, float _phiU, float S) {
		std::complex<float> j(0.0f, 1.0f);
		return std::exp(j * phi(_phi0, _phiV, _phiU, S));
	}

	std::complex<float> operator()(float S) {
		return slope_f(phi0, phiV, phiU, S);
	}
};

USTRUCT()
struct FPhiSlope {
	GENERATED_USTRUCT_BODY()

	float phiV;
	float phiU;

	std::complex<float> slope_f(float _phiV, float _phiU, float S) {
		std::complex<float> j(0.0f, 1.0f);
		return std::exp(j * (phiV * S + phiU * (S * S)));
	}

	std::complex<float> operator()(float S) {
		return slope_f(phiV, phiU, S);
	}
};

//円弧
USTRUCT(BlueprintType)
struct FCircle
{
	GENERATED_USTRUCT_BODY()

	FCircle(FVector _center, float _radius, FRotator _centerRotation, float _startAngle, float _endAngle, float _sign) :isPoint(false), center(_center), radius(_radius), centerRotation(_centerRotation), startAngle(_startAngle), endAngle(_endAngle), sign(_sign) {
	};

	FCircle(FVector point, FRotator _centerRotation, float _startAngle, float _endAngle, float _sign) :isPoint(true), center(point), radius(0), centerCircle(point), startCircle(point), endCircle(point), centerRotation(_centerRotation), startAngle(_startAngle), endAngle(_endAngle), sign(_sign) {
	};

	FCircle(FVector point,FRotator _centerRotation) :isPoint(true),center(point),radius(0),centerCircle(point),startCircle(point),endCircle(point),centerRotation(_centerRotation),startAngle(_centerRotation.Yaw),endAngle(_centerRotation.Yaw),sign(0.0f) {
	};

	FCircle() :isPoint(true) {
	};

	UPROPERTY()
		bool isPoint = false;

	UPROPERTY(BlueprintReadOnly)
		FVector center = FVector();

	UPROPERTY()
		float radius = 0;

	UPROPERTY(BlueprintReadOnly)
		FVector centerCircle = FVector();

	UPROPERTY(BlueprintReadOnly)
		FVector startCircle = FVector();

	UPROPERTY(BlueprintReadOnly)
		FVector endCircle = FVector();

	UPROPERTY()
		FRotator centerRotation = FRotator();

	UPROPERTY()
		float startAngle = 0;

	UPROPERTY()
		float endAngle = 0;

	UPROPERTY()
		float sign = 0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SKYPATHLEARNING_API USkyPathComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkyPathComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	USVO_SaveGame* svo;

	UPathFind_Data *pathFind_Data;

	FWaypointD CurrentWaypointData;

	TArray<FWaypointD>Openlist;

	TArray<FWaypointD>Closelist;

	TArray<int32>Obstacles;

	//FVector center;

	//FVector side;

	int32 goalVoxelMortonCode;

	int32 registGoalMortonCode;
	
	/*
	UPROPERTY(BlueprintReadOnly, Category = search)
		FVector startLocation;
	*/
	
	UPROPERTY(BlueprintReadOnly, Category = search)
		FVector goalLocation;

	UPROPERTY(EditAnywhere, Category = MaxSearchCount)
		int MaxSearchCount = 10000;

	UPROPERTY(EditAnywhere, Category = Debug)
		bool isDraw = false;

	UPROPERTY(EditAnywhere, Category = Debug)
		float drawPaththickness = 20.0f;

	UPROPERTY(EditAnywhere, Category = Debug)
		float drawPathDuration = 5.0f;

	UPROPERTY(EditAnywhere, Category = Debug)
		FColor drawPathStartColor = FColor::Red;

	UPROPERTY(EditAnywhere, Category = Debug)
		FColor drawPathGoalColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, Category = Debug)
		FColor drawPathColor = FColor::Green;

	UFUNCTION()
		bool Containslist(int32 mortonCode, TArray<FWaypointD>& list);

	UFUNCTION()
		void Remove(FWaypointD& RemoveTarget);

	UFUNCTION()
		FWaypointD FindByMortonCodeInCloselist(int32 mortonCode);

	UFUNCTION(BlueprintCallable)
		bool Read_svoData(FString svoName,int userIndex);

	UFUNCTION(BlueprintCallable)
		bool Delete_svoData();

	UFUNCTION()
		bool Read_pathFindData(FString name_pathFindData,int userIndex);

	UFUNCTION()
		bool Delete_pathFindData();

	UFUNCTION()
		bool Save_PathFindData(int32 userIndex);

	UFUNCTION(BlueprintCallable, Category = "python")
		void drawSeachNodes(int32 userIndex);

	UFUNCTION()
		bool PathInitialize(FVector& tempStartLocation, FVector& tempGoalLocaion);

	UFUNCTION(BlueprintCallable, Category = Search)
		TArray<FVoxelData> searchVoxels(FVector startLocation,FVector GoalLocaion);

	virtual FWaypointD SearchOpenlist();

	virtual void AddOpnelist();

	UFUNCTION(BlueprintCallable, Category = Search)
		TArray<FVoxelData> GetGoalToStartOnMortonCode();

	UFUNCTION(BlueprintCallable, Category = Search)
		TMap<int32, FVector2D> GetStraightPath(float CornerOffsetRatio, EFunnelAxis funnelAxis, UPARAM(ref) TArray<FVoxelData>& pathDatas);

	UFUNCTION(BlueprintCallable, Category = Search)
		TArray<FVector> GetFunnelAlgorithmResult(float CornerOffsetRatio, UPARAM(ref) TArray<FVoxelData>& pathDatas);

	UFUNCTION(BlueprintCallable, Category = Search)
		TArray<FVector> GetshortcutPathResult(UPARAM(ref) TArray<FVector>& pathDatas, ECollisionChannel channel);
	/*
	UFUNCTION(BlueprintCallable, Category = GetStraightPath)
		TArray<FVector> GetClothoidCurvePath(UPARAM(ref) TArray<FVoxelData>& pathDatas);
	*/

//ClothoidCurve
	UFUNCTION(BlueprintCallable, Category = Search)
		TArray<FVector>calcClothoidSpline(UPARAM(ref) TArray<FVector>& pathDatas);

	UFUNCTION(BlueprintCallable, Category = Search)
		TArray<FVector>calcClothoidSplineV2(UPARAM(ref) TArray<FVector>& pathDatas,bool useCustomRotation,FRotator customRotation);

	UFUNCTION(BlueprintCallable, Category = CalcClothoidCurve)
		TArray<FVector>calcClothoidCurve(int n, float phi1, float phi0, float straightDis, float fov, FVector startLocation, FVector endLocation,float &clothoidLength);

	//UFUNCTION()
	void simpson_integral(FSlope f, float a, float b, std::complex<float>* r);

	//UFUNCTION()
	void phiSimpson_integral(FPhiSlope f, float a, float b, std::complex<float>* r);

	UFUNCTION()
		double fx(int n, float phi, float _x, float fov);

	//(ノット,旋回性能(バンク角の代わりに使用))
	UFUNCTION(BlueprintCallable, Category = CalcClothoidSpline)
		float calcTurnRadius(float turn_speed, float turnningPerformance, float scale);

	float calc_CircleLength(FCircle &circle,float angle1,float angle2);

	double angle_diff(double theta1, double theta2);

	void calcCircleInfo(FCircle& circle, FVector targetLocation);

	void test_circleCheck(FCircle& circle);

	UFUNCTION(BlueprintCallable, Category = CalcClothoidSpline)
		float get_clothoid_angle(float radius);

	UFUNCTION(BlueprintCallable, Category = CalcClothoidSpline)
		float calc_curve_angle(float length, float radius);

	UFUNCTION(BlueprintCallable, Category = CalcClothoidSpline)
		FVector calcMaxCircleAngle(FCircle& circle, FCircle& circle2);

	float getPhi_newtonMethod(float n, float _phi, float fov);

	UFUNCTION(BlueprintCallable)
		float getThreePointAngle(FVector p1, FVector p2, FVector p3);

	UFUNCTION(BlueprintCallable)
		float getMiddleAngle(float angle1, float angle2);

	UFUNCTION(BlueprintCallable)
		float getCenterAngle(float angle1, float angle2);

	UFUNCTION(BlueprintCallable)
		float getMiddleAngle_gimbal(float angle1, float angle2,bool sign);

	FVector calcStartLocationOnArc(FVector current, float arc_radius, FVector center, float sign);

	float convert_Angle_to_controllAngle(float angle);

	float calculateArcLength(float radius, float startAngle, float endAngle);

	TArray<FVector>calcCircleLocations(float startAngle, float endAngle, float radius, FVector curve_start, FVector curve_end, float sign);

	UFUNCTION(BlueprintCallable, Category = CalcClothoidSpline)
		FVector getAngleLocation(float angle, float radius, FVector centerLocation);

	FVector getAngleLocationFromThreepoint(float thetaMax, FVector p1, FVector p2, FVector p3, float radius);

	UFUNCTION()
		float combineFloat(float input, float combineValue);

	UFUNCTION()
		float deltaFloat(float input, float deltaValue);

	float angleOf2DVector(FVector2D p1, FVector2D p2);

	UFUNCTION(BlueprintCallable)
		float calc_TurnRate_Newton(float _sign, FCircle &circle, FVector _target);

	UFUNCTION(BlueprintCallable)
		float calc_TurnRate_Newton2(FCircle& circle, FCircle& circle2);

	UFUNCTION(BlueprintCallable)
		float f_TurnRate(float value, float _sign, float _centerAngle, float _radius, FVector _center, FVector _target);

	/*
	UFUNCTION(BlueprintCallable, Category = ClothoidSpline)
		TArray<FVector> CalcClothoidSplinePathResult(UPARAM(ref) TArray<FVector>& pathDatas, int accuracy);

	*/

	//bool IsNotObstacle(int32 TargetMortonCode);

	//Ownerのルートコンポーネントの大きさだけをとる
	UFUNCTION()
		bool GetBoundingBox(AActor* BoundingTarget, FVector& targetCenter, FVector& targetSide);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothoid", meta = (DisplayName = "CurrentSpeed"))
		float currentSpeed = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothoid", Meta = (ClampMin = 0, ClampMax = 90))
		float turn_rate = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clothoid", Meta = (ClampMin = 0, ClampMax = 89))
		float turningPerformance = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clothoid", Meta = (ClampMin = 0, ClampMax = 250))
		float scale_calcTurnRadius = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Clothoid")
		FRotator currentRotation;

	UPROPERTY(BlueprintReadWrite)
		float test_phiZeroValue;

	UPROPERTY(BlueprintReadWrite)
		float test_phiOneValue;

	UPROPERTY(BlueprintReadOnly)
		int check_loopCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float test_turnRate = 1;

	UPROPERTY(EditAnywhere)
		int check_CircleIndex = 0;

	UPROPERTY(BlueprintReadOnly)
		FCircle test_Circle = FCircle();

	UPROPERTY(BlueprintReadOnly)
		FVector test_location;

	UPROPERTY(EditDefaultsOnly,Meta = (ClampMin = 1, ClampMax = 7))
		int accuracy = 3;

	UPROPERTY(EditDefaultsOnly,Meta = (ClampMin = 1, ClampMax = 100))
		int arc_accuracy = 50;

	UPROPERTY(EditDefaultsOnly, Category = "Clothoid")
		bool useDebugCurrentRotation = false;

	UPROPERTY()
		TArray<int32>searchNodes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "python")
		bool python_MachineLearning = false;

	UPROPERTY(EditAnywhere, Category = "python")
		FString name_PathFindData = "None";

private:
	int lowestLevel = -1;

	FRotator getRotFromCircleTangent(FVector center, FVector circleTangent, int sign);

	float getCircleDirection(FCircle& circle, FVector targetLoc);

	//現在の読み込み番号
	int CurrentListIndex = 0;

	FCircle pre_Circle;

	bool clothoidInitiallize = false;

	bool preControllP = false;

	FRotator preRotation = FRotator();
};
