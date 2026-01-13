#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

#include "InteractableTestListener.h"
#include "Interactable/InteractableComponent.h" 

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractableComponent_InteractBroadcasts,
	"ProjectEnduroTests.Interaction.InteractableComponent.BroadcastsOnInteract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter
)

bool FInteractableComponent_InteractBroadcasts::RunTest(const FString& Parameters)
{
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	TestNotNull(TEXT("World should exist"), World);
	if (!World) return false;

	AActor* TargetActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Target actor spawned"), TargetActor);
	if (!TargetActor) return false;

	// Create & register component
	UInteractableComponent* Interactable = NewObject<UInteractableComponent>(TargetActor);
	TestNotNull(TEXT("Interactable component created"), Interactable);
	if (!Interactable) return false;

	Interactable->RegisterComponent();

	// Spawn an instigator character to pass through
	ACharacter* Instigator = World->SpawnActor<ACharacter>();
	TestNotNull(TEXT("Instigator character spawned"), Instigator);
	if (!Instigator) return false;

	// Listener for the dynamic delegate
	UInteractableTestListener* Listener = NewObject<UInteractableTestListener>();
	TestNotNull(TEXT("Listener created"), Listener);
	if (!Listener) return false;

	Interactable->OnInteract.AddDynamic(Listener, &UInteractableTestListener::HandleInteract);

	// Act
	Interactable->Interact(Instigator);

	// Assert broadcast happened
	TestTrue(TEXT("OnInteract should broadcast"), Listener->bInteractFired);

	// Assert instigator passed through correctly
	TestTrue(TEXT("Received instigator should match"), Listener->ReceivedInstigator.Get() == Instigator);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
