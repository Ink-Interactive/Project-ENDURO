#include "Interactable/InteractableComponent.h"

UInteractableComponent::UInteractableComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableComponent::Interact_Implementation(ACharacter* InstigatorCharacter) {
	if (!bEnabled) {
		return;
	}

	OnInteract.Broadcast(InstigatorCharacter);
}
