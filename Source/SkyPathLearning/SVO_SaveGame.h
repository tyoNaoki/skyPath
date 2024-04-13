// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SVO_SaveGame.generated.h"

class ASVO_Volume;

//�t�@���l���A���S���Y��//
//�{�N�Z���̊e��
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

//���m�莲
UENUM(BlueprintType)
enum class ESkipAxis : uint8 {
	SA_xy    UMETA(DisplayName = "Skip_xy"),
	SA_xz    UMETA(DisplayName = "Skip_xz"),
	SA_yz    UMETA(DisplayName = "Skip_yz"),
	SA_none     UMETA(DisplayName = "Skip_none"),
};

//�t�@���l���A���S���Y���Ȑ��f�[�^
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

//�{�N�Z���f�[�^
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

//�אڃm�[�h
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


	//�����t���O
	UPROPERTY()
		bool isGenerate = false;

	//SVO�������t���O
	UPROPERTY()
		bool isDoneGenerateFramework = false;

	//�{�N�Z���̍��v��
	UPROPERTY()
		int32 dwCellNum;

	//SVO�̕ϐ�
	UPROPERTY()
		ASVO_Volume* _svo_Volume;

	// �ŉ��ʃ��x��
	UPROPERTY()
		unsigned int uiLevel;

	//�@�ݒ�ł���z��̑傫��(�z��̂��߂̕ϐ��Ȃ̂ł�������[�P)
	static const int MAX_INDEX = 7;

	//���x�����Ƃ̈�̃{�N�Z���̑傫��
	UPROPERTY()
		FVector units[MAX_INDEX + 1];

	//�ׂ���z��
	//f(x) = X_n-1 * 8
	//[1,8,64,512,,,,]
	UPROPERTY()
		int32 iPow[MAX_INDEX + 1];

	//�w�肵�����x���܂ł̑S�Ă�Voxel���܂߂����v��
	//f(x) = X_n-1 + iPow[X] 
	//[1,9,]
	UPROPERTY()
		int32 totalOfVoxels[MAX_INDEX + 1];

	//��Q��
	UPROPERTY()
		TArray<int32>Obstacles;

	//�T���m�[�h
	//UPROPERTY()
	//	TArray<int32>searchNodes;

	//voxel��64bit�Ŏ��[
	UPROPERTY()
		TArray<uint64>bricks;

	//SVO
	//<��Ԕԍ�(���[�g���R�[�h),�אڂ�����Ԃ̃��X�g�����q�ɂ����\����>
	UPROPERTY()
		TMap<int32, FAdjacentNodes>svo_Nodes;

	

public:
	USVO_SaveGame();

	// �̈�̕�
	UPROPERTY(BlueprintReadOnly, Category = svo)
		FVector unit_W;

	// �̈�̍ŏ��l
	UPROPERTY(BlueprintReadOnly, Category = svo)
		FVector rgnMin;

	// �̈�̍ő�l
	UPROPERTY(BlueprintReadOnly, Category = svo)
		FVector rgnMax;

private:
	//X����1���Z
	int32 incMortonCodeX32(int32 code);

	//Y����1���Z
	int32 incMortonCodeY32(int32 code);

	//Z����1���Z
	int32 incMortonCodeZ32(int32 code);

	//X����1���Z
	int32 decMortonCodeX32(int32 code);

	//Y����1���Z
	int32 decMortonCodeY32(int32 code);

	//Z����1���Z
	int32 decMortonCodeZ32(int32 code);

	//�m�[�h��Ώۂ̃m�[�h�����Z�i���炷�j
	int32 addMortonCode32(int32 code, int32 add);

	//3bit���ɊԊu���J����֐�
	int32 BitSeparateFor3D(int32 n);

	//�����؃��[�g�������Z�o�֐�
	int32 Get3DMortonNumber(int32 x, int32 y, int32 z);

	//�_�̍��W���烂�[�g���R�[�h���擾����֐�
	int32 GetPointElem(FVector& p);

	//���[�g���R�[�h������W�ϊ�
	void fromMortonCode32(int32 code, float& xRet, float& yRet, float& zRet);

	//���[�g���R�[�h����x�N�g���ϊ�
	void fromMortonCode32(int32 code, FVector& TargetVector);

	//�O���[�o�����烍�[�J�����[�g���R�[�h�ɕϊ�
	int32 GetLocalMortonCode(int32 globalMortonCode, int32 level);

	//���[�J������O���[�o�����[�g���R�[�h�ɕϊ�
	int32 GetGlovalMortonCode(int32 localMortonCode, int level);

public:
	
	//SVO�̊���������֐�
	bool GenerateFrameworkOfSVO(ASVO_Volume* svoVolume, int Level);

	//�Ώۂ̃��[�g���R�[�h�ɑΉ�����{�N�Z������Q���ł͂Ȃ����𔻒肷��֐�
	UFUNCTION()
		bool IsNotObstacle(int32 glovalMortonCode);

	//�S�Ă̏�Q����o�^����֐�
	bool RegistAllObstacles(UObject* WorldContextObject, ETraceTypeQuery traceType, const TArray<AActor*>& ActorsToIgnore);

	//���W�����Q���o�^����֐�
	void RegistObstacleFromLocation(FVector obstacleLocation, FVector obstacleScale);

	//���[�g���R�[�h�����Q���o�^����֐�
	bool RegistObstacleFromNumber(int32 glovalMortonCode);

	//�Ώۂ̃{�N�Z����`�悷��֐�
	UFUNCTION()
		void DrawVoxelFromNumber(int glovalNumber, FColor color, float duration, float thickness);

	//��Q���{�N�Z����`�悷��֐�
	UFUNCTION()
		void DrawObstacleVoxels(FColor color, float duration, float thickness);

	//�m�[�h�����ׂĕ`�悷��֐�
	UFUNCTION()
		void DrawNodes(FColor color, float duration, float thickness);

	//�Ώۂ̃m�[�h�ɗאڂ����m�[�h�����ׂĕ`�悷��֐�
	UFUNCTION()
		void DrawAdjacentNodes(int32 mortonCode, FColor color, float duration, float thickness);

	//�Ώۂ̃m�[�h�̊e�ʂ�`�悷��֐�
	UFUNCTION()
		void DrawFaces(int32 mortonCode, float size, FColor color, float duration);

	//�o�^����Ă����Q�����폜����֐�
	UFUNCTION()
		void ClearObstacle();

	//SVO�����֐�
	bool GenerateSparseVoxelOctree();

	//�O���[�o�����[�g���ԍ��̃��x���擾����֐�
	UFUNCTION()
		int GetLevel(int32 glovalMortonCode);

	//���W����O���[�o�����[�g���R�[�h���擾�֐�
	UFUNCTION()
		int32 GetMortonNumber(FVector Min, FVector Max);

	//�L�����N�^�[��p�̃��[�g���R�[�h�擾�֐�
	UFUNCTION()
		int32 GetMortonNumberToCharacter(FVector center, FVector side);

	//��_���炻�̍��W���܂ރ{�N�Z���̃��[�g���ԍ��擾
	UFUNCTION()
		int32 GetMortonNumberFromPoint(FVector targetLocation);

	//�אڂ����{�N�Z���̃��[�g���R�[�h���擾����֐�
	//����:SVO�m�[�h���̃��[�g���R�[�h
	UFUNCTION()
		FAdjacentNodes &GetAdjacentNodes(int32 mortonCode);

	//��Q�������̗אڂ����{�N�Z���̃��[�g���R�[�h�擾����֐�
	//����:��Q������̃��[�g���R�[�h
	UFUNCTION()
		TArray<int32> GetNearEmptyVoxelNumbers(int32 glovalMortonCode);

	//�O���[�o�����[�g���R�[�h������W�擾
	UFUNCTION()
		FVector GetLocationFromMortonCode(int32 globalMortonCode, int level);

	//���[�J�����[�g���R�[�h�ɑΉ�����{�N�Z���f�[�^�擾
	UFUNCTION()
		FVector GetExtentOfVoxel(int32 mortonCode, int level);

	//���x������{�N�Z����Ԑ��擾
	UFUNCTION()
		int32 GetSizeOfVoxel(int differrenceOfLevel);

	//�Ώۂ̃��[�g���R�[�h�ɑΉ�����SVO�m�[�h���̃��[�g���R�[�h���擾����֐�
	UFUNCTION()
		bool GetMortonCodeInNode(int32& MortonCode, int level);

	//�q�̃��[�g���ԍ��擾����֐�
	UFUNCTION()
		int32 GetChildMortonNumber(int32 parentMortonCodeOfGloval);

	//�e�̃��[�g���ԍ��擾
	UFUNCTION()
		int32 GetParentMortonNumber(int32 childMortonCodeOfGloval);

private:
	//�u���b�N�𐶐�����֐�
	void GenerateBricks();

	//�m�[�h�𐶐�����֐�
	void GenerateNodes();

	//�ċA�֐�
	//��Q���t�߂̋�Ԃ��Œ჌�x��(uiLevel)�܂ŉ񂵂āA�אڂ�����Ԃ�ǉ����Ă���
	void AddSVOLinkNearObstacle(TArray<FFaceInfo>& faces,const EVoxelSurface & surface,int32 glovalMortonCode,unsigned int levelCount);

	//SV�m�[�h
	void registNotObstaclesInSVONodes(int32 glovalMortonCode, int levelCount, int& count);

	//�{�N�Z���̕������x���������ꍇ���אڃ{�N�Z���̃��x���������傫���ꍇ
	FFaceInfo calcFaceInformation(const int32 glovalMortonCode,const int level, const EVoxelSurface& surface,const bool isChild = false);

	//�אڂ��Ă��邩���肷��֐�
	bool IsNeighbor(const EVoxelSurface& surface, int32 localMortonCode);

	//��U���x��6�̐擪���[�g���ԍ�������o���֐�
	int32 GetFirstMortonCodeOfMaxLevel(int32 glovalMortonCode,unsigned int currentLevel);

	//Bricks�Ƀ��[�g���R�[�h��o�^����֐�
	void RegistMortonCodeToBricks(int32 registGlovalMortonCode);

	//��Q����������܂Őe���J��グ��(�ő�L2�܂�(1~8))
//�Ԃ�l�̓O���[�o�����[�g���R�[�h
	int32 getIsNotObstacleCodeInMaxLevel(int32 glovalMortonCode);

	//�Ώۂ̃��[�g���R�[�h�͐e�̃��[�g���R�[�h�ɏȗ�����Ă��邩�ǂ������肷��֐�
	bool IsSameOnMortonCode(int32 glovalMortonCode);
	
};
