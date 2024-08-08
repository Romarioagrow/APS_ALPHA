#include "GameSave.h"

UGeneratedWorld* UGameSave::GetGeneratedWorld()
{
	return GeneratedWorld;
}

FString UGameSave::GetSaveSlotName()
{
	return SaveSlotName;
}
