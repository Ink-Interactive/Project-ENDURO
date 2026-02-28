#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "EnduroSessionTestHelpers.generated.h"

USTRUCT(BlueprintType)
struct FEnduroSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 PingMS = 0;
	UPROPERTY(BlueprintReadOnly) FString MatchType;
};

UCLASS()
class ENDUROTESTS_API UEnduroSessionTestHelpers : public UObject
{
	GENERATED_BODY()

public:
	// Builds info the same way your UI expects.
	static FEnduroSessionInfo BuildInfo(const FOnlineSessionSearchResult& Result, const FName MatchTypeKey = FName("MatchType"));

	// Normalizes "IP:0" / "IP" to default port.
	static FString NormalizeConnectString(const FString& InConnectString, int32 DefaultPort = 7777);
};