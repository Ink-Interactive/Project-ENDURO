// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Multiplayer/MultiplayerSessionsSubsystem.h"

#include "Virtualization/VirtualizationTypes.h"

static const FName NAME_GameSessionConst = NAME_GameSession;
static const FName KEY_MatchType(TEXT("MatchType"));

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() {
	
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get()) {
		SessionInterface = Subsystem->GetSessionInterface();
	}
	
	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::HandleCreateSessionComplete);
	
	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::HandleFindSessionsComplete);
	
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::HandleJoinSessionComplete);
	
	DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(
		this, &UMultiplayerSessionsSubsystem::HandleDestroySessionComplete);
}

const ULocalPlayer* UMultiplayerSessionsSubsystem::GetFirstLocalPlayer() const {
	
	if (!GetWorld()) return nullptr;
	return GetWorld()->GetFirstLocalPlayerFromController();
}

void UMultiplayerSessionsSubsystem::CreateSession(int MaxConnections, bool IsLan, const FString& MatchType) {
	
	if (!SessionInterface.IsValid()) {
		OnCreateSessionFinished.Broadcast(false);
		return;
	}
	
	//If a session already exists. destroy it first
	if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr) {
		DestroySession();
		
		//TODO: Add way to Auto-Retry
		OnCreateSessionFinished.Broadcast(false);
		return;
	}
	
	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = IsLan;
	LastSessionSettings->NumPublicConnections = MaxConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->Set(KEY_MatchType, MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	
	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		OnCreateSessionFinished.Broadcast(false);
		return;
	}
	
	const bool IsStarted  = SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSessionConst, *LastSessionSettings);
	if (!IsStarted) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		OnCreateSessionFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleCreateSessionComplete(FName SessionName, bool WasSuccessful) {
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}
	
	OnCreateSessionFinished.Broadcast(WasSuccessful);
}

void UMultiplayerSessionsSubsystem::FindSessions(int MaxResults, bool IsLan) {
	
	if (!SessionInterface.IsValid()) {
		OnFindSessionsFinished.Broadcast(false);
		return;
	}
	
	LastSessionSettings.Reset();
	LastSessionSearchResults.Reset();
	
	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxResults;
	LastSessionSearch->bIsLanQuery = IsLan;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	
	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	
	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		OnFindSessionsFinished.Broadcast(false);
		return;
	}
	
	const bool IsStarted = SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());
	if (!IsStarted) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		OnFindSessionsFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleFindSessionsComplete(bool WasSuccessful) {
	
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}
	
	if (!WasSuccessful || !LastSessionSettings.IsValid()) {
		OnFindSessionsFinished.Broadcast(false);
		return;
	}
	
	LastSessionSearchResults = LastSessionSearch->SearchResults;
	
	for (const FOnlineSessionSearchResult& Result : LastSessionSearchResults) {
		FSessionInfo Info;
		Info.Name = Result.GetSessionIdStr();
		Info.PingMS = Result.PingInMs;
		
		const FOnlineSession& Session = Result.Session;
		Info.MaxPlayers = Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = Info.MaxPlayers - Session.NumOpenPublicConnections;
		
		FString MatchType;
		if (Session.SessionSettings.Get(KEY_MatchType, MatchType)) {
			Info.MatchType = MatchType;
		}
		
		LastSessionInfo.Add(Info);
	}
	
	OnFindSessionsFinished.Broadcast(true);
}

void UMultiplayerSessionsSubsystem::JoinSessionByIndex(int Index) {
	
	if (!SessionInterface.IsValid() || Index < 0 || Index >= LastSessionSearchResults.Num()) {
		OnJoinSessionFinished.Broadcast(false);
		return;
	}
	
	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	
	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnJoinSessionFinished.Broadcast(false);
		return;
	}
	
	const bool IsStarted = SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSessionConst, LastSessionSearchResults[Index]);
	if (!IsStarted) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnJoinSessionFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result) {
	
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}
	
	LastConnectionString.Reset();
	
	const bool Success = (Result == EOnJoinSessionCompleteResult::Success);
	if (Success && SessionInterface.IsValid()) {
		FString Address;
		if (SessionInterface->GetResolvedConnectString(NAME_GameSessionConst, Address)) {
			LastConnectionString = Address;
		}
	}
	
	OnJoinSessionFinished.Broadcast(Success && !LastConnectionString.IsEmpty());
}

void UMultiplayerSessionsSubsystem::DestroySession() {
	
	if (!SessionInterface.IsValid()) {
		OnDestroySessionFinished.Broadcast(false);
		return;
	}
	
	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	
	const bool IsStarted = SessionInterface->DestroySession(NAME_GameSessionConst);
	if (!IsStarted) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		OnDestroySessionFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleDestroySessionComplete(FName SessionName, bool WasSuccessful) {
	
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}
	
	OnDestroySessionFinished.Broadcast(WasSuccessful);
}
