// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractSignature, ACharacter*, InstigatorCharacter);

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class PROJECTENDURO_API UInteractableComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UInteractableComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interactable")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interactable")
	float MaxInteractDistance = 300.f;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interactable")
	void Interact(ACharacter* InstigatorCharacter);
	virtual void Interact_Implementation(ACharacter* InstigatorCharacter);

	UPROPERTY(BlueprintAssignable, Category="Interactable")
	FOnInteractSignature OnInteract;
};
