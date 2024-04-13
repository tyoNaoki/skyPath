// Fill out your copyright notice in the Description page of Project Settings.


#include "PathFind_Data.h"

void UPathFind_Data::InitializeData(USVO_SaveGame *_svo)
{
	svo = _svo;
	searchNodes.Empty();
}

bool UPathFind_Data::AddSearchNodes(int32 mortonCode)
{
	if(searchNodes.Contains(mortonCode)){
		return false;
	}else{
		searchNodes.AddUnique(mortonCode);
		return true;
	}	
}

TArray<int32>&UPathFind_Data::GetSearchNodes()
{
	return searchNodes;
}

USVO_SaveGame*UPathFind_Data::GetSVO()
{
	return svo;
}



