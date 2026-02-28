#include "Misc/AutomationTest.h"
#include "Multiplayer/EnduroSessionTestHelpers.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnduro_BuildInfo_Basic,
    "ProjectEnduroTests.Multiplayer.SessionTests.BuildInfo.Basic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnduro_BuildInfo_Basic::RunTest(const FString& Parameters)
{
    FOnlineSessionSearchResult R;
    R.PingInMs = 12;

    R.Session.SessionSettings.NumPublicConnections = 4;
    R.Session.NumOpenPublicConnections = 3; // 1 player

    R.Session.SessionSettings.Set(FName("MatchType"), FString("Enduro"),
        EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    const FEnduroSessionInfo Info = UEnduroSessionTestHelpers::BuildInfo(R);

    TestEqual(TEXT("PingMS"), Info.PingMS, 12);
    TestEqual(TEXT("MaxPlayers"), Info.MaxPlayers, 4);
    TestEqual(TEXT("CurrentPlayers"), Info.CurrentPlayers, 1);
    TestEqual(TEXT("MatchType"), Info.MatchType, FString("Enduro"));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnduro_BuildInfo_Clamp,
    "ProjectEnduroTests.Multiplayer.SessionTests.BuildInfo.ClampPlayerCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnduro_BuildInfo_Clamp::RunTest(const FString& Parameters)
{
    FOnlineSessionSearchResult R;
    R.Session.SessionSettings.NumPublicConnections = 4;
    R.Session.NumOpenPublicConnections = 4; // would be 0, clamp to 1

    const FEnduroSessionInfo Info = UEnduroSessionTestHelpers::BuildInfo(R);

    TestEqual(TEXT("CurrentPlayers clamped"), Info.CurrentPlayers, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnduro_NormalizeConnectString_PortZero,
    "ProjectEnduroTests.Multiplayer.SessionTests.NormalizeConnectString.PortZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnduro_NormalizeConnectString_PortZero::RunTest(const FString& Parameters)
{
    const FString Out = UEnduroSessionTestHelpers::NormalizeConnectString(TEXT("192.168.9.20:0"), 7777);
    TestEqual(TEXT("Port 0 -> 7777"), Out, FString(TEXT("192.168.9.20:7777")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnduro_NormalizeConnectString_NoPort,
    "ProjectEnduroTests.Multiplayer.SessionTests.NormalizeConnectString.NoPort",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnduro_NormalizeConnectString_NoPort::RunTest(const FString& Parameters)
{
    const FString Out = UEnduroSessionTestHelpers::NormalizeConnectString(TEXT("192.168.9.20"), 7777);
    TestEqual(TEXT("No port -> append 7777"), Out, FString(TEXT("192.168.9.20:7777")));
    return true;
}