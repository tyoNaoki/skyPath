

// Fill out your copyright notice in the Description page of Project Settings.


#include "SkyPathComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "PathFind_Data.h"
#include "SVO_Volume.h"

using namespace std;

DEFINE_LOG_CATEGORY_STATIC(Clothoid_Debug_LOG, Error, All);

#define ClothoidDebug(x,y) UE_LOG(Clothoid_Debug_LOG,Error,TEXT(x),y);

// Sets default values for this component's properties
USkyPathComponent::USkyPathComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USkyPathComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

}


// Called every frame
void USkyPathComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool USkyPathComponent::Containslist(int32 mortonCode, TArray<FWaypointD>& list) {
	for (auto& x : list) {
		if (x.faceInfo.mortonCode == mortonCode) {
			return true;
		}
	}

	return false;
}

void USkyPathComponent::Remove(FWaypointD& RemoveTarget) {
	for (auto x : Openlist) {
		if (x == RemoveTarget) {
			Openlist.Remove(x);
			return;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("%d : Remove false MortonCode"), RemoveTarget.faceInfo.mortonCode);
}

FWaypointD USkyPathComponent::FindByMortonCodeInCloselist(int32 mortonCode) {
	for (auto& x : Closelist) {
		if (x.faceInfo.mortonCode == mortonCode) {
			return x;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Not Found %d"), mortonCode);
	return FWaypointD();
}

bool USkyPathComponent::Read_svoData(FString svoName, int userIndex)
{
	if (svoName == "None") {
		UE_LOG(LogTemp, Error, TEXT("svoName is None!!"));
		return false;
	}

	if (!UGameplayStatics::DoesSaveGameExist(svoName, userIndex)) {
		UE_LOG(LogTemp, Error, TEXT("can't read %s!!"),*svoName);
		return false;
	}

	svo = Cast<USVO_SaveGame>(UGameplayStatics::LoadGameFromSlot(svoName, userIndex));
	if (!svo) {
		UE_LOG(LogTemp, Error, TEXT("%s cast failed!!"),*svoName);
		return false;
	}

	if(python_MachineLearning){
		if(!Read_pathFindData(name_PathFindData,userIndex)){
			pathFind_Data = Cast<UPathFind_Data>(UGameplayStatics::CreateSaveGameObject(UPathFind_Data::StaticClass()));
			Save_PathFindData(userIndex);
		}
	}

	return true;
}

bool USkyPathComponent::Delete_svoData()
{
	if(!svo){
		UE_LOG(LogTemp, Error, TEXT("svo is NULL!"));
		return false;
	}
	svo = nullptr;
	
	return true;
}

bool USkyPathComponent::Read_pathFindData(FString _name_pathFindData, int userIndex)
{
	if (_name_pathFindData == "None") {
		UE_LOG(LogTemp, Error, TEXT("svoName is None!!"));
		return false;
	}

	if (!UGameplayStatics::DoesSaveGameExist(_name_pathFindData, userIndex)) {
		UE_LOG(LogTemp, Error, TEXT("can't read %s!!"), *_name_pathFindData);
		return false;
	}

	pathFind_Data = Cast<UPathFind_Data>(UGameplayStatics::LoadGameFromSlot(_name_pathFindData, userIndex));
	if (!pathFind_Data) {
		UE_LOG(LogTemp, Error, TEXT("%s cast failed!!"), *_name_pathFindData);
		return false;
	}

	return true;
}

bool USkyPathComponent::Delete_pathFindData()
{
	if (!pathFind_Data) {
		UE_LOG(LogTemp, Error, TEXT("pathFind_Data is NULL!"));
		return false;
	}
	pathFind_Data = nullptr;

	return true;
}

bool USkyPathComponent::Save_PathFindData(int32 userIndex)
{
	if(!pathFind_Data||name_PathFindData == "None") return false;

	return UGameplayStatics::SaveGameToSlot(pathFind_Data,name_PathFindData,userIndex);
}

void USkyPathComponent::drawSeachNodes(int32 userIndex)
{
	if(!pathFind_Data) {
		if(name_PathFindData == "None"){
			return;
		}
		
		if (UGameplayStatics::DoesSaveGameExist(name_PathFindData, userIndex)) {
			pathFind_Data = Cast<UPathFind_Data>(UGameplayStatics::LoadGameFromSlot(name_PathFindData, userIndex));
		}else{
			return;
		}
	}

	USVO_SaveGame*drawSVOdata = pathFind_Data->GetSVO();
	if(!drawSVOdata){
		return;
	}
	
	UE_LOG(LogTemp, Error, TEXT("pathFind_Data is %d"), pathFind_Data->GetSearchNodes().Num());
	
	UWorld* world = GetWorld();
	
	FLinearColor color = FLinearColor::Green;
	FRotator rotation = FRotator(0,0,0);
	float duration = 5.0f;
	float thickness = 10.0f;
	for(auto &x:pathFind_Data->GetSearchNodes()){
		int level = drawSVOdata->GetLevel(x);
		FVector obstacleExtent = drawSVOdata->GetExtentOfVoxel(x, level);
		UKismetSystemLibrary::DrawDebugBox(world, drawSVOdata->GetLocationFromMortonCode(x, level), obstacleExtent, color, rotation, duration, thickness);
	}
}

bool USkyPathComponent::PathInitialize(FVector& tempStartLocation,FVector& tempGoalLocaion) {
	 
	Openlist.Empty();
	Closelist.Empty();
	FVector startLocation = tempStartLocation;
	goalLocation = tempGoalLocaion;

	

	if (!svo) {
		UE_LOG(LogTemp, Warning, TEXT("svo is NULL!!"));
		return false;
	}

	//�Ώۂ�SVO�������ł�����Ă��Ȃ��̂Ŏ��s��Ԃ�
	/*if(!svo->isGenerate){
		UE_LOG(LogTemp, Warning, TEXT("IsGenerate is false!!"));
		return false;
	}*/
	FVector center = FVector();
	FVector side = FVector();
	//�傫������
	if(!GetBoundingBox(GetOwner(), center, side)){
		UE_LOG(LogTemp, Error, TEXT("getBoundingBox is Failed!!"));
		return false;
	}
	UE_LOG(LogTemp,Error,TEXT("center is %s, side is %s"),*center.ToString(),*side.ToString());

	//lowestLevel�̊���o�����߂̃��[�g���R�[�h
	int32 tempStartMortonCode = svo->GetMortonNumberToCharacter(center, side);
	int32 tempGoalMortonCode = svo->GetMortonNumberFromPoint(tempGoalLocaion);
	registGoalMortonCode = tempGoalMortonCode;

	if (tempStartMortonCode == -1) {
		UE_LOG(LogTemp, Error, TEXT("StartMortonCode is not found!!"));
		return false;
	}

	//�Œ჌�x�����肵�āA����ȏ�̃��x���ɍs�����Ȃ��悤�ɂ���B�Ώۂ����������ꏊ��ʂ点�Ȃ��悤�ɂ��邽��
	lowestLevel = svo->GetLevel(tempStartMortonCode);
	//UE_LOG(LogTemp, Error, TEXT("StartLevel : %d"), lowestLevel);

	if (tempStartMortonCode == 0 || tempGoalMortonCode == 0 || tempGoalMortonCode == -1) {
		UE_LOG(LogTemp, Warning, TEXT("Out of range or %s is too big(Root to %s )"), *GetOwner()->GetName(), *tempGoalLocaion.ToString());
		return false;
	}

	//���ꂼ��̃{�N�Z������Q������̏ꍇ�ɂ�����false��Ԃ�����
	//�{�N�Z���T���̍ۂɃX�^�[�g�͍Œ჌�x���������Ȃ��悤�ɂ���B
	//�T���̍ۂɂ��̃��[�g���{�N�Z���̎��͂�W�J���A��Q�����邩�ǂ������m�F����B
	//���͂̃{�N�Z������Q������̏ꍇ�͏K�����s�Afalse��Ԃ��B
	//Start
	if (!svo->IsNotObstacle(tempStartMortonCode)) {
		UE_LOG(LogTemp, Warning, TEXT("%d(StartMortonCode) is Obstacle!!"), tempStartMortonCode);
		TArray<int32>nearEmptyStartNums = svo->GetNearEmptyVoxelNumbers(tempStartMortonCode);
		if (nearEmptyStartNums.Num() == 0) {
			UE_LOG(LogTemp, Error, TEXT("%d(StartMortonCode)'s NearEmptyVoxel is NotFound"), tempStartMortonCode);
			return false;
		}

		float nearLength = 9999999999.0f;
		for (auto x : nearEmptyStartNums) {
			int level = svo->GetLevel(x);
			FVector location = svo->GetLocationFromMortonCode(x, level);
			float distanceToGoal = abs(FVector::Distance(goalLocation, location));

			if (nearLength > distanceToGoal) {
				nearLength = distanceToGoal;
				tempStartMortonCode = x;
			}
		}
		startLocation = svo->GetLocationFromMortonCode(tempStartMortonCode, svo->GetLevel(tempStartMortonCode));
	}

	if (!svo->GetMortonCodeInNode(tempStartMortonCode, lowestLevel)) {
		UE_LOG(LogTemp, Warning, TEXT("%d(StartMortonCode) is Obstacle!!"), tempGoalMortonCode);
		return false;
	}

	//Goal
	if (!svo->IsNotObstacle(tempGoalMortonCode)) {
		UE_LOG(LogTemp, Warning, TEXT("%d(GoalMortonCode) is Obstacle!!"), tempGoalMortonCode);
		int goallevel = svo->GetLevel(tempGoalMortonCode);
		if (lowestLevel < goallevel) {
			int i = goallevel;

			while (lowestLevel != i) {
				tempGoalMortonCode = svo->GetParentMortonNumber(tempGoalMortonCode);
				i++;
			}
		}

		TArray<int32>nearEmptyGoalNums = svo->GetNearEmptyVoxelNumbers(tempGoalMortonCode);
		if (nearEmptyGoalNums.Num() == 0) {
			UE_LOG(LogTemp, Error, TEXT("%d(GoalMortonCode)'s NearEmptyVoxel is NotFound"), tempGoalMortonCode);
			return false;
		}

		float nearLength = 9999999999.0f;
		for (auto x : nearEmptyGoalNums) {
			int level = svo->GetLevel(x);
			FVector location = svo->GetLocationFromMortonCode(x, level);
			float distanceFromStart = abs(FVector::Distance(location, tempStartLocation));

			if (nearLength > distanceFromStart) {
				nearLength = distanceFromStart;
				tempGoalMortonCode = x;
			}
		}
		goalLocation = svo->GetLocationFromMortonCode(tempGoalMortonCode, svo->GetLevel(tempGoalMortonCode));
	}

	int level = svo->GetLevel(tempGoalMortonCode);
	if (!svo->GetMortonCodeInNode(tempGoalMortonCode, level)) {
		return false;
	}

	int startlevel = svo->GetLevel(tempStartMortonCode);
	int goallevel = svo->GetLevel(tempGoalMortonCode);
	FVector localStartLocation = svo->GetLocationFromMortonCode(tempStartMortonCode, startlevel);
	FVector localGoalLocation = svo->GetLocationFromMortonCode(tempGoalMortonCode, goallevel);
	if (isDraw) {
		UKismetSystemLibrary::DrawDebugBox(GetWorld(), localStartLocation, svo->GetExtentOfVoxel(tempStartMortonCode, startlevel), drawPathStartColor, FRotator(0, 0, 0), drawPathDuration, drawPaththickness);
		UKismetSystemLibrary::DrawDebugBox(GetWorld(), localGoalLocation, svo->GetExtentOfVoxel(tempGoalMortonCode, goallevel), drawPathGoalColor, FRotator(0, 0, 0), drawPathDuration, drawPaththickness);
	}

	float HeuristicCost = FVector::Distance(localStartLocation, localGoalLocation);
	FFaceInfo startInfo = FFaceInfo(tempStartMortonCode, startLocation);

	Openlist.Push(FWaypointD(-1, HeuristicCost, 0.0f, startInfo));
	goalVoxelMortonCode = tempGoalMortonCode;

	UE_LOG(LogTemp, Log,
		TEXT("%s ( MortonCode : %d ) started searching To ( MortonCode : %d ) "),
		*GetOwner()->GetName(), tempStartMortonCode, tempGoalMortonCode);

	if(python_MachineLearning&&pathFind_Data){
		pathFind_Data->InitializeData(svo);
	}

	return true;
}

TArray<FVoxelData> USkyPathComponent::searchVoxels(FVector startLocation,FVector GoalLocaion)
{
	TArray<FVoxelData>result;
	if (!PathInitialize(startLocation,GoalLocaion)) {
		UE_LOG(LogTemp, Warning, TEXT("%s Failed Search by Pathinitialize"), *GetOwner()->GetName());
		return result;
	}

	int count = 0;

	//A*alogorism
	while (true) {

		CurrentWaypointData = SearchOpenlist();

		//null�Ȃ��Openlist����
		if(!CurrentWaypointData) {
			UE_LOG(LogTemp, Error, TEXT("%s Failed Search by openlist was null"),
				*GetOwner()->GetName())
				return TArray<FVoxelData>();
		}

		if(count >= MaxSearchCount){
			UE_LOG(LogTemp, Error, TEXT("%s Failed Search by reached max search %d!!"), *GetOwner()->GetName(), count);
			return TArray<FVoxelData>();
		}

		//UE_LOG(LogTemp,Error,TEXT("CurrentWaypointData.mortonCode : %d"),CurrentWaypointData.mortonCode);
		
		if (isDraw) {
			int tempLevel = svo->GetLevel(CurrentWaypointData.faceInfo.mortonCode);
			UKismetSystemLibrary::DrawDebugBox(GetWorld(), svo->GetLocationFromMortonCode(CurrentWaypointData.faceInfo.mortonCode, tempLevel), svo->GetExtentOfVoxel(CurrentWaypointData.faceInfo.mortonCode, tempLevel), drawPathColor, FRotator(0, 0, 0), drawPathDuration, drawPaththickness + 10.0f);
		}

		//�S�[���ɂ��ǂ蒅���Ă��邩�ǂ���
		if (CurrentWaypointData.faceInfo.mortonCode == goalVoxelMortonCode) {
			break;
		}

		AddOpnelist();

		count++;
	}

	UE_LOG(LogTemp, Log, TEXT("%s A-star Algorizm search by reached search %d count!!"), *GetOwner()->GetName(),count);
	return GetGoalToStartOnMortonCode();
}

FWaypointD USkyPathComponent::SearchOpenlist()
{
	float cost_min = 10000000000.0;
	FWaypointD tempWaypointData = FWaypointD();

	for (auto& x : Openlist) {

		//svo->searchNodes.AddUnique(x.faceInfo.mortonCode);

		if (python_MachineLearning && pathFind_Data) {
			pathFind_Data->AddSearchNodes(x.faceInfo.mortonCode);
		}

		if (x.faceInfo.mortonCode == goalVoxelMortonCode) {
			tempWaypointData = x;
			break;
		}

		if (x.Score < cost_min) {
			cost_min = x.Score;
			tempWaypointData = x;
		}

	}

	//UE_LOG(LogTemp,Error,TEXT("SeachOpnelist %d "),tempWaypointData.mortonCode);
	return tempWaypointData;
}

void USkyPathComponent::AddOpnelist() {
	float tempMoveCost = 0.0f;

	//const FAdjacentNodes* adjacentNodes = svo->svo_Nodes.Find(CurrentWaypointData.faceInfo.mortonCode);
	const FAdjacentNodes& adjacentNodes = svo->GetAdjacentNodes(CurrentWaypointData.faceInfo.mortonCode);

	if (adjacentNodes.adjacentFaces.Num()==0) {
		UE_LOG(LogTemp, Error, TEXT("%d mortonCode is adjacentNodes is NULL!!"),CurrentWaypointData.faceInfo.mortonCode);
		return;
	}

	//CurrentWaypointData�̗א�Waypoint��Openlist�ɓo�^����
	for (auto& x : adjacentNodes.adjacentFaces) {

		if (x.mortonCode == goalVoxelMortonCode) {
			//FVector tempLocation = goalLocation;
			//FVector tempCurrentLocation = CurrentWaypointData.faceInfo.center;
			//tempMoveCost = FVector::Distance(tempCurrentLocation, tempLocation);
			//tempMoveCost = svo->CalcLength(tempCurrentLocation, tempLocation);
			//float moveCost = tempMoveCost + CurrentWaypointData.MoveCost;
			//float HeuristicCost = FVector::Distance(tempLocation, goalLocation);
			FFaceInfo faceInfo = x;
			Openlist.Push(FWaypointD(CurrentWaypointData.faceInfo.mortonCode, 0.0f, 0.0f, faceInfo));
			//Openlist.Push(FWaypointD(CurrentWaypointData.faceInfo.mortonCode, 0.0f, 0.0f, faceInfo));
			break;
		}

		int level = svo->GetLevel(x.mortonCode);
		if (level > lowestLevel) {
			continue;
		}

		//Openlist��Closelist�Ɋ܂܂�Ă��邩
		if (Containslist(x.mortonCode, Openlist) || Containslist(x.mortonCode, Closelist)) {
			continue;
		}

		FVector tempLocation = x.center;
		FVector tempCurrentLocation = CurrentWaypointData.faceInfo.center;
		tempMoveCost = FVector::Distance(tempCurrentLocation, tempLocation);
		//tempMoveCost = svo->CalcLength(tempCurrentLocation, tempLocation);

		float moveCost = tempMoveCost + CurrentWaypointData.MoveCost;
		float HeuristicCost = FVector::Distance(tempLocation, goalLocation);

		float score = HeuristicCost + moveCost;
		//UE_LOG(LogTemp,Error,TEXT("x.xyLeft is %s"),*x.xyLeft.ToString());
		//UE_LOG(LogTemp, Error, TEXT("x.xy.mortonCode is %d"), x.xy.mortonCode);
		FFaceInfo faceInfo = x;

		Openlist.Push(FWaypointD(CurrentWaypointData.faceInfo.mortonCode, score, moveCost, faceInfo));
	}

	Remove(CurrentWaypointData);
	Closelist.Push(CurrentWaypointData);
}

bool USkyPathComponent::GetBoundingBox(AActor* BoundingTarget, FVector& targetCenter, FVector& targetSide) {
	UCapsuleComponent* TargetComp = NULL;
	TArray<USceneComponent* >comps;
	BoundingTarget->GetComponents(comps);
	//TArray<UCapsuleComponent* >comps;
	//BoundingTarget->GetComponents(comps);

	if(!comps.IsValidIndex(0)||comps[0] == nullptr)
	{
		// �C���f�b�N�X���L���ŁA���̗v�f��Null�ł��B
		UE_LOG(LogTemp, Error, TEXT("%s : Not Founding CapsuleComponents"), *BoundingTarget->GetName());
		return false;
	}

	//TargetComp = comps[0];

	float Scale = 0.0f;

	//UKismetSystemLibrary::GetComponentBounds(TargetComp, targetCenter, targetSide, Scale);
	UKismetSystemLibrary::GetComponentBounds(comps[0], targetCenter, targetSide, Scale);
	return true;
}

TArray<FVoxelData> USkyPathComponent::GetGoalToStartOnMortonCode() {
	TArray<FVoxelData>pathDatas;

	FFaceInfo faceInfo = FFaceInfo(registGoalMortonCode, goalLocation);
	pathDatas.Push(FVoxelData(goalLocation, FVector(1.0f, 1.0f, 1.0f), faceInfo));

	while (true) {
		//UE_LOG(LogTemp,Log,TEXT("mortoncode : %d , %d"),CurrentWaypointData.mortonCode,CurrentWaypointData.parentMortonCode);

		int32 level = svo->GetLevel(CurrentWaypointData.faceInfo.mortonCode);
		FVector tempExtent = svo->GetExtentOfVoxel(CurrentWaypointData.faceInfo.mortonCode, level);
		FVector tempCenter = svo->GetLocationFromMortonCode(CurrentWaypointData.faceInfo.mortonCode, level);
		pathDatas.Push(FVoxelData(tempCenter, tempExtent, CurrentWaypointData.faceInfo));
		if (CurrentWaypointData.parentMortonCode == -1) {
			break;
		}
		else {
			CurrentWaypointData = FindByMortonCodeInCloselist(CurrentWaypointData.parentMortonCode);
		}
	}

	if(python_MachineLearning){
		Save_PathFindData(0);
	}

	return pathDatas;
}

TMap<int32, FVector2D> USkyPathComponent::GetStraightPath(float CornerOffsetRatio, EFunnelAxis funnelAxis, UPARAM(ref) TArray<FVoxelData>& pathDatas)
{

	TMap<int32, FVector2D>result;

	// �R�[�i�[��������ւ��炷�����̊����i0.5�Ń|�[�^���G�b�W�̒��S��ʂ�j
	// 0�̏ꍇ�͂��ꂢ�ɕǂɉ������o�H����邪�K�؂Ȓl�ɐݒ肷��Εǂ��班�����ꂽ�����I�Ȍo�H�����.
	// CornerOffset��0.0f ~ 1.0f�͈̔͂Ɏ��߂�.
	CornerOffsetRatio = FMath::Max(FMath::Min(CornerOffsetRatio, 1.f), 0.f);
	TArray<FCellEdge> PortalEdges;

	FVector2D Left, Right = FVector2D();

	for (int i = pathDatas.Num() - 1; i >= 0; i--)
	{
		bool IsNotSkip = true;
		if (funnelAxis == EFunnelAxis::FA_XY) {
			if (pathDatas[i].faceInfo.skipAxis == ESkipAxis::SA_xy) {
				IsNotSkip = false;
			}
			else {
				Left = pathDatas[i].faceInfo.xyLeft;
				Right = pathDatas[i].faceInfo.xyRight;
			}
		}
		else if (funnelAxis == EFunnelAxis::FA_XZ) {
			if (pathDatas[i].faceInfo.skipAxis == ESkipAxis::SA_xz) {
				IsNotSkip = false;
			}
			else {
				Left = pathDatas[i].faceInfo.xzLeft;
				Right = pathDatas[i].faceInfo.xzRight;
			}
		}
		else {
			if (pathDatas[i].faceInfo.skipAxis == ESkipAxis::SA_yz) {
				IsNotSkip = false;
			}
			else {
				Left = pathDatas[i].faceInfo.yzLeft;
				Right = pathDatas[i].faceInfo.yzRight;
			}
		}
		if (IsNotSkip) {
			PortalEdges.Push(FCellEdge(Left, Right, pathDatas[i].faceInfo.mortonCode));
		}
	}

	// �t�@���l���A���S���Y�����J�n����|�[�^���G�b�W�̊�_�Ɗ�_���猩�č����̓_�ƉE���̓_
	FVector2D PortalApex, PortalLeft, PortalRight;
	int ApexIndex = 0, LeftIndex = 0, RightIndex = 0;

	// �|�[�^���͎O�p�`
	PortalApex = PortalEdges[0].left;	// �|�[�^���̒��_
	PortalLeft = PortalEdges[1].left;	// �|�[�^���G�b�W�̍����̓_
	PortalRight = PortalEdges[1].right;	// �|�[�^���G�b�W�̉E���̓_
	LeftIndex = 1;
	RightIndex = 1;

	//UE_LOG(LogTemp,Error,TEXT("PortalEdges's Num is %d"),PortalEdges.Num());

	for (int i = 2; i < PortalEdges.Num(); i++)
	{
		FVector2D NextLeft = PortalEdges[i].left;		// ���̃|�[�^���G�b�W�̍��̌��_
		FVector2D NextRight = PortalEdges[i].right;	// ���̃|�[�^���G�b�W�̉E�̌��_

		// �E�̓_���X�V

		// NextRight �� PortalRight ���������ɂ���Ƃ������Ƃ̓t�@���l������苷���o����Ƃ������ƂȂ̂� PortalRight �� Right �ւƐi�߂�.
		FFunnelAlgorithmMath::EPointSide Classification = FFunnelAlgorithmMath::ClassifyPoint(PortalApex, PortalRight, NextRight);
		if (Classification != FFunnelAlgorithmMath::EPointSide::RIGHT_SIDE)
		{
			// NextRight �� PortalApex �� PortalLeft ����Ȃ�x�N�g�������E���ɂ��邩�`�F�b�N�i�������O�p�`���\������Ă��邩�`�F�b�N�j
			Classification = FFunnelAlgorithmMath::ClassifyPoint(PortalApex, PortalLeft, NextRight);
			if (PortalApex == PortalRight || Classification == FFunnelAlgorithmMath::EPointSide::RIGHT_SIDE)//|| Classification == FFunnelAlgorismMath::EPointSide::ON_LINE)
			{
				// �t�@���l�������̉E�̈ʒu�ɐi�߂�
				PortalRight = NextRight;
				RightIndex = i;
			}
			// NextRight �� PortalLeft ��ǂ��z���Ă��܂���
			else
			{
				PortalLeft = PortalLeft * (1 - CornerOffsetRatio) + (PortalEdges[LeftIndex].right * CornerOffsetRatio);
				//Result.Push(PortalLeft);
				result.Add(PortalEdges[LeftIndex].mortonCode, PortalLeft);

				// ���̓_����|�[�^���̒��_���쐬
				PortalApex = PortalLeft;
				ApexIndex = LeftIndex;
				i = ApexIndex + 1;
				if (i < PortalEdges.Num()) {
					// �|�[�^�������Z�b�g
					PortalLeft = PortalEdges[i].left;
					PortalRight = PortalEdges[i].right;
					LeftIndex = i;
					RightIndex = i;
				}

				//// Restart scan
				continue;
			}
		}

		// ���̓_���X�V
		// ����Ă��邱�Ƃ͉E�̓_�Ɠ���

		Classification = FFunnelAlgorithmMath::ClassifyPoint(PortalApex, PortalLeft, NextLeft);
		if (Classification != FFunnelAlgorithmMath::EPointSide::LEFT_SIDE)
		{
			Classification = FFunnelAlgorithmMath::ClassifyPoint(PortalApex, PortalRight, NextLeft);
			if (PortalApex == PortalLeft || Classification == FFunnelAlgorithmMath::EPointSide::LEFT_SIDE)// || Classification == FFunnelAlgorismMath::EPointSide::ON_LINE)
			{

				PortalLeft = NextLeft;
				LeftIndex = i;
			}
			else
			{
				PortalRight = PortalRight * (1 - CornerOffsetRatio) + (PortalEdges[RightIndex].left * CornerOffsetRatio);
				//Result.Push(PortalRight);
				result.Add(PortalEdges[RightIndex].mortonCode, PortalRight);

				// �E�̓_����|�[�^���̒��_���쐬
				PortalApex = PortalRight;
				ApexIndex = RightIndex;
				i = ApexIndex + 1;
				if (i < PortalEdges.Num()) {
					// �|�[�^�������Z�b�g
					PortalLeft = PortalEdges[i].left;
					PortalRight = PortalEdges[i].right;
					LeftIndex = i;
					RightIndex = i;
				}

				// Restart Scan
				continue;
			}
		}
	}

	return result;
}


TArray<FVector> USkyPathComponent::GetFunnelAlgorithmResult(float CornerOffsetRatio, UPARAM(ref)TArray<FVoxelData>& pathDatas)
{
	TArray<FVector>result;
	TArray<FFunnelAlgorithmAxis>axisResult;

	//���ꂼ��̎��ł̌��ʎ擾
	TMap<int32, FVector2D>fa_XYresult = GetStraightPath(CornerOffsetRatio, EFunnelAxis::FA_XY, pathDatas);
	TMap<int32, FVector2D>fa_XZresult = GetStraightPath(CornerOffsetRatio, EFunnelAxis::FA_XZ, pathDatas);
	TMap<int32, FVector2D>fa_YZresult = GetStraightPath(CornerOffsetRatio, EFunnelAxis::FA_YZ, pathDatas);

	axisResult.Add(FFunnelAlgorithmAxis(true, true, true, pathDatas.Last().faceInfo.center, FVector::ZeroVector, FVector::ZeroVector));
	//UE_LOG(LogTemp, Error, TEXT("pathDatas.Last().faceInfo.center is %s"), *pathDatas.Last().faceInfo.center.ToString());

	//�ʁX�̎��̃t�@���l���A���S���Y�����ʂ��}�[�W���āA��ɂ���B
	for (int i = pathDatas.Num() - 2; i >= 1; i--) {
		FVector fa_Result = FVector::ZeroVector;
		bool isFound = false;
		bool IsdecidedX = false;
		bool IsdecidedY = false;
		bool IsdecidedZ = false;

		FVector2D* xy = fa_XYresult.Find(pathDatas[i].faceInfo.mortonCode);
		FVector2D* xz = fa_XZresult.Find(pathDatas[i].faceInfo.mortonCode);
		FVector2D* yz = fa_YZresult.Find(pathDatas[i].faceInfo.mortonCode);

		//fa_Result�̌��ʂ͕����̖ʂŊm�肵���ꍇ�A���ꂼ��̊m�肵�����ʂōX�V���Ă���
		if (xy) {
			fa_Result = FVector(xy->X, xy->Y, 0.0f);
			IsdecidedX = true;
			IsdecidedY = true;
			isFound = true;
		}

		if (xz) {
			fa_Result = FVector(xz->X, fa_Result.Y, xz->Y);
			IsdecidedX = true;
			IsdecidedZ = true;
			isFound = true;
		}

		if (yz) {
			fa_Result = FVector(fa_Result.X, yz->X, yz->Y);
			IsdecidedZ = true;
			IsdecidedY = true;
			isFound = true;
		}

		if (isFound) {
			if (IsdecidedX && IsdecidedY && IsdecidedZ) {
				UE_LOG(LogTemp, Error, TEXT("dexided mortonCode : %d"), pathDatas[i].faceInfo.mortonCode);
			}
			UE_LOG(LogTemp, Error, TEXT("axisResult[%d].Add(mortonCode : %d)"), axisResult.Num(), pathDatas[i].faceInfo.mortonCode);
			FVector max = pathDatas[i].location + pathDatas[i].extent;
			FVector min = pathDatas[i].location - pathDatas[i].extent;
			axisResult.Add(FFunnelAlgorithmAxis(IsdecidedX, IsdecidedY, IsdecidedZ, fa_Result, max, min));
		}
	}

	//�S�[���ǉ�
	//UE_LOG(LogTemp, Error, TEXT("pathDatas[0].faceInfo.center is %s"), *pathDatas[0].faceInfo.center.ToString());
	axisResult.Add(FFunnelAlgorithmAxis(true, true, true, pathDatas[0].faceInfo.center, FVector::ZeroVector, FVector::ZeroVector));

	//���m��̎����m�肳����
	for (int i = 0; i < axisResult.Num(); i++) {
		//FVector resultLocation = FVector::ZeroVector;

		if (axisResult[i].isDecideX == false) {
			float x1 = 0.0f, x2 = 0.0f;
			float y1 = 0.0f, y2 = 0.0f;
			float z1 = 0.0f, z2 = 0.0f;

			//�O
			for (int j = i - 1; j >= 0; j--) {
				if (axisResult[j].isDecideX && axisResult[j].isDecideY && axisResult[j].isDecideZ) {
					x1 = axisResult[j].location.X;
					y1 = axisResult[j].location.Y;
					z1 = axisResult[j].location.Z;
					break;
				}
			}
			//���
			for (int j = i + 1; j < axisResult.Num(); j++) {
				if (axisResult[j].isDecideX && axisResult[j].isDecideY && axisResult[j].isDecideZ) {
					x2 = axisResult[j].location.X;
					y2 = axisResult[j].location.Y;
					z2 = axisResult[j].location.Z;
					break;
				}
			}

			//���m�肵�Ă��Ȃ������m�肳����
			if (x2 == x1) {
				axisResult[i].location.X = x1;
			}
			else {
				if (axisResult[i].location.Y != y1 || y2 != y1) {
					axisResult[i].location.X = ((axisResult[i].location.Y - y1) / (y2 - y1)) * (x2 - x1) + x1;
				}
				else if (axisResult[i].location.Z != z1 || z2 != z1) {
					axisResult[i].location.X = ((axisResult[i].location.Z - z1) / (z2 - z1)) * (x2 - x1) + x1;
				}
				else {
					axisResult[i].location.X = x1;
				}
			}
			axisResult[i].location.X = FMath::Clamp(axisResult[i].location.X, axisResult[i].widthMin.X, axisResult[i].widthMax.X);
			axisResult[i].isDecideX = true;

		}
		else if (axisResult[i].isDecideY == false) {
			float x1 = 0.0f, x2 = 0.0f;
			float y1 = 0.0f, y2 = 0.0f;
			float z1 = 0.0f, z2 = 0.0f;

			//�O
			for (int j = i - 1; j >= 0; j--) {
				if (axisResult[j].isDecideX && axisResult[j].isDecideY && axisResult[j].isDecideZ) {
					x1 = axisResult[j].location.X;
					y1 = axisResult[j].location.Y;
					z1 = axisResult[j].location.Z;
					break;
				}
			}
			//���
			for (int j = i + 1; j < axisResult.Num(); j++) {
				if (axisResult[j].isDecideX && axisResult[j].isDecideY && axisResult[j].isDecideZ) {
					x2 = axisResult[j].location.X;
					y2 = axisResult[j].location.Y;
					z2 = axisResult[j].location.Z;
					break;
				}
			}
			//���m�肵�Ă��Ȃ������m�肳����
			if (y2 == y1) {
				axisResult[i].location.Y = y1;
			}
			else {
				if (axisResult[i].location.X != x1 || x2 != x1) {
					axisResult[i].location.Y = ((axisResult[i].location.X - x1) / (x2 - x1)) * (y2 - y1) + y1;
				}
				else if (axisResult[i].location.Z != z1 || z2 != z1) {
					axisResult[i].location.Y = ((axisResult[i].location.Z - z1) / (z2 - z1)) * (y2 - y1) + y1;
				}
				else {
					axisResult[i].location.Y = y1;
				}
			}
			axisResult[i].location.Y = FMath::Clamp(axisResult[i].location.Y, axisResult[i].widthMin.Y, axisResult[i].widthMax.Y);
			axisResult[i].isDecideY = true;

		}
		else if (axisResult[i].isDecideZ == false) {
			float x1 = 0.0f, x2 = 0.0f;
			float y1 = 0.0f, y2 = 0.0f;
			float z1 = 0.0f, z2 = 0.0f;
			//�O
			for (int j = i - 1; j >= 0; j--) {
				if (axisResult[j].isDecideX && axisResult[j].isDecideY && axisResult[j].isDecideZ) {
					x1 = axisResult[j].location.X;
					y1 = axisResult[j].location.Y;
					z1 = axisResult[j].location.Z;
					break;
				}
			}
			//���
			for (int j = i + 1; j < axisResult.Num(); j++) {
				if (axisResult[j].isDecideX && axisResult[j].isDecideY && axisResult[j].isDecideZ) {
					x2 = axisResult[j].location.X;
					y2 = axisResult[j].location.Y;
					z2 = axisResult[j].location.Z;
					break;
				}
			}
			//���m�肵�Ă��Ȃ������m�肳����
			if (z2 == z1) {
				axisResult[i].location.Z = z1;
			}
			else {
				if (axisResult[i].location.X != x1 || x2 != x1) {
					axisResult[i].location.Z = ((axisResult[i].location.X - x1) / (x2 - x1)) * (z2 - z1) + z1;
				}
				else if (axisResult[i].location.Y != y1 || y2 != y1) {
					axisResult[i].location.Z = ((axisResult[i].location.Y - y1) / (y2 - y1)) * (z2 - z1) + z1;
				}
				else {
					axisResult[i].location.Z = z1;
				}
			}
			axisResult[i].location.Z = FMath::Clamp(axisResult[i].location.Z, axisResult[i].widthMin.Z, axisResult[i].widthMax.Z);
			axisResult[i].isDecideZ = true;

		}

		result.Add(axisResult[i].location);
	}

	return result;
}


TArray<FVector> USkyPathComponent::GetshortcutPathResult(UPARAM(ref)TArray<FVector>& pathDatas, ECollisionChannel channel) {
	TArray<FVector> result;

	//���C��΂����߂̃p�����[�^�ݒ�
	FHitResult HitCall(ForceInit);
	FCollisionQueryParams ParamsCall = FCollisionQueryParams(FName(TEXT("Trace")), true, GetOwner());
	ParamsCall.bTraceComplex = true;
	//Params->bTraceAsyncScene = true;
	ParamsCall.bReturnPhysicalMaterial = true;

	//DrawDebugLine(GetWorld(), Start, End, FColor::Green, true);

	//���ݒn
	int current = 0;

	//�I���n
	int goal = pathDatas.Num() - 1;

	//pathDatas�̔�ׂ�Ώ�
	int next = goal;

	UWorld* world = GetWorld();

	//�X�^�[�g�n�_��ǉ�
	result.Add(pathDatas[current]);

	while (current != goal) {
		//���ݒn���玟�̒n�_�܂Ń��C���΂��A��Q������擾
		bool Traced = world->LineTraceSingleByChannel(
			//*Hit,     //result
			HitCall,
			pathDatas[current],
			pathDatas[next],
			channel,    //collision channel
			ParamsCall
		);

		//��Q������
		if (Traced && HitCall.bBlockingHit) {
			//���̒n�_����O�̒n�_�ɍX�V
			next--;

			//���ݒn�Ɣ�ׂ�n�_���������ǂ���
			if (current == next) {
				//���ʂɌ��ݒn�̎��̒n�_��ǉ�
				result.Add(pathDatas[current + 1]);

				//���ݒn�����̒n�_�ɍX�V
				current++;

				//��ׂ�n�_���S�[���Ƀ��Z�b�g
				next = goal;
			}
		}
		else { //��Q������

		   //���ʂɎ��̒n�_��ǉ�
			result.Add(pathDatas[next]);

			//���ݒn�����̒n�_�ɍX�V
			current = next;

			//���̒n�_����U�S�[���n�_�Ƀ��Z�b�g
			next = goal;
		}
	}

	return result;
}

//ClothoidSpline//

TArray<FVector> USkyPathComponent::calcClothoidSpline(UPARAM(ref) TArray<FVector>& pathDatas)
{
	TArray<FVector>results;
	
	if (pathDatas.Num() <= 1) {
		UE_LOG(LogTemp, Error, TEXT("TArray.Num() is %d"), pathDatas.Num());
		return results;
	}

	//���ݑ��x�ݒ�
	float preSpeed = currentSpeed;

	int splineCount = pathDatas.Num() - 1;

	float s = 0.5f;

	int current = 0;
	int target = 1;
	int next = 2;
	FVector currentLoc = pathDatas[current];
	FVector2D currentLocXY = FVector2D(currentLoc.X, currentLoc.Y);
	//FRotator currentRot = this->currentRotation;
	FVector targetLoc;
	FVector2D targetLocXY;
	int splineNum = pathDatas.Num();

	if(!clothoidInitiallize){
		if (GetOwner()) {
			//�f�o�b�O���[�h:�n�_�̃N���\�C�h�����p�x��C�ӂ̊p�x�ɐݒ肷��
			//�ʏ펞:���݂̋@�̂̊p�x���n�_�̏����p�x�ɐݒ�
			if (useDebugCurrentRotation) {
				pre_Circle = FCircle(GetOwner()->GetActorLocation(), currentRotation);
				UE_LOG(Clothoid_Debug_LOG,Error,TEXT("currentRotation is %s"),*currentRotation.ToString());
				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("debugMode is true"));
			}else{
				currentRotation = GetOwner()->GetActorRotation();
				pre_Circle = FCircle(GetOwner()->GetActorLocation(), currentRotation);
			}
		}else{
			UE_LOG(LogTemp,Error,TEXT("GetOwner() is Null"));
			return results;
		}
		clothoidInitiallize = true;
	}

	UE_LOG(Clothoid_Debug_LOG,Error,TEXT("pre_Circle.centerRotation is %s"),*pre_Circle.centerRotation.ToString());

	results.Add(currentLoc);

	for (; current < splineCount;) {
		check_loopCount = current;

		//�Ȑ���(L)
		float curveLength = 0;

		//�����p�x
		float turn_rate_start = 0;

		//�I�_�p�x
		float turn_rate_end = 0;

		//�N���\�C�h�Ȑ��Ɖ~�ʂ̃o�����X
		//�����قǁA�~�ʂ̐��񗦂������Ȃ�A�N���\�C�h�Ȑ��̋ȗ���0�ɋ߂Â��i�����j
		//�Ⴂ�قǁA�~�ʂ̐��񗦂�0�ɋ߂Â��A�N���\�C�h�Ȑ����ȗ���0�ɋ߂Â�(�Ȑ�)
		//0�̏ꍇ�A�~�ʂ��g�p�����A�N���\�C�h�Ȑ��݂̂̃J�[�u
		float r_temp = 0.0f;

		//���̐���_�̍��W�ݒ�
		targetLoc = pathDatas[target];
		targetLocXY = FVector2D(targetLoc.X, targetLoc.Y);

		//����p�x
		float turnnningAngle = 1;

		//�N���\�C�h�ɂȂ���~�ʍ��W(�񎟌�)
		FVector2D arc_start_XY;

		FCircle circle = FCircle();

		//�N���\�C�h�Ȑ��z��
		TArray<FVector>curve;

		//���񔼌a
		circle.radius = calcTurnRadius(currentSpeed, turningPerformance, scale_calcTurnRadius);

		//���̃p�X������ꍇ
		if (pathDatas.IsValidIndex(next)) {
			FVector2D nextLocXY = FVector2D(pathDatas[next].X,pathDatas[next].Y);
			float distanceToTarget = UKismetMathLibrary::Distance2D(targetLocXY, currentLocXY);
			float distanceToNext = UKismetMathLibrary::Distance2D(targetLocXY,nextLocXY);

			//���񔼌a���p�X�Ԃ̋����̔������������ꍇ�A�p�X�Ԃ̋����̔�������񔼌a�ɂ���
			if ((distanceToTarget / 2.1) <= (circle.radius * 2.0)) {
				circle.radius = (distanceToTarget / 2) / 2;
			}else if ((distanceToNext / 2.1) <= (circle.radius * 2)) {
				circle.radius = (distanceToNext / 2) / 2;
			}

			//�O�_�̒��S�p�����߂�
			float angle_threePoint = getThreePointAngle(FVector(currentLocXY, 0), FVector(targetLocXY, 0), FVector(nextLocXY, 0));
			UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] angle_threePoint is  %f"),check_loopCount, angle_threePoint);
			float sign = UKismetMathLibrary::SignOfFloat(angle_threePoint)>0.0 ? 1.0f : -1.0f;
			float r1 = UKismetMathLibrary::FindLookAtRotation(FVector(targetLocXY,0), FVector(currentLocXY,0)).Yaw;
			float r2 = UKismetMathLibrary::FindLookAtRotation(FVector(targetLocXY,0), FVector(nextLocXY,0)).Yaw;
			FRotator angle_Middle = FRotator(0.0f,getCenterAngle(r1,r2),0.0f);
			UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] r1 = %f, r2 = %f, angleMiddle = %s"), current,r1,r2, *angle_Middle.ToString());
			//�~�ʂ̒��S�p�x����L�̌v�Z�ŋ��߂����S�p�Őݒ�
			circle.centerRotation = UKismetMathLibrary::ComposeRotators(angle_Middle, FRotator(0, 180, 0));
			//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d] circle.centerRotation = %s"),current,*circle.centerRotation.ToString());
			
			//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d]angle_threePoint = %f"),current,angle_threePoint);
			
			//�~�ʂ̒��S�_�ƍ��W�Ɛ��񎞂̒��S���v�Z
			circle.center = FVector(targetLocXY,0) + UKismetMathLibrary::GetForwardVector(angle_Middle) * circle.radius;
			circle.centerCircle = getAngleLocation(circle.centerRotation.Yaw, circle.radius, circle.center);
			
			//�O��̐���_����_�̍��W�̏ꍇ
			if (pre_Circle.isPoint) {

				//�~�ʂ̐���ő�p�x���j���[�g���@�Ōv�Z
				float turn_rate_startMax = 0.0f;
				if(r_temp >= 0.1f){
					turn_rate_startMax = calc_TurnRate_Newton(sign, circle, FVector(currentLocXY, 0));
					turn_rate_start = turn_rate_startMax * r_temp;
				}

				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("turn_rate_startMax %f"), turn_rate_startMax);

				//�~�ʂ̎n�_�v�Z
				//startRot.Yaw�����̕������A�����
				//startRot.Yaw�����̕������A�E���
				FRotator startRot = FRotator(0, sign * turn_rate_start, 0);
				circle.startAngle = UKismetMathLibrary::NormalizedDeltaRotator(circle.centerRotation, startRot).Yaw;
				circle.startCircle = getAngleLocation(circle.startAngle, circle.radius, circle.center);

				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] circle.startCircle %s"),check_loopCount,*circle.startCircle.ToString());
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] startRot %f"), check_loopCount, startRot.Yaw);
	
				circle.sign = getCircleDirection(circle,FVector(currentLocXY,0));
				circle.isPoint = false;

				arc_start_XY = FVector2D(circle.startCircle.X, circle.startCircle.Y);

				//�����擾
				float _hValue = UKismetMathLibrary::Distance2D(arc_start_XY, currentLocXY);
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[current : %d] _hvalue : %f"), current, _hValue);

				int num = _hValue * ((float)accuracy / 100.0);

				FVector vec_tangent = circle.startCircle - FVector(currentLocXY,0);
				//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d] vec_tangent : %s"),current,*vec_tangent.ToString());

				float _phi0Value = pre_Circle.centerRotation.Yaw;
				float _phi1Value = turn_rate_start != 0.0f ? 
				getRotFromCircleTangent(circle.center, circle.startCircle, circle.sign).Yaw :
				combineFloat(circle.centerRotation.Yaw,90.0f*circle.sign);

				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d ~ %d] _phi0Value is %f , _phi1Value is %f"), current, target, _phi0Value, _phi1Value);

				//����p�v�Z
				float _fov = deltaFloat(angleOf2DVector(currentLocXY, arc_start_XY), _phi0Value);

				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] angleOf2DVector is %f, _phiValue0 is %f, fov is %f"), current, angleOf2DVector(currentLocXY, arc_start_XY), _phi0Value,_fov);

				//0���Z�΍�̂��߂ɂP������
				if (num < 1) {
					num = 1;
				}

				//�N���\�C�h�Ȑ��𐶐�
				float clothoidLength = 0.0f;
				TArray<FVector> clothoidCurve = calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, FVector(currentLocXY, 0), circle.startCircle,clothoidLength);

				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] clothoidLength is %f"), current, clothoidLength);
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] currentLoc : %s, clothoidCurve Target : %s"), current, *FVector(currentLocXY, 0).ToString(),*circle.startCircle.ToString());
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] clothoidCurve.last() : %s"), current, *clothoidCurve.Last().ToString());

				//�N���\�C�h�Ȑ��ŏI�_�̍��W�Ɖ~�ʂ̍��W�̍������~�ʂɓK�p���čČv�Z
				calcCircleInfo(circle, clothoidCurve.Last());

				//�����v�Z�Ɠ����蔻��𗼕��s��
				//isNotObstale�g�p����

				//�~�ʍ��W�v�Z
				TArray<FVector>circleLocations = calcCircleLocations(circle.startAngle, circle.centerRotation.Yaw, circle.radius, clothoidCurve.Last(), circle.centerCircle, circle.sign);
				//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("circle,sign is %f"),circle.sign);

				float circleLength = circleLocations.Num()!=0 ? calc_CircleLength(circle,circle.startAngle,circle.centerRotation.Yaw) : 0.0f;
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("circle.startAngle is %f,circle.centerRotation.yaw is %f"),circle.startAngle,circle.centerRotation.Yaw );
				//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("cirlceLength is %f"), calc_CircleLength(circle, circle.startAngle, circle.centerRotation.Yaw));
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] circleLocations.Num() is %d"), current,circleLocations.Num());
			
				float curveHeight = targetLoc.Z - currentLoc.Z;
				//ClothoidDebug("[0] curveHeight : %f",curveHeight);
				curveLength = clothoidLength + circleLength;
				float height = clothoidLength * (1.0f / curveLength) * curveHeight;
				//ClothoidDebug("[0] curveLength is %f",curveLength);
				//ClothoidDebug("[0] clothoidLength is %f", clothoidLength);
				//ClothoidDebug("[0] circleLength is %f", circleLength);
				//ClothoidDebug("[0] height : %f", height);

				int i_height = 0;
				for (auto& x : clothoidCurve) {
					if (i_height == clothoidCurve.Num() - 1) {
						x.Z = height + currentLoc.Z;
						break;
					}
					float stepVal = height / clothoidCurve.Num();
					x.Z = stepVal * i_height + stepVal +currentLoc.Z;
					i_height++;
				}
				height = curveHeight - height;
				i_height = 0;
				for (auto& x : circleLocations) {
					if(i_height == circleLocations.Num()-1){
						x.Z = height + clothoidCurve.Last().Z;
						break;
					}
					float stepVal = height / circleLocations.Num();
					x.Z = stepVal * i_height + stepVal + clothoidCurve.Last().Z;
					i_height++;
				}
				if(circleLocations.Num()!=0){
					UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] circleLocations is %s"), current, *circleLocations.Last().ToString());
				}
				

				curve.Append(clothoidCurve);
				curve.Append(circleLocations);

			//�O��̐���_���~�ʂ̏ꍇ
			}else {
				float turn_rate_endMax = 0.0f;
				if(r_temp>=0.1f){
					turn_rate_endMax = calc_TurnRate_Newton2(pre_Circle, circle);
					turn_rate_end = turn_rate_endMax * r_temp;
				}
				
				FRotator endRot = FRotator(0, pre_Circle.sign * turn_rate_end, 0);
				//�p�x
				pre_Circle.endAngle = UKismetMathLibrary::NormalizedDeltaRotator(pre_Circle.centerRotation, endRot).Yaw;

				//���W
				pre_Circle.endCircle = getAngleLocation(pre_Circle.endAngle, pre_Circle.radius, pre_Circle.center);

				//circle�̎n�_�v�Z
				float turn_rate_startMax = 0.0f;

				if (r_temp >= 0.1f) {
					turn_rate_startMax = calc_TurnRate_Newton(sign * -1, circle, pre_Circle.endCircle);
					turn_rate_start = turn_rate_startMax * r_temp;
				}
				
				FRotator startRot = FRotator(0, sign * turn_rate_start, 0);
				//�p�x
				circle.startAngle = UKismetMathLibrary::NormalizedDeltaRotator(circle.centerRotation, startRot).Yaw;
				//���W
				circle.startCircle = getAngleLocation(circle.startAngle, circle.radius, circle.center);
				circle.isPoint = false;
				circle.sign = getCircleDirection(circle,FVector(currentLocXY,0));
				//circle.sign *= -1;
				
				TArray<FVector> circleLocations = calcCircleLocations(pre_Circle.centerRotation.Yaw, pre_Circle.endAngle, pre_Circle.radius, FVector(currentLocXY,0), pre_Circle.endCircle, pre_Circle.sign);
				float circleLength = circleLocations.Num()!=0 ? 
				pre_Circle.radius * abs(deltaFloat(pre_Circle.centerRotation.Yaw, pre_Circle.endAngle)) : 0.0f;
				FVector circleEnd = circleLocations.Num() != 0 ? circleLocations.Last() : pre_Circle.endCircle;
				FVector2D circleEndXY = FVector2D(circleEnd.X, circleEnd.Y);

				circle.startCircle.Z = 0.0f;
				arc_start_XY = FVector2D(circle.startCircle.X, circle.startCircle.Y);
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] arc_start_XY : %s"), current,*arc_start_XY.ToString());

				//�����擾
				float _hValue = UKismetMathLibrary::Distance2D(arc_start_XY, circleEndXY);
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[current : %d] _hvalue : %f"), current, _hValue);

				int num = _hValue * ((float)accuracy / 100.0);

				FVector vec_tangent = circle.startCircle - circleEnd;
				float _phi0Value = getRotFromCircleTangent(pre_Circle.center, pre_Circle.endCircle, pre_Circle.sign).Yaw;
				float _phi1Value = getRotFromCircleTangent(circle.center, circle.startCircle, circle.sign).Yaw;

				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d ~ %d] _phi0Value is %f , _phi1Value is %f"), current, target, _phi0Value, _phi1Value);

				//����p�v�Z
				float _fov = deltaFloat(angleOf2DVector(circleEndXY, arc_start_XY), _phi0Value);

				//�N���\�C�h�Ȑ��𐶐�
				if (num < 1) {
					num = 1;
				}

				//�N���\�C�h�Ȑ��v�Z
				float clothoidLength = 0.0f;
				TArray<FVector>clothoidCurve = calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, circleEnd, circle.startCircle,clothoidLength);
				//(Clothoid_Debug_LOG,Error,TEXT("[%d] circle.startCircle is %s"),current,*circle.startCircle.ToString());
				
				calcCircleInfo(circle, clothoidCurve.Last());

				//target�̉~�ʍ��W�v�Z
				TArray<FVector>circleLocations2 = calcCircleLocations(circle.startAngle, circle.centerRotation.Yaw, circle.radius, clothoidCurve.Last(), circle.centerCircle, circle.sign);
				float circleLength2 = circleLocations2.Num()!=0 ? 
				circle.radius * abs(deltaFloat(circle.startAngle, circle.centerRotation.Yaw)): 0.0f;

				float curveHeight = targetLoc.Z - currentLoc.Z;
				curveLength = circleLength + clothoidLength + circleLength2;

				float height = circleLength * (1.0f / curveLength) * curveHeight;
				int i_height = 0;
				for (auto& x : circleLocations) {
					if (i_height == circleLocations.Num() - 1) {
						x.Z = height + currentLoc.Z;
						break;
					}
					float stepVal = height / circleLocations.Num();
					x.Z = stepVal * i_height + stepVal + currentLoc.Z;
					i_height++;
				}

				float height2 = clothoidLength * (1.0f / curveLength) * curveHeight;
				i_height = 0;
				for (auto& x : clothoidCurve) {
					if (i_height == clothoidCurve.Num() - 1) {
						if (circleLocations.Num() != 0) {
							x.Z = height2 + circleLocations.Last().Z;
						}
						else {
							x.Z = height2 + currentLoc.Z;
						}
						break;
					}
					float stepVal = height2 / clothoidCurve.Num();
					if(circleLocations.Num()!=0){
						x.Z = stepVal * i_height + stepVal + circleLocations.Last().Z;
					}else{
						x.Z = stepVal * i_height + stepVal + currentLoc.Z;
					}
					i_height++;
				}

				height = curveHeight - (height + height2);
				i_height = 0;
				for (auto& x : circleLocations2) {
					if(i_height == circleLocations2.Num()-1){
						x.Z = height + clothoidCurve.Last().Z;
						break;
					}
					float stepVal = height / circleLocations2.Num();
					x.Z = stepVal * i_height + stepVal + clothoidCurve.Last().Z;
					i_height++;
				}

				curve.Append(circleLocations);
				curve.Append(clothoidCurve);
				curve.Append(circleLocations2);
				
			}

		}else {//���̃p�X���Ȃ��ꍇ
		  //�Ώۂ̃p�X���W�ɐ���_�ɐݒ肷��

			if (pre_Circle.isPoint) {
				//�~�ʂ������̂Ō��݂̃p�X���W����Ώۂ̃p�X���W�ւ̊p�x�����̂܂ܐ���_�ɐݒ肷��
				FVector vec = FVector(targetLocXY,0) - FVector(currentLocXY,0);
				circle.centerRotation = UKismetMathLibrary::FindLookAtRotation(FVector(currentLocXY, 0), FVector(targetLocXY, 0));
				circle = FCircle(FVector(targetLocXY, 0), circle.centerRotation, circle.centerRotation.Yaw, circle.centerRotation.Yaw,
				0.0f);

				//�����擾
				float _hValue = UKismetMathLibrary::Distance2D(targetLocXY, currentLocXY);
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[current : %d] _hvalue : %f"), current, _hValue);

				int num = _hValue * ((float)accuracy / 100.0);

				FVector vec_tangent = targetLoc - currentLoc;
				float _phi0Value = pre_Circle.centerRotation.Yaw;
				//float _phi1Value = circle.startAngle;
				//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d]circle.center is %s ,circle.startCircle is %s"), current, *circle.center.ToString(), *circle.startCircle.ToString());
				
				//phi1Value��phi0Value�̊p�x�ɉ����Ē����Ŏ󂯎���悤�ɂ���
				float _phi1Value = circle.centerRotation.Yaw;

				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d ~ %d] _phi0Value is %f , _phi1Value is %f"), current, target, _phi0Value, _phi1Value);

				//����p�v�Zedr
				float _fov = deltaFloat(angleOf2DVector(currentLocXY, targetLocXY), _phi0Value);

				//�N���\�C�h�Ȑ��𐶐�
				if (num < 1) {
					num = 1;
				}

				//curve.Append(calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, currentLoc, circle.startCircle));
				float clothoidLength = 0.0f;
				curve.Append(calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, FVector(currentLocXY,0), circle.startCircle, clothoidLength));

				calcCircleInfo(circle, curve.Last());

				float curveHeight = targetLoc.Z - currentLoc.Z;

				int i_height = 0;
				for (auto& x : curve) {
					if (i_height == curve.Num() - 1) {
						x.Z = curveHeight + currentLoc.Z;
						break;
					}
					float stepVal = curveHeight / curve.Num();
					x.Z = stepVal * i_height + stepVal + currentLoc.Z;
					i_height++;
				}
				
			}else {
				
				//�O�̉~�ʌv�Z
				float turn_rate_endMax = 0.0f;
				if(r_temp>=0.1f){
					turn_rate_endMax = calc_TurnRate_Newton(-1 * pre_Circle.sign, pre_Circle, FVector(targetLocXY, 0));
					turn_rate_end = turn_rate_endMax * r_temp;
				}
				
				FRotator endRot = FRotator(0, pre_Circle.sign * turn_rate_end, 0);
				//�p�x
				pre_Circle.endAngle = UKismetMathLibrary::NormalizedDeltaRotator(pre_Circle.centerRotation, endRot).Yaw;
				//���W
				pre_Circle.endCircle = getAngleLocation(pre_Circle.endAngle, pre_Circle.radius, pre_Circle.center);
				//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d]pre_Circle.endAngle is %f, pre_Circle.endCircle is %s"),current,pre_Circle.endAngle,*pre_Circle.endCircle.ToString());

				TArray<FVector>circleLocations = calcCircleLocations(pre_Circle.centerRotation.Yaw, pre_Circle.endAngle, pre_Circle.radius, FVector(currentLocXY,0), pre_Circle.endCircle, pre_Circle.sign);
				float circleLength = circleLocations.Num()!=0 ? pre_Circle.radius * abs(deltaFloat(pre_Circle.centerRotation.Yaw, pre_Circle.endAngle)) : 0.0f;
				FVector circleEnd;
				if(circleLocations.Num() != 0){
					circleEnd = circleLocations.Last();
				}else{
					circleEnd = pre_Circle.endCircle;
				}

				circle.centerRotation = UKismetMathLibrary::FindLookAtRotation(circleEnd, FVector(targetLocXY, 0));

				//�ŏI�~�ʂ̊p�x��sign�����̂��̂܂܂̊p�x������
				circle = FCircle(FVector(targetLocXY,0), circle.centerRotation, circle.centerRotation.Yaw, circle.centerRotation.Yaw, 0.0f);

				//�����擾
				float _hValue = UKismetMathLibrary::Distance2D(targetLocXY,FVector2D(circleEnd.X,circleEnd.Y));
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[current : %d] _hvalue : %f"), current, _hValue);

				int num = _hValue * ((float)accuracy / 100.0);

				//�ŏI�~�ʊp�x��target�ւ̊p�x��phi1Value�ɑ��
				FVector vec = FVector(targetLocXY, 0) - circleEnd;
				float _phi0Value = getRotFromCircleTangent(pre_Circle.center, pre_Circle.endCircle, pre_Circle.sign).Yaw;
				float _phi1Value = circle.centerRotation.Yaw;

				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d ~ %d] _phi0Value is %f , _phi1Value is %f"), current, target, _phi0Value, _phi1Value);

				//����p�v�Z
				float _fov = deltaFloat(angleOf2DVector(FVector2D(circleEnd.X,circleEnd.Y), targetLocXY), _phi0Value);

				//�N���\�C�h�Ȑ��𐶐�
				if (num < 1) {
					num = 1;
				}

				calcCircleInfo(circle, circleEnd);
				
				float clothoidLength = 0.0f;
				TArray<FVector>clothoidCurve = calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, circleEnd, circle.startCircle, clothoidLength);
				
				//�����v�Z
				float curveHeight = targetLoc.Z - currentLoc.Z;

				curveLength = circleLength + clothoidLength;
				//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] circleLength is %f"), current, circleLength);
				float height = circleLength * (1.0f / curveLength) * curveHeight;

				int i_height = 0;
				for (auto& x : circleLocations) {
					if (i_height == circleLocations.Num() - 1) {
						x.Z = height + currentLoc.Z;
						break;
					}
					float stepVal = height / circleLocations.Num();
					x.Z = stepVal * i_height + stepVal + currentLoc.Z;
					i_height++;
				}

				height = curveHeight - height;

				i_height = 0;
				//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d] circleLocations is Num(%d) "),current,circleLocations.Num());
				for (auto& x : clothoidCurve) {
					if (i_height == clothoidCurve.Num() - 1) {
						//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] height + currentLoc.Z is %s "), current, *FVector(height + currentLoc.Z).ToString());
						if (circleLocations.Num() != 0) {
							
							x.Z = height + circleLocations.Last().Z;
							//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] height + circleLocations.Last().Z is %s "), current, *x.ToString());
						}else {
							x.Z = height + currentLoc.Z;
							//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] height + currentLoc.Z is %s !!"), current, *FVector(height + currentLoc.Z).ToString());
							//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] circleLocations is Num(%d) "), current, circleLocations.Num());
						}
						break;
					}
					float stepVal = height / clothoidCurve.Num();
					if(circleLocations.Num()!=0){
						x.Z = stepVal * i_height + stepVal + circleLocations.Last().Z;
					}else {
						x.Z = stepVal * i_height + stepVal + currentLoc.Z;
					}
					
					i_height++;
				}

				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] clothoidCurve.Last() is %s"), current, *clothoidCurve.Last().ToString());
				curve.Append(circleLocations);
				curve.Append(clothoidCurve);
			}
		}

		results.Append(curve);

		//���ʂ̍ŏI���W�����̃N���\�C�h�Ȑ��v�Z�̃X�^�[�g�n�_�ɐݒ�
		currentLoc = results.Last();
		currentLocXY = FVector2D(currentLoc.X, currentLoc.Y);

		if(check_loopCount == check_CircleIndex){
			
			test_Circle = circle;
		}
		//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("currentSpeed = %f"), currentSpeed);
		 
		pre_Circle = circle;

		curve.Empty();
		current++;
		target++;
		next++;
	}

	return results;
}

TArray<FVector> USkyPathComponent::calcClothoidSplineV2(UPARAM(ref)TArray<FVector>& pathDatas,bool useCustomRotation,FRotator customRotation)
{
	TArray<FVector>results;

	if (pathDatas.Num() <= 1) {
		UE_LOG(LogTemp, Error, TEXT("TArray.Num() is %d"), pathDatas.Num());
		return results;
	}

	//���ݑ��x�ݒ�
	float preSpeed = currentSpeed;

	if(useCustomRotation){
		preRotation = customRotation;
	}

	UE_LOG(Clothoid_Debug_LOG,Error,TEXT("preRotation is %f"),preRotation.Yaw);

	const int splineCount = pathDatas.Num() - 1;

	const float s = 0.5f;

	int current = 0;
	int target = 1;
	int next = 2;
	FVector currentLoc = pathDatas[current];
	FVector2D currentLocXY = FVector2D(currentLoc.X, currentLoc.Y);
	FVector targetLoc;
	FVector2D targetLocXY;

	/*
	if (!clothoidInitiallize) {
		if (GetOwner()) {
			//�f�o�b�O���[�h:�n�_�̃N���\�C�h�����p�x��C�ӂ̊p�x�ɐݒ肷��
			//�ʏ펞:���݂̋@�̂̊p�x���n�_�̏����p�x�ɐݒ�
			if (useDebugCurrentRotation) {
				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("currentRotation is %s"), *currentRotation.ToString());
				UE_LOG(Clothoid_Debug_LOG, Error, TEXT("debugMode is true"));
				pre_Rotation = currentRotation;
			}else {
				currentRotation = GetOwner()->GetActorRotation();
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("GetOwner() is Null"));
			return results;
		}
		clothoidInitiallize = true;
	}
	*/

	if(!clothoidInitiallize){
		results.Add(currentLoc);
		clothoidInitiallize = true;
	}
	
	for (; current < splineCount;) {
		check_loopCount = current;

		//�Ȑ���(L)
		float curveLength = 0;

		//�����p�x
		float turn_rate_start = 0;

		//�I�_�p�x
		float turn_rate_end = 0;

		//���̐���_�̍��W�ݒ�
		targetLoc = pathDatas[target];
		targetLocXY = FVector2D(targetLoc.X, targetLoc.Y);

		//����p�x
		float turnnningAngle = 1;

		//�N���\�C�h�Ȑ��z��
		TArray<FVector>curve;

		//���̃p�X������ꍇ
		if (pathDatas.IsValidIndex(next)) {
			//FVector2D nextLocXY = FVector2D(pathDatas[next].X, pathDatas[next].Y);
			//FVector nextLoc = pathDatas[next];

			const FRotator targetRot = UKismetMathLibrary::FindLookAtRotation(currentLoc, targetLoc);

			//FRotator nextRot= UKismetMathLibrary::FindLookAtRotation(targetLoc,nextLoc);
			//�����擾
			const float _hValue = UKismetMathLibrary::Distance2D(targetLocXY, currentLocXY);

			//0���Z�΍�̂��߂ɂP������
			const int num = _hValue * ((float)accuracy / 100.0) !=0.0f ? _hValue * ((float)accuracy / 100.0) : 1;

			const float _phi0Value = preRotation.Yaw;
			const float _phi1Value = angleOf2DVector(currentLocXY, targetLocXY);

			UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d ~ %d] _phi0Value is %f , _phi1Value is %f"), current, target, _phi0Value, _phi1Value);

			//����p�v�Z
			const float _fov = deltaFloat(_phi1Value, _phi0Value);

			

			//�N���\�C�h�Ȑ��𐶐�
			float clothoidLength = 0.0f;
			TArray<FVector> clothoidCurve = calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, FVector(currentLocXY, 0), FVector(targetLocXY, 0), clothoidLength);

			//�����v�Z�Ɠ����蔻��𗼕��s��
			//isNotObstale�g�p����

			const float curveHeight = targetLoc.Z - currentLoc.Z;

			const float stepS = 1.0f / num;
			for(int i = 0;i<num;i++){
				float S = stepS * i;
				clothoidCurve[i].Z = curveHeight*S + currentLoc.Z;
			}

			curve.Append(clothoidCurve);

			preControllP = true;
			preRotation = targetRot;

		}else {//���̃p�X���Ȃ��ꍇ
		 //�Ώۂ̃p�X���W�ɐ���_�ɐݒ肷��

			const FRotator targetRot = UKismetMathLibrary::FindLookAtRotation(currentLoc,targetLoc);

			//�����擾
			const float _hValue = UKismetMathLibrary::Distance2D(targetLocXY, currentLocXY);

			const int num = _hValue * ((float)accuracy / 100.0) >= 1.0f ? _hValue * ((float)accuracy / 100.0) : 1;

			const float _phi0Value = preRotation.Yaw;

			//phi1Value��phi0Value�̊p�x�ɉ����Ē����Ŏ󂯎���悤�ɂ���
			const float _phi1Value = targetRot.Yaw;

			UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d ~ %d] _phi0Value is %f , _phi1Value is %f"), current, target, _phi0Value, _phi1Value);

			//����p�v�Zedr
			const float _fov = deltaFloat(_phi1Value, _phi0Value);

			float clothoidLength = 0;
			TArray<FVector>clothoidCurve = calcClothoidCurve(num, _phi1Value, _phi0Value, _hValue, _fov, FVector(currentLocXY, 0), FVector(targetLocXY, 0), clothoidLength);

			const float curveHeight = targetLoc.Z - currentLoc.Z;

			const float stepS = 1.0f / num;

			for (int i = 0; i < num; i++) {
				float S = stepS * (i + 1);
				clothoidCurve[i].Z = curveHeight * S + currentLoc.Z;
			}

			//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] clothoidCurve.Last() is %s"), current, *clothoidCurve.Last().ToString());

			preControllP = true;

			curve.Append(clothoidCurve);

			preRotation = targetRot;

		}

		results.Append(curve);

		//���ʂ̍ŏI���W�����̃N���\�C�h�Ȑ��v�Z�̃X�^�[�g�n�_�ɐݒ�
		currentLoc = results.Last();
		currentLocXY = FVector2D(currentLoc.X, currentLoc.Y);

		//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("currentSpeed = %f"), currentSpeed);

		curve.Empty();
		current++;
		target++;
		next++;
	}

	return results;
}


TArray<FVector> USkyPathComponent::calcClothoidCurve(int n, float phi1, float phi0, float straightDis, float fov, FVector startLocation, FVector endLocation,float & clothoidLength) {

	int num_CalcClothoid = 50;
	float stepS = 1.0f / num_CalcClothoid;
	float iota = straightDis;
	TArray<FVector2D> psiPoints;
	TArray<FVector2D> psiCalcuPoints;

	TArray<FVector> results;

	FSlope slope;
	slope.phi0 = UKismetMathLibrary::DegreesToRadians(phi0);
	slope.phiV = 0;
	slope.phiU = 0;

	FPhiSlope phiSlope;

	float rad_phi1 = UKismetMathLibrary::DegreesToRadians(phi1);
	float rad_phi0 = UKismetMathLibrary::DegreesToRadians(phi0);
	float diff_phi = angle_diff(rad_phi1, rad_phi0);
	
	//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d]n = %d , diff_phi is %f, fov = %f"), check_loopCount, n, diff_phi, fov);
	phiSlope.phiV = UKismetMathLibrary::DegreesToRadians(getPhi_newtonMethod(n, diff_phi, fov));
	phiSlope.phiU = diff_phi - phiSlope.phiV;

	psiPoints.SetNum(num_CalcClothoid);

	complex<float> psiP_Vector;
	for (int i = 0; i < num_CalcClothoid; ++i) {
		float S = stepS * i;

		complex<float> r;
		phiSimpson_integral(phiSlope, S, S + stepS, &r);
		psiP_Vector += r;

		float l_x = static_cast<float>(psiP_Vector.real());
		float l_y = static_cast<float>(psiP_Vector.imag());

		psiPoints[i] = (1 * FVector2D(l_x, l_y));
		//psiPoints.Add(1 * FVector2D(l_x, l_y));
	}

	float lamda = UKismetMathLibrary::Distance2D(psiPoints.Last(), psiPoints[0]);

	float h = iota / lamda;

	//float h = iota;

	//UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d] : h = %f, iota = %f / lamda = %f"),check_loopCount,h,iota,lamda);

	FSlope cSlope;
	cSlope.phi0 = UKismetMathLibrary::DegreesToRadians(phi0);
	cSlope.phiV = phiSlope.phiV;
	cSlope.phiU = phiSlope.phiU;
	//UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] : phiSlope.phiV = %f, phiSlope.phiU = %f"), check_loopCount,phiSlope.phiV,phiSlope.phiU);

	stepS = 1.0f / n;

	//�N���\�C�h���
	complex<float> cP_Vector;
	results.SetNum(n);
	for (int i = 0; i < n; ++i) {
		float S = stepS * i;

		complex<float> r;
		simpson_integral(cSlope, S, S + stepS, &r);
		cP_Vector += r;

		float l_x = cP_Vector.real();
		float l_y = cP_Vector.imag();
		//cPoints.Add(h * FVector2D(l_x, l_y));
		FVector2D clothoidCurve = (h * FVector2D(l_x, l_y));

		results[i] = FVector(clothoidCurve.X + startLocation.X, clothoidCurve.Y + startLocation.Y,0.0f);
	}

	clothoidLength = h;

	return results;
}


void USkyPathComponent::simpson_integral(FSlope f, float a, float b, std::complex<float>* r) {
	float mul = (b - a) * static_cast<float>(1.0 / 6.0);
	*r = mul * (f(a) + static_cast<float>(4.0) * f((a + b) * static_cast<float>(0.5)) + f(b));
};

void USkyPathComponent::phiSimpson_integral(FPhiSlope f, float a, float b, std::complex<float>* r) {
	float mul = (b - a) * static_cast<float>(1.0 / 6.0);
	*r = mul * (f(a) + static_cast<float>(4.0) * f((a + b) * static_cast<float>(0.5)) + f(b));
};


double USkyPathComponent::fx(int n, float phi, float _x, float fov)
{
	float stepS = 1.0f / n;
	TArray<FVector2D> points;
	FPhiSlope pSlope;


	pSlope.phiV = UKismetMathLibrary::DegreesToRadians(_x);
	pSlope.phiU = phi - pSlope.phiV;

	//�^����ꂽ����p�ƈ�v���邩�v�Z

	complex<float> psiP_Vector;
	for (int i = 0; i < n; ++i) {
		float S = stepS * i;

		complex<float> r;
		
		phiSimpson_integral(pSlope, S, S + stepS, &r);
		psiP_Vector += r;
		

		float x = static_cast<float>(psiP_Vector.real());
		float y = static_cast<float>(psiP_Vector.imag());
		
		points.Add(1 * FVector2D(x, y));
		
	}
	
	// 2�_�̍��W���烉�W�A�������߂�
	double result = atan2(points.Last().Y - points[0].Y, points.Last().X - points[0].X);
	//UKismetMathLibrary::RadiansToDegrees(radian)

	//float temp_angle = atan2(points.Last().Y - points[0].Y, points.Last().X - points[0].X);
	result = UKismetMathLibrary::RadiansToDegrees(result);
	//result = angle_diff(result, UKismetMathLibrary::DegreesToRadians(fov));

	return deltaFloat(result,fov);
	
	//radian is ok
	//degree = UKismetMathLibrary::RadiansToDegrees(radian)
	//float diff = angle_diff(temp_angle, UKismetMathLibrary::DegreesToRadians(fov));
	
	///return UKismetMathLibrary::RadiansToDegrees(result);
}


float USkyPathComponent::calcTurnRadius(float turn_speed, float turnningPerformance, float scale)
{

	float angle = FMath::Clamp(turnningPerformance, 0.0f, 89.0f);
	//���x��kt(�m�b�g)����m/s�ɕύX
	double metorSpeed = turn_speed * 1.852 * 1000 / 3600;
	//�o���N�p�����W�A���ɕϊ�
	double radian_bankAngle = angle * UKismetMathLibrary::GetPI() / 180;

	//���񔼌a����(R=V^2/(g(9.8)*tan��))
	double result = pow(metorSpeed, 2) / (9.8 * tan(radian_bankAngle));

	//�l��scale�l�Ŕ�����
	return result / scale;
}

float USkyPathComponent::calc_CircleLength(FCircle &circle, float angle1, float angle2)
{
	if(circle.sign==1.0f){
		return circle.radius* abs(deltaFloat(angle1, angle2));
	}else if(circle.sign==-1.0f){
		return circle.radius * abs(combineFloat(angle1, angle2));
	}else{
		return 0.0f;
	}

}

double USkyPathComponent::angle_diff(double theta1, double theta2)
{
	double pi = UKismetMathLibrary::GetPI();
	double diff = theta1 - theta2; // �������v�Z

	while (diff > pi) diff -= 2 * pi; // �������΂��傫���ꍇ��2�΂����炷
	while (diff < -pi) diff += 2 * pi; // ������-�΂�菬�����ꍇ��2�΂����₷
	return diff; // ���K�����ꂽ������Ԃ�
}


void USkyPathComponent::calcCircleInfo(FCircle& circle, FVector targetLocation) {
	//targetlocation�Ƃ̐��l������circle�ɉ��Z���ďC������
	FVector2D tempTargetLoc2D = FVector2D(targetLocation.X,targetLocation.Y);
	FVector2D tempStartLoc2D = FVector2D(circle.startCircle.X,circle.startCircle.Y);
	FVector2D diffLocation2D = tempTargetLoc2D - tempStartLoc2D;

	circle.startCircle += FVector(diffLocation2D,0);
	circle.centerCircle += FVector(diffLocation2D, 0);
	circle.center += FVector(diffLocation2D, 0);
	circle.startCircle.Z = targetLocation.Z;
}

void USkyPathComponent::test_circleCheck(FCircle& circle) {
	if (check_loopCount == check_CircleIndex) {
		test_Circle = circle;
	}
}

float USkyPathComponent::get_clothoid_angle(float radius)
{
	double scale = 1; //�X�P�[�����q
	double L = scale * UKismetMathLibrary::Sqrt(radius);
	float angle = pow(L, 2) / (2 * pow(scale, 2));
	return angle;
}


float USkyPathComponent::calc_curve_angle(float length, float radius)
{
	//�l�����p

	float scale = 1;

	if (radius == 0.0f) {
		return 0.0f;
	}

	return (1 / radius) * length * scale;
}

FVector USkyPathComponent::calcMaxCircleAngle(FCircle& circle, FCircle& circle2)
{
	FVector center = circle.center;
	FVector center2 = circle2.center;
	FRotator center_to_center2 = UKismetMathLibrary::FindLookAtRotation(center, center2);
	FRotator circleAngle = FRotator(0, circle2.sign * 90, 0);
	FRotator maxRot = UKismetMathLibrary::ComposeRotators(center_to_center2, circleAngle);
	return circle2.center + UKismetMathLibrary::GetForwardVector(maxRot) * circle2.radius;
}

float USkyPathComponent::getPhi_newtonMethod(float n, float _phi, float fov) {
	float error = 0.01;
	float delta = 0.001;
	float x1 = 0;
	int k = 0;
	int countMax = 30;


	
	while (true) {
		float x2 = x1 + delta;
		float y1 = fx(n, _phi, x1, fov);
		float y2 = fx(n, _phi, x2, fov);
		if (abs(x2 - x1) == 0) {
			break;
		}
		float diff_x = (y2 - y1) / (x2 - x1);
		float next_x = x1 - y1 / diff_x;

		if (std::fabs(y1 - 0) < error || k >= countMax) {
			UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] getaPhiNewton is success!!"), check_loopCount);
			break;
		}
		x1 = next_x;
		k += 1;
	}

	if (k >= countMax) {
		UE_LOG(LogTemp, Error, TEXT("Failed to converge!!"));
	}
	
	UE_LOG(Clothoid_Debug_LOG, Error, TEXT("(result : %f,n : %f,phi : %f,fov : %f),count %d"), x1, n, UKismetMathLibrary::RadiansToDegrees(_phi), fov, k);

	return x1;
}


float USkyPathComponent::getThreePointAngle(FVector p1, FVector p2, FVector p3)
{
	FVector vec1 = p1 - p2;
	FVector vec2 = p2 - p3;
	float dotNum = UKismetMathLibrary::Dot_VectorVector(vec1, vec2);
	FVector crossNum = UKismetMathLibrary::Cross_VectorVector(vec1, vec2);

	return UKismetMathLibrary::DegAtan2(crossNum.Size(), dotNum)
		* UKismetMathLibrary::SignOfFloat(crossNum.Z);
}


float USkyPathComponent::getMiddleAngle(float angle1, float angle2)
{
	float angle1R = UKismetMathLibrary::DegreesToRadians(angle1);
	float angle2R = UKismetMathLibrary::DegreesToRadians(angle2);

	float x = (UKismetMathLibrary::Cos(angle1R) + UKismetMathLibrary::Cos(angle2R)) / 2;
	float y = (UKismetMathLibrary::Sin(angle1R) + UKismetMathLibrary::Sin(angle2R)) / 2;

	return UKismetMathLibrary::DegAtan2(y, x);
}

float USkyPathComponent::getCenterAngle(float angle1, float angle2)
{
	float result = 0.0f;
	float angle360_1 = angle1;
	float angle360_2 = angle2;
	if(angle360_1<0.0f){
		angle360_1+=360.0f;
	}
	if(angle360_2<0.0f){
		angle360_2+=360.0f;
	}

	result = ((angle360_1 + angle360_2) / 2.0f);
	bool inRange = UKismetMathLibrary::InRange_FloatFloat(result,-180,180,false,false);
	if(!inRange){result-=360.0f;}

	//���ʂ��͈͓��̊p�x���o���Ă��邩�`�F�b�N
	float value = result;
	if(abs(angle360_1-angle360_2)<=180){
		if(value < 0.0f){
			value+=360;
		}
	}else{
		if(angle360_1>180.0f){
			angle360_1-=360.0f;
		}
		if(angle360_2>180.0f){
			angle360_2-=360.0f;
		}
		if(value>0.0f){
			value-=360.0f;
		}
	}

	float min = UKismetMathLibrary::Min(angle360_1, angle360_2);
	float max = UKismetMathLibrary::Max(angle360_1, angle360_2);

	if (!UKismetMathLibrary::InRange_FloatFloat(value, min, max, true, true)){
		result -=180;
	}

	return result;
}

float USkyPathComponent::getMiddleAngle_gimbal(float angle1, float angle2, bool sign)
{
	float value = (angle1 - angle2) / 2;

	return sign ? combineFloat(angle1, value) : deltaFloat(angle1, value);
}

FVector USkyPathComponent::calcStartLocationOnArc(FVector current, float arc_radius, FVector center, float sign)
{

	FVector vec = center - current;
	float rad = atan2(vec.Y, vec.X);
	float dis = vec.Size();
	float _theta = UKismetMathLibrary::Asin(arc_radius / dis) * sign + rad;
	float _x = arc_radius * cos(_theta);
	float _y = arc_radius * sin(_theta);

	return FVector(_x + current.X, _y + current.Y, center.Z);
}

float USkyPathComponent::convert_Angle_to_controllAngle(float angle)
{
	if (UKismetMathLibrary::InRange_FloatFloat(angle, -180, -90, true, true)) {
		return UKismetMathLibrary::Abs(angle) - 180;
	}
	if (UKismetMathLibrary::InRange_FloatFloat(angle, -90, 0, false, false)) {
		return UKismetMathLibrary::Abs(angle) - 180;
	}
	if (UKismetMathLibrary::InRange_FloatFloat(angle, 0, 180, true, true)) {
		return 180 - angle;
	}
	return 0.0f;
}


float USkyPathComponent::calculateArcLength(float radius, float startAngle, float endAngle)
{
	return radius * (UKismetMathLibrary::DegreesToRadians(endAngle) - UKismetMathLibrary::DegreesToRadians(startAngle));
}

TArray<FVector> USkyPathComponent::calcCircleLocations(float startAngle, float endAngle, float radius, FVector curve_start, FVector curve_end, float sign)
{
	TArray<FVector>results;

	 float angleLength = deltaFloat(startAngle, endAngle);
	UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] angleLength = %f,startAngle = %f ,endAngle = %f"), check_loopCount, angleLength, startAngle, endAngle);

	int num = UKismetMathLibrary::Abs(angleLength) * ((float)arc_accuracy / 100.0);
	if (num < 1) {
		return results;
	}

	float step = 1.0f / num;

	FVector center = FVector::ZeroVector;
	FVector startAngleLoc = getAngleLocation(startAngle, radius, center);
	if (sign == 1) {
		angleLength = -std::abs(angleLength);
	}else {
		angleLength = std::abs(angleLength);
	}

	results.SetNum(num-1);
	for (int i = 0; i < num-1; i++) {
		float s = (i + 1) * step;
		float arc_point_angle = UKismetMathLibrary::ComposeRotators(FRotator(0, startAngle, 0), FRotator(0, angleLength * s, 0)).Yaw;
		FVector arcPoint = getAngleLocation(arc_point_angle, radius, center);
		results[i] = FVector(curve_start.X + arcPoint.X - startAngleLoc.X, curve_start.Y + arcPoint.Y - startAngleLoc.Y, curve_start.Z);
	}

	return results;
}

FVector USkyPathComponent::getAngleLocation(float angle, float radius, FVector centerLocation)
{
	return centerLocation + FVector(
		radius * UKismetMathLibrary::DegCos(angle),
		radius * UKismetMathLibrary::DegSin(angle),
		0.0f
	);
}

FVector USkyPathComponent::getAngleLocationFromThreepoint(float thetaMax, FVector p1, FVector p2, FVector p3, float radius)
{

	float r1 = UKismetMathLibrary::FindLookAtRotation(p2, p1).Yaw;
	float r2 = UKismetMathLibrary::FindLookAtRotation(p2, p3).Yaw;
	FRotator angle_Middle = FRotator(0, getMiddleAngle(r1, r2), 0);

	FRotator composeRot = UKismetMathLibrary::ComposeRotators(angle_Middle, FRotator(0, 180, 0));
	float angle_threePoint = getThreePointAngle(p1, p2, p3);
	FRotator deltaRot = FRotator(0, UKismetMathLibrary::SignOfFloat(angle_threePoint) * turn_rate, 0);

	float angle_circle = UKismetMathLibrary::NormalizedDeltaRotator(composeRot, deltaRot).Yaw;
	FVector center = p2 + UKismetMathLibrary::GetForwardVector(angle_Middle) * radius;

	return getAngleLocation(angle_circle, radius, center);
}


float USkyPathComponent::angleOf2DVector(FVector2D p1, FVector2D p2)
{
	// 2�_�̍��W���烉�W�A�������߂�
	double radian = (atan2(p2.Y - p1.Y, p2.X - p1.X));

	// ���W�A������x�����߂�
	return  UKismetMathLibrary::RadiansToDegrees(radian);
}


float USkyPathComponent::calc_TurnRate_Newton(float _sign, FCircle &circle, FVector _target)
{
	float error = 1.0;
	float delta = 1;
	float x1 = 0;
	int k = 0;
	int countMax = 30;
	float _centerAngle = circle.centerRotation.Yaw;
	float _radius = circle.radius;
	FVector _center = circle.center;

	while (true) {
		float x2 = x1 + delta;
		float y1 = f_TurnRate(x1, _sign, _centerAngle, _radius, _center, _target);
		
		float y2 = f_TurnRate(x2, _sign, _centerAngle, _radius, _center, _target);
		if((x2 - x1)==0|| (y2-y1)==0||(x1-y1)==0){
			return 0.0f;
		}
		float diff_x = (y2 - y1) / (x2 - x1);

		float next_x = x1 - y1 / diff_x;

		if (std::fabs(y1 - 0) < error || k >= countMax) {
			break;
		}
		x1 = next_x;
		k += 1;
	}

	if (k >= countMax) {
		UE_LOG(LogTemp, Error, TEXT("Failed to converge!!"));
	}

	return x1;
}

float USkyPathComponent::calc_TurnRate_Newton2(FCircle& circle, FCircle& circle2)
{

	float error = 1.0;
	float delta = 1;
	float x1 = 0;
	int k = 0;
	int countMax = 30;
	float _sign = circle.sign * -1;
	float _centerAngle = circle.centerRotation.Yaw;
	float _radius = circle.radius;
	FVector _center = circle.center;

	while (true) {

		float x2 = x1 + delta;

		float angle = combineFloat(_centerAngle, x1 * _sign);
		FVector angleLoc = getAngleLocation(angle, _radius, _center);
		float targetAngle = combineFloat(angle, circle2.sign * 180);
		FVector _target = getAngleLocation(targetAngle, circle2.radius, circle2.center);

		float y1 = f_TurnRate(x1, _sign, _centerAngle, _radius, _center, _target);

		angle = combineFloat(_centerAngle, x2 * _sign);
		angleLoc = getAngleLocation(angle, _radius, _center);
		targetAngle = combineFloat(angle, circle2.sign * 180);
		_target = getAngleLocation(targetAngle, circle2.radius, circle2.center);

		float y2 = f_TurnRate(x2, _sign, _centerAngle, _radius, _center, _target);
		float diff_x = (y2 - y1) / (x2 - x1);
		float next_x = x1 - y1 / diff_x;

		if (std::fabs(y1 - 0) < error || k >= countMax) {
			break;
		}
		x1 = next_x;
		k += 1;
	}

	if (k >= countMax) {
		UE_LOG(LogTemp, Error, TEXT("Failed to converge!!"));
	}

	return x1;
}

float USkyPathComponent::f_TurnRate(float value, float _sign, float _centerAngle, float _radius, FVector _center, FVector _target) {
	float combineAngle = combineFloat(_centerAngle, value * _sign);

	FVector angleLoc = getAngleLocation(combineAngle, _radius, _center);

	//�~�ʏ�̊p�x�̐ړ_����Ώۂ̃p�X�܂ł̊p�x�v�Z
	float angle_To_Target = UKismetMathLibrary::FindLookAtRotation(angleLoc, _target).Yaw;

	//�~�ʏ�̐ڐ��̊p�x�v�Z
	float angle = UKismetMathLibrary::FindLookAtRotation(_center, angleLoc).Yaw;
	float angleOnCircle = combineFloat(angle, _sign * 90);

	return deltaFloat(angle_To_Target, angleOnCircle);
}

FRotator USkyPathComponent::getRotFromCircleTangent(FVector center, FVector circleTangent, int sign) {

	FRotator rot = UKismetMathLibrary::FindLookAtRotation(center, circleTangent);
	rot = UKismetMathLibrary::ComposeRotators(rot, FRotator(0, 90 * sign, 0));
	return rot;
}

float USkyPathComponent::getCircleDirection(FCircle& circle,FVector targetLoc)
{
	float tempSign = deltaFloat(circle.centerRotation.Yaw, circle.startAngle);
	if (abs(tempSign) == 0.0f) {
		UE_LOG(Clothoid_Debug_LOG,Error,TEXT("[%d] tempSign is zero"),check_loopCount);
		//float yaw = UKismetMathLibrary::FindLookAtRotation(targetLoc,circle.center).Yaw;
		float yaw = UKismetMathLibrary::FindLookAtRotation(circle.center, targetLoc).Yaw;
		tempSign = deltaFloat(circle.centerRotation.Yaw,yaw);
		UE_LOG(Clothoid_Debug_LOG, Error, TEXT("[%d] tempSign is %f"),check_loopCount,tempSign);
	}
	return tempSign >= 0.0f ? 1.0f : -1.0f;
}

float USkyPathComponent::combineFloat(float input, float combineValue)
{
	FRotator rot1 = FRotator(0, input, 0);
	FRotator rot2 = FRotator(0, combineValue, 0);
	return UKismetMathLibrary::ComposeRotators(rot1, rot2).Yaw;
}

float USkyPathComponent::deltaFloat(float input, float deltaValue)
{
	FRotator rot1 = FRotator(0, input, 0);
	FRotator rot2 = FRotator(0, deltaValue, 0);
	return UKismetMathLibrary::NormalizedDeltaRotator(rot1, rot2).Yaw;
}