#include "Multiplayer/EnduroSessionTestHelpers.h"

FEnduroSessionInfo UEnduroSessionTestHelpers::BuildInfo(const FOnlineSessionSearchResult& Result, const FName MatchTypeKey)
{
	FEnduroSessionInfo Info;

	Info.Name = Result.GetSessionIdStr();
	Info.PingMS = Result.PingInMs;

	const FOnlineSession& Session = Result.Session;

	Info.MaxPlayers = Session.SessionSettings.NumPublicConnections;
	Info.CurrentPlayers = Info.MaxPlayers - Session.NumOpenPublicConnections;

	// Keep UI sane for Null/LAN: at least 1 when MaxPlayers is known
	if (Info.MaxPlayers > 0)
	{
		Info.CurrentPlayers = FMath::Clamp(Info.CurrentPlayers, 1, Info.MaxPlayers);
	}

	FString MatchType;
	if (Session.SessionSettings.Get(MatchTypeKey, MatchType))
	{
		Info.MatchType = MatchType;
	}

	return Info;
}

FString UEnduroSessionTestHelpers::NormalizeConnectString(const FString& InConnectString, int32 DefaultPort)
{
	if (InConnectString.IsEmpty())
	{
		return InConnectString;
	}

	FString Host, PortStr;
	if (InConnectString.Split(TEXT(":"), &Host, &PortStr))
	{
		if (PortStr.IsEmpty() || PortStr == TEXT("0"))
		{
			return Host + TEXT(":") + FString::FromInt(DefaultPort);
		}
		return InConnectString;
	}

	return InConnectString + TEXT(":") + FString::FromInt(DefaultPort);
}