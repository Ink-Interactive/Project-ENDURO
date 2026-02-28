// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerSessionsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSessionInfo {
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) int CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int MaxPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int PingMS = 0;
	UPROPERTY(BlueprintReadOnly) FString MatchType;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateSessionFinished, bool, Success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindSessionsFinished, bool, Success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionFinished, bool, Success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroySessionFinished, bool, Success);

UCLASS(BlueprintType)
class PROJECTENDURO_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem {
	
	GENERATED_BODY()
	
public:
	UMultiplayerSessionsSubsystem();
	
	//Functions for UI
	UFUNCTION(BlueprintCallable) void CreateSession(int MaxConnections, bool IsLan, const FString& MatchType);
	UFUNCTION(BlueprintCallable) void FindSessions(int MaxResults, bool IsLan);
	UFUNCTION(BlueprintCallable) void JoinSessionByIndex(int Index);
	UFUNCTION(BlueprintCallable) void DestroySession();
	
	UFUNCTION(BlueprintCallable) TArray<FSessionInfo> GetLastSessionInfo() const { return LastSessionInfo; }; 
	UFUNCTION(BlueprintCallable) FString GetLastConnectionString() const { return LastConnectionString; }
	
	//Bindings
	UPROPERTY(BlueprintAssignable) FOnCreateSessionFinished OnCreateSessionFinished;
	UPROPERTY(BlueprintAssignable) FOnFindSessionsFinished OnFindSessionsFinished;
	UPROPERTY(BlueprintAssignable) FOnJoinSessionFinished OnJoinSessionFinished;
	UPROPERTY(BlueprintAssignable) FOnDestroySessionFinished OnDestroySessionFinished;
	
	private:
	IOnlineSessionPtr SessionInterface;
	
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	
	TArray<FOnlineSessionSearchResult> LastSessionSearchResults;
	TArray<FSessionInfo> LastSessionInfo;
	FString LastConnectionString;
	
	//Delegates
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteHandle;
	
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteHandle;
	
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteHandle;
	
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteHandle;
	
	//Vars
	bool CreateAfterDestroy = false;
	int PendingMaxConnections = 0;
	bool PendingIsLan = true;
	FString PendingMatchType;
	bool JoinAfterDestroy = false;
	int32 PendingJoinIndex = INDEX_NONE;
	
private:
	void HandleCreateSessionComplete(FName SessionName, bool WasSuccessful);
	void HandleFindSessionsComplete(bool WasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool WasSuccessful);
	
	const ULocalPlayer* GetFirstLocalPlayer() const;
};

