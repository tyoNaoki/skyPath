// Fill out your copyright notice in the Description page of Project Settings.


#include "SVO_SaveGame.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SVO_Volume.h"
#include "SVOSystemLibrary.h"

USVO_SaveGame::USVO_SaveGame() {

}

int32 USVO_SaveGame::incMortonCodeX32(int32 code) {
	int32 mask = 0x49249249 & 0x3FFFFFFF;
	return (((code & mask) + (1 | ~mask)) & mask) | (code & ~mask);
}

int32 USVO_SaveGame::incMortonCodeY32(int32 code) {
	int32 mask = (0x49249249 << 1) & 0x3FFFFFFF;
	return (((code & mask) + (2 | ~mask)) & mask) | (code & ~mask);
}

int32 USVO_SaveGame::incMortonCodeZ32(int32 code) {
	int32 mask = (0x49249249 << 2) & 0x3FFFFFFF;
	return (((code & mask) + (4 | ~mask)) & mask) | (code & ~mask);
}

int32 USVO_SaveGame::decMortonCodeX32(int32 code) {
	int32 mask = 0x49249249 & 0x3FFFFFFF;
	return (((code & mask) + (-1 | ~mask)) & mask) | (code & ~mask);
}

int32 USVO_SaveGame::decMortonCodeY32(int32 code)
{
	int32 mask = (0x49249249 << 1) & 0x3FFFFFFF;
	return (((code & mask) + (-2 | ~mask)) & mask) | (code & ~mask);
}

int32 USVO_SaveGame::decMortonCodeZ32(int32 code)
{
	int32 mask = (0x49249249 << 2) & 0x3FFFFFFF;
	return (((code & mask) + (-4 | ~mask)) & mask) | (code & ~mask);
}

int32 USVO_SaveGame::addMortonCode32(int32 code, int32 add)
{
	int32 maskX = 0x49249249 & 0x3FFFFFFF;
	int32 maskY = (0x49249249 << 1) & 0x3FFFFFFF;
	int32 maskZ = (0x49249249 << 2) & 0x3FFFFFFF;
	return (((code & maskX) + (add | ~maskX)) & maskX) |
		(((code & maskY) + (add | ~maskY)) & maskY) |
		(((code & maskZ) + (add | ~maskZ)) & maskZ);
}

int32 USVO_SaveGame::BitSeparateFor3D(int32 n)
{
	int32 s = n;
	s = (s | s << 8) & 0x0000f00f;
	s = (s | s << 4) & 0x000c30c3;
	s = (s | s << 2) & 0x00249249;
	return s;
}

int32 USVO_SaveGame::Get3DMortonNumber(int32 x, int32 y, int32 z)
{
	return BitSeparateFor3D(x) | BitSeparateFor3D(y) << 1 | BitSeparateFor3D(z) << 2;
}

int32 USVO_SaveGame::GetPointElem(FVector& p)
{
	//unit->units[uiLevel]
	return Get3DMortonNumber(
		(int32)((p.X - rgnMin.X) / (units[uiLevel].X)),
		(int32)((p.Y - rgnMin.Y) / (units[uiLevel].Y)),
		(int32)((p.Z - rgnMin.Z) / (units[uiLevel].Z))
	);
}

void USVO_SaveGame::fromMortonCode32(int32 code, float& xRet, float& yRet, float& zRet) {
	uint32 x = code, y = code >> 1, z = code >> 2;

	x &= 0x49249249; x |= x >> 2; x &= 0xC30C30C3; x |= x >> 4;  x &= 0x0F00F00F; x |= x >> 8; x &= 0xFF0000FF; x |= x >> 16;

	y &= 0x49249249; y |= y >> 2; y &= 0xC30C30C3; y |= y >> 4;  y &= 0x0F00F00F; y |= y >> 8; y &= 0xFF0000FF; y |= y >> 16;

	z &= 0x49249249; z |= z >> 2; z &= 0xC30C30C3; z |= z >> 4;  z &= 0x0F00F00F; z |= z >> 8; z &= 0xFF0000FF; z |= z >> 16;

	xRet = x & 0x3FF; yRet = y & 0x3FF; zRet = z & 0x3FF;
}

void USVO_SaveGame::fromMortonCode32(int32 code, FVector& TargetVector) {
	int32 x = code, y = code >> 1, z = code >> 2;

	x &= 0x49249249; x |= x >> 2; x &= 0xC30C30C3; x |= x >> 4;  x &= 0x0F00F00F; x |= x >> 8; x &= 0xFF0000FF; x |= x >> 16;

	y &= 0x49249249; y |= y >> 2; y &= 0xC30C30C3; y |= y >> 4;  y &= 0x0F00F00F; y |= y >> 8; y &= 0xFF0000FF; y |= y >> 16;

	z &= 0x49249249; z |= z >> 2; z &= 0xC30C30C3; z |= z >> 4;  z &= 0x0F00F00F; z |= z >> 8; z &= 0xFF0000FF; z |= z >> 16;

	TargetVector.X = x & 0x3FF; TargetVector.Y = y & 0x3FF; TargetVector.Z = z & 0x3FF;
}

int32 USVO_SaveGame::GetLocalMortonCode(int32 globalMortonCode, int32 level) {
	if (level == 0) {
		return globalMortonCode;
	}

	//1,9,73 ::72
	return globalMortonCode - totalOfVoxels[level - 1];
}

int32 USVO_SaveGame::GetGlovalMortonCode(int32 localMortonCode, int level) {
	if (level == 0) {
		return localMortonCode;
	}

	return localMortonCode + totalOfVoxels[level - 1];
}


bool USVO_SaveGame::GenerateFrameworkOfSVO(ASVO_Volume* svo_Volume, int Level)
{
	if (!svo_Volume) {
		UE_LOG(LogTemp, Error, TEXT("svoGenerater is NULL!!"));
		return false;
	}

	if (!svo_Volume->GetWorld()) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("GetWorld() Is NULL!!"));
		UE_LOG(LogTemp, Error, TEXT("GetWorld() Is NULL!!"));
		return false;
	}

	this->_svo_Volume = svo_Volume;

	iPow[0] = 1;
	totalOfVoxels[0] = 1;

	int32_t totaltemp = 1;
	for (int i = 1; i < MAX_INDEX + 1; i++) {
		iPow[i] = iPow[i - 1] * 8;
		totaltemp += iPow[i];
		totalOfVoxels[i] = totaltemp;
	}

	//レベル制限(6)
	uiLevel = Level > 6 ? 6 : Level;

	//Level(0基点)の配列作成
	dwCellNum = (iPow[uiLevel + 1] - 1) / 7;
	FVector mHalfExtent = _svo_Volume->GetBounds().BoxExtent;
	FVector center = _svo_Volume->GetActorLocation();

	rgnMax = center + _svo_Volume->GetActorForwardVector() * mHalfExtent.X + \
		_svo_Volume->GetActorRightVector() * mHalfExtent.Y + \
		_svo_Volume->GetActorUpVector() * mHalfExtent.Z;
	rgnMin = center - _svo_Volume->GetActorForwardVector() * mHalfExtent.X - \
		_svo_Volume->GetActorRightVector() * mHalfExtent.Y - \
		_svo_Volume->GetActorUpVector() * mHalfExtent.Z;
	unit_W = mHalfExtent * 2;

	float temp((float)(1 << uiLevel));
	FVector unit = FVector(unit_W / temp);

	for (int i = uiLevel; i >= 0; i--) {
		units[i] = unit;
		unit = unit * 2;
	}

	ClearObstacle();
	svo_Nodes.Empty();
	bricks.Empty();

	isDoneGenerateFramework = true;
	isGenerate = false;
	return true;
}

bool USVO_SaveGame::IsNotObstacle(int32 glovalMortonCode)
{
	//mortonCodeがエラー数字の場合は範囲外なので障害物なしで返す。
	if (glovalMortonCode >= totalOfVoxels[6] || glovalMortonCode < 9) {
		return true;
	}

	int nodeIndex = (glovalMortonCode - 9) >> 6;
	int temp = glovalMortonCode;
	temp -= 9;
	temp = temp & 0x3f;
	return!((bricks[nodeIndex] & ((uint64)1 << temp)) != 0);
}


bool USVO_SaveGame::RegistAllObstacles(UObject* WorldContextObject, ETraceTypeQuery traceType, const TArray<AActor*>& ActorsToIgnore) {
	if (uiLevel < 2 || !isDoneGenerateFramework) {
		UE_LOG(LogTemp, Error, TEXT("RegistLevel is less than the 2 level or IsDoneGenerateFramework is false!!"));
		return false;
	}

	FRotator orientation = FRotator::ZeroRotator;
	FHitResult HitCall(ForceInit);
	TArray<AActor*>actorsToIgnore;

	UWorld* world = _svo_Volume->GetWorld();
	if (!world) {
		UE_LOG(LogTemp, Error, TEXT("world is Null!!"));
		return false;
	}

	/** 障害物判定 **/
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {
		EObjectTypeQuery::ObjectTypeQuery1,	//! WorldStatic
	};
	TArray<AActor*> OverlapActors;

	for (int i = 0; i < iPow[uiLevel]; i++) {
		int32 detectNum = totalOfVoxels[uiLevel - 1] + i;
		FVector traceLocation = GetLocationFromMortonCode(detectNum, uiLevel);

		//UE4.24で使用	
		/*
		if (UKismetSystemLibrary::BoxTraceSingle(
			world,
			traceLocation,
			traceLocation,
			GetExtentOfVoxel(detectNum, level),
			orientation,
			traceType,
			false,
			actorsToIgnore,
			EDrawDebugTrace::None,
			HitCall,
			true,
			FLinearColor::Red,
			FLinearColor::Green,
			1.0)) {
			if (HitCall.bBlockingHit) {
				RegistObstacleFromNumber(detectNum);
			}
		}
		*/

		//UE4.22で使用	
		if (USVOSystemLibrary::BoxOverlapActors(
			WorldContextObject,
			traceLocation,
			GetExtentOfVoxel(detectNum, uiLevel),
			orientation,
			ObjectTypes,
			nullptr,
			ActorsToIgnore,
			OverlapActors
		)) {
			RegistObstacleFromNumber(detectNum);
		}

	}
	return true;
}

void USVO_SaveGame::RegistObstacleFromLocation(FVector obstacleLocation, FVector obstacleScale) {
	FVector tempMin = obstacleLocation - obstacleScale;
	FVector tempMax = obstacleLocation + obstacleScale;

	int32 tempRegistMortonCode = GetMortonNumber(tempMin, tempMax);

	if (Obstacles.Contains(tempRegistMortonCode)) {
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RegistMortonCode : %d"), tempRegistMortonCode);

	Obstacles.Push(tempRegistMortonCode);
}

bool USVO_SaveGame::RegistObstacleFromNumber(int32 glovalMortonCode) {
	if (Obstacles.Contains(glovalMortonCode)) {
		UE_LOG(LogTemp, Error, TEXT("%d : It's already regist Number"), glovalMortonCode);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("%d : It's regist successd!!"), glovalMortonCode);

	Obstacles.Push(glovalMortonCode);
	return true;
}

void USVO_SaveGame::DrawVoxelFromNumber(int glovalNumber, FColor color, float duration, float thickness) {
	if (!_svo_Volume) {
		UE_LOG(LogTemp, Error, TEXT("svoGenerater is Null!!"));
		return;
	}

	int level = GetLevel(glovalNumber);
	UKismetSystemLibrary::DrawDebugBox(_svo_Volume->GetWorld(), GetLocationFromMortonCode(glovalNumber, level), GetExtentOfVoxel(glovalNumber, level), color, FRotator(0, 0, 0), duration, thickness);
}


void USVO_SaveGame::DrawObstacleVoxels(FColor color, float duration, float thickness) {
	FRotator rotation = FRotator(0, 0, 0);

	if (Obstacles.Num() == 0) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Obstacle.Num() return 0"));
		UE_LOG(LogTemp, Warning, TEXT("Obstacle.Num() return 0"));
		return;
	}

	if (!_svo_Volume) {
		UE_LOG(LogTemp, Error, TEXT("svoGenerater Is NULL!!"));
		return;
	}

	UWorld* world = _svo_Volume->GetWorld();
	int level = GetLevel(*Obstacles.begin());
	FVector obstacleExtent = GetExtentOfVoxel(*Obstacles.begin(), level);
	for (auto& x : Obstacles) {
		//UE_LOG(LogTemp, Log, TEXT("%d : Draw"),x);
		UKismetSystemLibrary::DrawDebugBox(world, GetLocationFromMortonCode(x, level), obstacleExtent, color, rotation, duration, thickness);
	}
}

void USVO_SaveGame::DrawNodes(FColor color, float duration, float thickness) {
	FRotator rotation = FRotator(0, 0, 0);
	if (!isGenerate) {
		UE_LOG(LogTemp, Error, TEXT("isGenerate Is false!!"));
		return;
	}

	if (!_svo_Volume) {
		UE_LOG(LogTemp, Error, TEXT("svoGenerater Is NULL!!"));
		return;
	}

	UWorld* world = _svo_Volume->GetWorld();
	TArray<int32>tempNodes;
	svo_Nodes.GenerateKeyArray(tempNodes);
	for (auto& x : tempNodes) {
		UKismetSystemLibrary::DrawDebugBox(world, GetLocationFromMortonCode(x, GetLevel(x)), GetExtentOfVoxel(x, GetLevel(x)), color, rotation, duration, thickness);
	}
}

void USVO_SaveGame::DrawAdjacentNodes(int32 mortonCode, FColor color, float duration, float thickness) {

	if (!isGenerate) {
		UE_LOG(LogTemp, Error, TEXT("isGenerate Is false!!"));
		return;
	}

	if (!svo_Nodes.Contains(mortonCode)) {
		UE_LOG(LogTemp, Error, TEXT("%d Is Not Contain!!"), mortonCode);
		return;
	}

	if (!_svo_Volume) {
		UE_LOG(LogTemp, Error, TEXT("svoGenerater Is NULL!!"));
		return;
	}

	FRotator rotation = FRotator(0, 0, 0);

	UWorld* world = _svo_Volume->GetWorld();

	TArray<FFaceInfo>& adNodes = svo_Nodes.Find(mortonCode)->adjacentFaces;
	UE_LOG(LogTemp, Error, TEXT("adNodes Num is %d"), adNodes.Num());

	if (adNodes.Num() <= 0) {
		UE_LOG(LogTemp, Error, TEXT("%d don't have never adjacentNode!!"));
		return;
	}


	for (auto& x : adNodes) {
		UKismetSystemLibrary::DrawDebugBox(world, GetLocationFromMortonCode(x.mortonCode, GetLevel(x.mortonCode)), GetExtentOfVoxel(x.mortonCode, GetLevel(x.mortonCode)), color, rotation, duration, thickness);
		UE_LOG(LogTemp, Error, TEXT("Draw AdjacentNode ( %d ) ,orginalNode ( %d )"), x.mortonCode, mortonCode);
	}
}

void USVO_SaveGame::DrawFaces(int32 mortonCode, float size, FColor color, float duration) {

	if (!isGenerate) {
		UE_LOG(LogTemp, Error, TEXT("isGenerate Is false!!"));
		return;
	}

	if (!svo_Nodes.Contains(mortonCode)) {
		UE_LOG(LogTemp, Error, TEXT("%d Is Not Contain!!"), mortonCode);
		return;
	}

	if (!_svo_Volume) {
		UE_LOG(LogTemp, Error, TEXT("svoGenerater Is NULL!!"));
		return;
	}

	FRotator rotation = FRotator(0, 0, 0);

	UWorld* world = _svo_Volume->GetWorld();

	TArray<FFaceInfo> nodeFaces = svo_Nodes.Find(mortonCode)->adjacentFaces;
	//TArray<int32>&adNode = tempAdjacentNodes->adjacentNode;
	if (nodeFaces.Num() <= 0) {
		UE_LOG(LogTemp, Error, TEXT("%d don't have never adjacentNode!!"));
		return;
	}

	for (auto& x : nodeFaces) {
		UKismetSystemLibrary::DrawDebugPoint(world, x.center, size, color, duration);
		UE_LOG(LogTemp, Error, TEXT("%d's face location is %s"), x.mortonCode, *x.center.ToString());
	}
}

void USVO_SaveGame::ClearObstacle() {
	Obstacles.Empty();
}


bool USVO_SaveGame::GenerateSparseVoxelOctree() {
	if (!isDoneGenerateFramework) {
		UE_LOG(LogTemp, Error, TEXT("Error : IsDoneGenerateFramwark is false !!"));
		return false;
	}

	GenerateBricks();

	GenerateNodes();

	UE_LOG(LogTemp, Log, TEXT("AGenerateSVO can generated"));

	isGenerate = true;

	return true;
}


int USVO_SaveGame::GetLevel(int32 glovalMortonCode)
{
	if (glovalMortonCode == 0) {
		return 0;
	}

	for (int i = 1; i <= MAX_INDEX; i++) {

		if (totalOfVoxels[i] > glovalMortonCode) {
			return i;
		}
	}

	return -1;
}


int32 USVO_SaveGame::GetMortonNumber(FVector Min, FVector Max)
{
	// 最小レベルにおける各軸位置を算出
	int32 LT = GetPointElem(Min);
	int32 RB = GetPointElem(Max);

	// 空間番号を引き算して
	// 最上位区切りから所属レベルを算出
	int32 Def = RB ^ LT;

	unsigned int HiLevel = 0;
	unsigned int i;

	for (i = 0; i < uiLevel; i++)
	{
		int32 Check = (Def >> (i * 3)) & 0x7;
		if (Check != 0)
			HiLevel = i + 1;
	}

	int32 SpaceNum = RB >> (HiLevel * 3);
	//UE_LOG(LogTemp, Error, TEXT("SpacenNum : %d"), SpaceNum);

	int32 AddNum = (iPow[uiLevel - HiLevel] - 1) / 7;
	//UE_LOG(LogTemp, Error, TEXT("AddNum : %d"), AddNum);

	SpaceNum += AddNum;

	if (SpaceNum > dwCellNum) {
		return 0xffffffff;
	}

	return SpaceNum;
}

int32 USVO_SaveGame::GetMortonNumberToCharacter(FVector center, FVector side) {
	if (!isGenerate) {
		UE_LOG(LogTemp,Error,TEXT("saveGame's isGenerate is false!!"));
		return -1;
	}
	
	int32 mortonCode = GetMortonNumberFromPoint(center);

	UE_LOG(LogTemp, Error, TEXT("GetMortonNumberFromPoint is %d"),mortonCode);

	float bestLength = side.X;
	if (bestLength < side.Y) {
		bestLength = side.Y;
	}
	if (bestLength < side.Z) {
		bestLength = side.Z;
	}

	int templowestlevel = -1;
	for (int i = uiLevel; i >= 2; i--) {
		if (units[i].X >= bestLength) {
			templowestlevel = i;
			break;
		}
	}

	if (templowestlevel == -1) { return -1; }

	for (int i = GetLevel(mortonCode); i > templowestlevel; i--) {
		mortonCode = GetParentMortonNumber(mortonCode);
	}

	return mortonCode;
}

int32 USVO_SaveGame::GetMortonNumberFromPoint(FVector targetLocation)
{
	int32 SpaceNum = GetPointElem(targetLocation);
	int32 AddNum = (iPow[uiLevel] - 1) / 7;
	SpaceNum += AddNum;

	if (SpaceNum > dwCellNum)
		return 0xffffffff;

	return SpaceNum;
}

FAdjacentNodes& USVO_SaveGame::GetAdjacentNodes(int32 mortonCode)
{
	return svo_Nodes.FindOrAdd(mortonCode);
}

TArray<int32> USVO_SaveGame::GetNearEmptyVoxelNumbers(int32 glovalMortonCode)
{
	TArray<int32>results;
	int targetLevel = GetLevel(glovalMortonCode);
	int32 localTargetMortonCode = GetLocalMortonCode(glovalMortonCode, targetLevel);
	int32 emptyVoxelNumber = 0;

	for (int i = 0; i <= 5; i++) {
		EVoxelSurface surface = EVoxelSurface(i);
		switch (surface)
		{
		case EVoxelSurface::VS_left:
			emptyVoxelNumber = incMortonCodeY32(localTargetMortonCode);
			break;
		case EVoxelSurface::VS_right:
			emptyVoxelNumber = decMortonCodeY32(localTargetMortonCode);
			break;
		case EVoxelSurface::VS_forward:
			emptyVoxelNumber = incMortonCodeX32(localTargetMortonCode);
			break;
		case EVoxelSurface::VS_back:
			emptyVoxelNumber = decMortonCodeX32(localTargetMortonCode);
			break;
		case EVoxelSurface::VS_top:
			emptyVoxelNumber = incMortonCodeZ32(localTargetMortonCode);
			break;
		case EVoxelSurface::VS_down:
			emptyVoxelNumber = decMortonCodeZ32(localTargetMortonCode);
			break;
		}

		if (emptyVoxelNumber >= iPow[targetLevel] || emptyVoxelNumber <= -1) {
			continue;
		}

		emptyVoxelNumber = GetGlovalMortonCode(emptyVoxelNumber, targetLevel);

		if (IsNotObstacle(emptyVoxelNumber)) {
			results.Add(emptyVoxelNumber);
		}
	}

	return results;
}

FVector USVO_SaveGame::GetLocationFromMortonCode(int32 globalMortonCode, int level)
{
	int32 localMortonCode = GetLocalMortonCode(globalMortonCode, level);
	FVector targetLocation;
	fromMortonCode32(localMortonCode, targetLocation);

	targetLocation = targetLocation * units[level];

	FVector targetLevelCenterUnit = units[level] / 2;

	return rgnMin + (targetLocation + targetLevelCenterUnit);
}

FVector USVO_SaveGame::GetExtentOfVoxel(int32 glovalmortonCode, int level) {
	return units[level] / 2;
}

int32 USVO_SaveGame::GetSizeOfVoxel(int differrenceOfLevel)
{
	if (differrenceOfLevel > 4) {
		return -1;
	}

	static int32_t sizeOfVoxel[5] = { 0,7,63,511,4095 };
	return sizeOfVoxel[differrenceOfLevel];
}

bool USVO_SaveGame::GetMortonCodeInNode(int32& MortonCode, int level) {
	if (level < 2) {
		return true;
	}

	if (svo_Nodes.Contains(MortonCode)) {
		return true;
	}

	int32 targetMortonCode = MortonCode;

	for (int i = level - 1; i >= 2; i--) {
		targetMortonCode = GetParentMortonNumber(targetMortonCode);
		if (svo_Nodes.Contains(targetMortonCode)) {
			MortonCode = targetMortonCode;
			return true;
		}
	}

	return false;
}

int32 USVO_SaveGame::GetChildMortonNumber(int32 parentMortonCodeOfGloval)
{
	return (parentMortonCodeOfGloval << 3) + 1;
}

int32 USVO_SaveGame::GetParentMortonNumber(int32 childMortonCodeOfGloval)
{
	return (childMortonCodeOfGloval - 1) >> 3;
}

void USVO_SaveGame::GenerateBricks() {
	bricks.SetNumZeroed(299593);

	for (auto& x : Obstacles) {

		//L1,L0のモートン番号をはじく
		if (x <= 8) continue;

		int currentLevelOfObstacle = GetLevel(x);

		if (currentLevelOfObstacle != 6) {

			//障害物のレベル6のモートン番号を登録する
			int differrencelevel = 6 - currentLevelOfObstacle;
			int32_t voxelSizeOfMortonCode = GetSizeOfVoxel(differrencelevel);

			int32_t firstMortonCodeOfMaxLevel = GetFirstMortonCodeOfMaxLevel(x, currentLevelOfObstacle);
			int32_t lastMortonCodeOfMaxLevel = firstMortonCodeOfMaxLevel + voxelSizeOfMortonCode;

			for (int32_t registMortonCode = firstMortonCodeOfMaxLevel; registMortonCode <= lastMortonCodeOfMaxLevel; registMortonCode++)
				RegistMortonCodeToBricks(registMortonCode);

		}
		else if (currentLevelOfObstacle == 6) {
			RegistMortonCodeToBricks(x);
		}
		else { //レベル7以上は対応していないので失敗
			return;
		}
	}

	// NodeIndex / Level
	//	0	L2
	//	1	L3
	//	9	L4
	//	73	L5
	//	585	L6
	uint64 firstNodeCount[] = { 0,0,0,1,9,73,585 };
	uint64 temp = 0xff;
	int level = 0;

	//L6まで
	for (int i = 4680; i > 0; i--) {

		if (bricks[i] == 0) continue;

		if (i >= 585) {
			level = 6;
		}
		else if (i >= 73) {
			level = 5;
		}
		else if (i >= 9) {
			level = 4;
		}
		else {
			level = 3;
		}

		temp = 0xff;
		for (int j = 0; j < 8; j++) {
			if ((bricks[i] & temp) != 0) {
				//LocalMortonCode
				int64 tempMortonCode = (i - firstNodeCount[level]) * 64;
				int64 parentLocalMortonCode = tempMortonCode / 8 + j;
				int parentlevel = level - 1;
				RegistMortonCodeToBricks(GetGlovalMortonCode(parentLocalMortonCode, parentlevel));
			}
			temp <<= 8;
		}
	}
}

void USVO_SaveGame::GenerateNodes() {
	//グローバルモートンコード
		//初期化も同時に行う
		//L2,L3,L4,L5,L6


	//障害物のない空間をSVOノードに登録する
	int count = 0;
	//Level2からスタート
	for (int32 m = 9; m <= 73; m++) {
		if (IsNotObstacle(m)) {
			svo_Nodes.Add(m);
			count++;
		}
		else {
			int nextLevel = 3;
			//再帰を使って、最低レベル(uiLevel)までの障害物のない空間をSVOノードに登録していく.
			registNotObstaclesInSVONodes(GetChildMortonNumber(m), nextLevel, count);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("testRegistSVONodes.Add count is %d"), count);

	//ノードのモートン番号が入る
	int32 registNode;
	//一時的なローカルモートンコード保存用変数
	int32 tempLocalMortonCode;
	//パス探索時の面の中心座標
	FVector registFaceCenterLocation;
	//パス探索時の通る面の大きさ
	FVector registFaceScale;
	//リンク元のノードのレベル
	unsigned int templevel;

	//Nodeの隣接リンクを張る
	for (auto& x : svo_Nodes) {
		templevel = GetLevel(x.Key);

		tempLocalMortonCode = GetLocalMortonCode(x.Key, templevel);

		for (int i = 0; i <= 5; i++) {
			EVoxelSurface surface = EVoxelSurface(i);
			//FFaceInfo faceInfo;

			switch (surface)
			{
			case EVoxelSurface::VS_left:
				registNode = incMortonCodeY32(tempLocalMortonCode);
				break;
			case EVoxelSurface::VS_right:
				registNode = decMortonCodeY32(tempLocalMortonCode);
				break;
			case EVoxelSurface::VS_forward:
				registNode = incMortonCodeX32(tempLocalMortonCode);
				break;
			case EVoxelSurface::VS_back:
				registNode = decMortonCodeX32(tempLocalMortonCode);
				break;
			case EVoxelSurface::VS_top:
				registNode = incMortonCodeZ32(tempLocalMortonCode);
				break;
			case EVoxelSurface::VS_down:
				registNode = decMortonCodeZ32(tempLocalMortonCode);
				break;
			default:
				return;
			}

			if (registNode >= iPow[templevel] || registNode <= -1) {
				continue;
			}

			//登録のため、グローバルに戻す
			registNode = GetGlovalMortonCode(registNode, templevel);

			//Nodeが存在すればそのまま登録する
			if (svo_Nodes.Contains(registNode)) {
				FFaceInfo faceInfo = calcFaceInformation(x.Key, templevel, surface);
				faceInfo.mortonCode = registNode;

				x.Value.adjacentFaces.Push(faceInfo);
			}
			//Nodeが存在しなかった場合は障害物チェックする
			else {
				//障害物なし
				if (IsNotObstacle(registNode)) {
					//対象の親をノードの中で見つけるまでループし、登録する(L2まで)
					if (templevel <= 2) continue;

					//int32 tempParentRegistNode;
					for (int j = templevel - 1; j >= 2; j--) {
						//tempParentRegistNode = GetParentMortonNumber(tempRegistNode);
						registNode = GetParentMortonNumber(registNode);
						if (svo_Nodes.Contains(registNode)) {
							FFaceInfo faceInfo = calcFaceInformation(x.Key, templevel, surface);
							faceInfo.mortonCode = registNode;
							x.Value.adjacentFaces.Push(faceInfo);
							break;
						}
					}
				}
				//障害物あり
				else {
					if (templevel >= uiLevel) continue;

					//再帰を使って、最低レベル(uiLevel)までリンクを貼るために回す
					AddSVOLinkNearObstacle(x.Value.adjacentFaces, surface, registNode, templevel + 1);
				}
			}
		}
	}
}

void USVO_SaveGame::AddSVOLinkNearObstacle(TArray<FFaceInfo>& faces, const EVoxelSurface& surface, int32 glovalMortonCode, unsigned int levelCount) {

	//子供たちの先頭を取得
	int32 top = GetChildMortonNumber(glovalMortonCode);
	//隣接確認のためにローカルに変換
	top = GetLocalMortonCode(top, levelCount);

	//障害物のないNodeを子供まで追っていき、登録する(上限Lv6)
	for (int32 m = 0; m <= 7; m++) {
		int32 mCode = top + m;

		//対象のNodeが隣接かどうか確認する
		if (!IsNeighbor(surface, mCode)) {
			continue;
		}
		else {
			//障害物確認やNodeに含まれているかどうかの確認のためにグローバルに変換
			mCode = GetGlovalMortonCode(mCode, levelCount);

			if (IsNotObstacle(mCode)) {
				if (svo_Nodes.Contains(mCode)) {
					FFaceInfo faceInfo = calcFaceInformation(mCode, levelCount, surface, true);
					faces.Push(faceInfo);
				}
			}
			else {
				if (levelCount + 1 <= uiLevel) {
					AddSVOLinkNearObstacle(faces, surface, mCode, levelCount + 1);
				}
			}
		}
	}

}

void USVO_SaveGame::registNotObstaclesInSVONodes(int32 glovalMortonCode, int levelCount, int& count) {

	for (int32 m = 0; m <= 7; m++) {
		const int32 targetMortonCode = glovalMortonCode + m;
		if (IsNotObstacle(targetMortonCode)) {
			svo_Nodes.Add(targetMortonCode);
			count++;
		}
		else {
			unsigned int nextLevel = levelCount + 1;
			if (nextLevel > uiLevel) continue;

			registNotObstaclesInSVONodes(GetChildMortonNumber(targetMortonCode), nextLevel, count);
		}
	}
}

FFaceInfo USVO_SaveGame::calcFaceInformation(const int32 glovalMortonCode,const int level, const EVoxelSurface& surface,const bool isChild) {
	FFaceInfo faceInfo;
	faceInfo.mortonCode = glovalMortonCode;
	faceInfo.center = GetLocationFromMortonCode(glovalMortonCode, level);
	FVector nodeExtent = GetExtentOfVoxel(glovalMortonCode, level);

	FVector leftUP;
	FVector leftDown;
	FVector rightUP;
	FVector rightDown;

	int sign = 1;

	if (isChild) {
		sign = -1;
	}

	switch (surface)
	{
	case EVoxelSurface::VS_forward:
		faceInfo.center.X += nodeExtent.X * sign;
		leftUP = faceInfo.center + FVector(0, -nodeExtent.Y, nodeExtent.Z);
		leftDown = faceInfo.center + FVector(0, -nodeExtent.Y, -nodeExtent.Z);
		rightUP = faceInfo.center + FVector(0, nodeExtent.Y, nodeExtent.Z);
		rightDown = faceInfo.center + FVector(0, nodeExtent.Y, -nodeExtent.Z);
		faceInfo.xyLeft = FVector2D(leftUP.X, leftUP.Y);
		faceInfo.xyRight = FVector2D(rightUP.X, rightUP.Y);
		faceInfo.xzLeft = FVector2D(leftDown.X, leftDown.Z);
		faceInfo.xzRight = FVector2D(leftUP.X, leftUP.Z);
		faceInfo.skipAxis = ESkipAxis::SA_yz;
		break;
	case EVoxelSurface::VS_back:
		faceInfo.center.X -= nodeExtent.X * sign;
		leftUP = faceInfo.center + FVector(0, nodeExtent.Y, nodeExtent.Z);
		leftDown = faceInfo.center + FVector(0, nodeExtent.Y, -nodeExtent.Z);
		rightUP = faceInfo.center + FVector(0, -nodeExtent.Y, nodeExtent.Z);
		rightDown = faceInfo.center + FVector(0, -nodeExtent.Y, -nodeExtent.Z);
		faceInfo.xyLeft = FVector2D(leftUP.X, leftUP.Y);
		faceInfo.xyRight = FVector2D(rightUP.X, rightUP.Y);
		faceInfo.xzLeft = FVector2D(rightUP.X, rightUP.Z);
		faceInfo.xzRight = FVector2D(rightDown.X, rightDown.Z);
		faceInfo.skipAxis = ESkipAxis::SA_yz;
		break;
	case EVoxelSurface::VS_left:
		faceInfo.center.Y += nodeExtent.Y * sign;
		leftUP = faceInfo.center + FVector(nodeExtent.X, 0, nodeExtent.Z);
		leftDown = faceInfo.center + FVector(nodeExtent.X, 0, -nodeExtent.Z);
		rightUP = faceInfo.center + FVector(-nodeExtent.X, 0, nodeExtent.Z);
		rightDown = faceInfo.center + FVector(-nodeExtent.X, 0, -nodeExtent.Z);
		faceInfo.xyLeft = FVector2D(leftUP.X, leftUP.Y);
		faceInfo.xyRight = FVector2D(rightUP.X, rightUP.Y);
		faceInfo.yzLeft = FVector2D(leftDown.Y, leftDown.Z);
		faceInfo.yzRight = FVector2D(leftUP.Y, leftUP.Z);
		faceInfo.skipAxis = ESkipAxis::SA_xz;
		break;
	case EVoxelSurface::VS_right:
		faceInfo.center.Y -= nodeExtent.Y * sign;
		leftUP = faceInfo.center + FVector(-nodeExtent.X, 0, nodeExtent.Z);
		leftDown = faceInfo.center + FVector(-nodeExtent.X, 0, -nodeExtent.Z);
		rightUP = faceInfo.center + FVector(nodeExtent.X, 0, nodeExtent.Z);
		rightDown = faceInfo.center + FVector(nodeExtent.X, 0, -nodeExtent.Z);
		faceInfo.xyLeft = FVector2D(leftUP.X, leftUP.Y);
		faceInfo.xyRight = FVector2D(rightUP.X, rightUP.Y);
		faceInfo.yzLeft = FVector2D(rightUP.Y, rightUP.Z);
		faceInfo.yzRight = FVector2D(rightDown.Y, rightDown.Z);
		faceInfo.skipAxis = ESkipAxis::SA_xz;
		break;
	case EVoxelSurface::VS_top:
		faceInfo.center.Z += nodeExtent.Z * sign;
		leftUP = faceInfo.center + FVector(-nodeExtent.X, -nodeExtent.Y, 0);
		leftDown = faceInfo.center + FVector(nodeExtent.X, -nodeExtent.Y, 0);
		rightUP = faceInfo.center + FVector(-nodeExtent.X, nodeExtent.Y, 0);
		rightDown = faceInfo.center + FVector(nodeExtent.X, nodeExtent.Y, 0);
		faceInfo.xzLeft = FVector2D(leftDown.X, leftDown.Z);
		faceInfo.xzRight = FVector2D(leftUP.X, leftUP.Z);
		faceInfo.yzLeft = FVector2D(rightDown.Y, rightDown.Z);
		faceInfo.yzRight = FVector2D(leftDown.Y, leftDown.Z);
		faceInfo.skipAxis = ESkipAxis::SA_xy;
		break;
	case EVoxelSurface::VS_down:
		faceInfo.center.Z -= nodeExtent.Z * sign;
		leftUP = faceInfo.center + FVector(nodeExtent.X, -nodeExtent.Y, 0);
		leftDown = faceInfo.center + FVector(-nodeExtent.X, -nodeExtent.Y, 0);
		rightUP = faceInfo.center + FVector(nodeExtent.X, nodeExtent.Y, 0);
		rightDown = faceInfo.center + FVector(-nodeExtent.X, nodeExtent.Y, 0);
		faceInfo.xzLeft = FVector2D(leftDown.X, leftDown.Z);
		faceInfo.xzRight = FVector2D(leftUP.X, leftUP.Z);
		faceInfo.yzLeft = FVector2D(leftUP.Y, leftUP.Z);
		faceInfo.yzRight = FVector2D(rightUP.Y, rightUP.Z);
		faceInfo.skipAxis = ESkipAxis::SA_xy;
		break;
	}

	return faceInfo;
}

bool USVO_SaveGame::IsNeighbor(const EVoxelSurface& surface, int32 localMortonCode)
{
	if (surface == EVoxelSurface::VS_left) {
		if ((localMortonCode & 7) == 2 || (localMortonCode & 7) == 3 ||
			(localMortonCode & 7) == 6 || (localMortonCode & 7) == 7) {
			return false;
		}
	}

	else if (surface == EVoxelSurface::VS_right) {
		if ((localMortonCode & 7) == 0 || (localMortonCode & 7) == 1 ||
			(localMortonCode & 7) == 4 || (localMortonCode & 7) == 5) {
			return false;
		}
	}
	if (surface == EVoxelSurface::VS_forward) {
		if ((localMortonCode & 7) == 1 || (localMortonCode & 7) == 3 ||
			(localMortonCode & 7) == 5 || (localMortonCode & 7) == 7) {
			return false;
		}
	}
	else if (surface == EVoxelSurface::VS_back) {
		if ((localMortonCode & 7) == 0 || (localMortonCode & 7) == 2 ||
			(localMortonCode & 7) == 4 || (localMortonCode & 7) == 6) {
			return false;
		}
	}
	else if (surface == EVoxelSurface::VS_top) {
		if ((localMortonCode & 7) == 4 || (localMortonCode & 7) == 5 ||
			(localMortonCode & 7) == 6 || (localMortonCode & 7) == 7) {
			return false;
		}
	}

	else if (surface == EVoxelSurface::VS_down) {
		if ((localMortonCode & 7) == 0 || (localMortonCode & 7) == 1 ||
			(localMortonCode & 7) == 2 || (localMortonCode & 7) == 3) {
			return false;
		}
	}

	return true;
}

int32 USVO_SaveGame::GetFirstMortonCodeOfMaxLevel(int32 glovalMortonCode, unsigned int currentLevel)
{
	if (currentLevel >= uiLevel) return glovalMortonCode;

	for (int i = currentLevel; i < 6; i++) {
		glovalMortonCode = GetChildMortonNumber(glovalMortonCode);
	}

	return glovalMortonCode;
}

void USVO_SaveGame::RegistMortonCodeToBricks(int32 registGlovalMortonCode)
{
	int nodeIndex = (registGlovalMortonCode - 9) >> 6;
	int temp = registGlovalMortonCode;
	temp -= 9;
	temp = temp & 0x3f;
	bricks[nodeIndex] |= ((uint64_t)1 << temp);
}

int32 USVO_SaveGame::getIsNotObstacleCodeInMaxLevel(int32 glovalMortonCode)
{
	//親のモートン番号を割り出す
	int32 parentMortonCode = (glovalMortonCode - 1) / 8;
	int32 tempMortonCode = glovalMortonCode;

	//障害物が見つかるまで親のモートン番号をさかのぼる。
	//L1以下のモートン番号が出た場合はそこで終了
	while (parentMortonCode > 8 && IsNotObstacle(parentMortonCode)) {
		tempMortonCode = parentMortonCode;
		parentMortonCode = (tempMortonCode - 1) / 8;
	}

	return tempMortonCode;
}

bool USVO_SaveGame::IsSameOnMortonCode(int32 glovalMortonCode)
{
	int level = GetLevel(glovalMortonCode);
	if (level == uiLevel || level == 0) return false;

	int32 childMortoCode = GetChildMortonNumber(glovalMortonCode);

	bool IsNotObstacleInParent = false;
	bool IsObstacleInParent = false;

	for (int32 i = 0; i < 8; i++) {
		if (IsNotObstacle(childMortoCode + i)) {
			IsNotObstacleInParent = true;
		}
		else {
			IsObstacleInParent = true;
		}
	}

	//全部が同じ
	return !(IsNotObstacleInParent && IsObstacleInParent);
}




