#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Character.h"
#include "InteractableTestListener.generated.h"

UCLASS()
class UInteractableTestListener : public UObject
{
	GENERATED_BODY()

public:
	bool bInteractFired = false;

	UPROPERTY()
	TObjectPtr<ACharacter> ReceivedInstigator = nullptr;

	UFUNCTION()
	void HandleInteract(ACharacter* InstigatorCharacter)
	{
		bInteractFired = true;
		ReceivedInstigator = InstigatorCharacter;
	}
};
