// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

static const FName NAME_GameSessionConst = NAME_GameSession;
static const FName KEY_MatchType(TEXT("MatchType"));

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem() {
	if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(UObject::GetWorld())) {
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

	if (SessionInterface->GetNamedSession(NAME_GameSessionConst) != nullptr) {
		// store pending create and retry after destroy completes
		CreateAfterDestroy = true;
		PendingMaxConnections = MaxConnections;
		PendingIsLan = IsLan;
		PendingMatchType = MatchType;

		DestroySession();
		return;
	}

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = IsLan;
	LastSessionSettings->NumPublicConnections = MaxConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bShouldAdvertise = true;

	// For LAN/Null: keep presence/lobbies OFF
	LastSessionSettings->bUsesPresence = false;
	LastSessionSettings->bAllowJoinViaPresence = false;
	LastSessionSettings->bUseLobbiesIfAvailable = false;

	LastSessionSettings->Set(KEY_MatchType, MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteHandle =
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer) {
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		OnCreateSessionFinished.Broadcast(false);
		return;
	}

	const bool bStarted =
		SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSessionConst,
		                                *LastSessionSettings);

	if (!bStarted) {
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

	LastSessionInfo.Reset();
	LastSessionSearchResults.Reset();

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxResults;
	LastSessionSearch->bIsLanQuery = IsLan;

	// For Null/LAN, do NOT filter by lobbies/presence keys (often returns 0)
	// LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	FindSessionsCompleteHandle =
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		OnFindSessionsFinished.Broadcast(false);
		return;
	}

	const bool bStarted =
		SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());

	if (!bStarted) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		OnFindSessionsFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleFindSessionsComplete(bool WasSuccessful) {
	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	if (!WasSuccessful || !LastSessionSearch.IsValid()) {
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
		
		// Null/LAN often reports 0 players for the host. If there is a valid owning user, clamp to at least 1.
		Info.CurrentPlayers = FMath::Max(Info.CurrentPlayers, 1);

		FString MatchType;
		if (Session.SessionSettings.Get(KEY_MatchType, MatchType)) {
			Info.MatchType = MatchType;
		}

		LastSessionInfo.Add(Info);
	}

	OnFindSessionsFinished.Broadcast(true);
}

void UMultiplayerSessionsSubsystem::JoinSessionByIndex(int32 Index) {
	if (!SessionInterface.IsValid()) {
		OnJoinSessionFinished.Broadcast(false);
		return;
	}

	// If we already have a named session, destroy then retry join
	if (SessionInterface->GetNamedSession(NAME_GameSessionConst) != nullptr) {
		JoinAfterDestroy = true;
		PendingJoinIndex = Index;
		DestroySession();
		return;
	}

	if (Index < 0 || Index >= LastSessionSearchResults.Num()) {
		OnJoinSessionFinished.Broadcast(false);
		return;
	}

	JoinSessionCompleteHandle =
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnJoinSessionFinished.Broadcast(false);
		return;
	}

	const bool bStarted = SessionInterface->JoinSession(
		*LocalPlayer->GetPreferredUniqueNetId(),
		NAME_GameSessionConst,
		LastSessionSearchResults[Index]);

	if (!bStarted) {
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnJoinSessionFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result) {
	if (!SessionInterface.IsValid()) {
		OnJoinSessionFinished.Broadcast(false);
		return;
	}

	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);

	const bool JoinOk = (Result == EOnJoinSessionCompleteResult::Success);
	LastConnectionString.Reset();

	bool HasAddress = false;
	if (JoinOk) {
		HasAddress = SessionInterface->GetResolvedConnectString(SessionName, LastConnectionString);

		if (HasAddress) {
			FString Host;
			FString PortStr;
			if (LastConnectionString.Split(TEXT(":"), &Host, &PortStr) && PortStr == TEXT("0")) {
				LastConnectionString = Host + TEXT(":7777");
			}
		}
	}

	OnJoinSessionFinished.Broadcast(JoinOk);
}

void UMultiplayerSessionsSubsystem::DestroySession() {
	if (!SessionInterface.IsValid()) {
		OnDestroySessionFinished.Broadcast(false);
		return;
	}

	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		DestroySessionCompleteDelegate);

	const bool IsStarted = SessionInterface->DestroySession(NAME_GameSessionConst);
	if (!IsStarted) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		OnDestroySessionFinished.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::HandleDestroySessionComplete(FName SessionName, bool WasSuccessful) {
	if (WasSuccessful && JoinAfterDestroy && PendingJoinIndex != INDEX_NONE) {
		JoinAfterDestroy = false;
		const int32 IndexToJoin = PendingJoinIndex;
		PendingJoinIndex = INDEX_NONE;
		JoinSessionByIndex(IndexToJoin);
	}

	if (SessionInterface.IsValid()) {
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	OnDestroySessionFinished.Broadcast(WasSuccessful);

	if (WasSuccessful && CreateAfterDestroy) {
		CreateAfterDestroy = false;
		CreateSession(PendingMaxConnections, PendingIsLan, PendingMatchType);
	}
}
