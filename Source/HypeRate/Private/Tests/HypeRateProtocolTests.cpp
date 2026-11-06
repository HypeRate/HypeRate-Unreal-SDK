#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HypeRateClientComponent.h"
#include "HypeRateHeartRateWidget.h"
#include "HypeRateProtocol.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypeRateParseUpdateTest,
    "HypeRate.Protocol.ParsesHeartRateUpdate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHypeRateParseUpdateTest::RunTest(const FString& Parameters)
{
    int32 HeartRate = 0;
    const bool bParsed = FHypeRateProtocol::TryParseHeartRate(
        TEXT("{\"event\":\"hr_update\",\"payload\":{\"hr\":76},\"ref\":null,\"topic\":\"hr:test\"}"),
        HeartRate);
    TestTrue(TEXT("hr_update is parsed"), bParsed);
    TestEqual(TEXT("Heart rate is preserved"), HeartRate, 76);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypeRateIgnoreReplyTest,
    "HypeRate.Protocol.IgnoresNonHeartRateMessages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHypeRateIgnoreReplyTest::RunTest(const FString& Parameters)
{
    int32 HeartRate = 0;
    TestFalse(
        TEXT("phx_reply is ignored"),
        FHypeRateProtocol::TryParseHeartRate(
            TEXT("{\"event\":\"phx_reply\",\"payload\":{\"status\":\"ok\"},\"topic\":\"hr:test\"}"),
            HeartRate));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypeRateJoinMessageTest,
    "HypeRate.Protocol.CreatesJoinMessage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHypeRateJoinMessageTest::RunTest(const FString& Parameters)
{
    const FString Message = FHypeRateProtocol::CreateJoin(TEXT("test\"device"), 42);
    TestTrue(TEXT("Topic is escaped"), Message.Contains(TEXT("hr:test\\\"device")));
    TestTrue(TEXT("Join event exists"), Message.Contains(TEXT("phx_join")));
    TestTrue(TEXT("Reference exists"), Message.Contains(TEXT("42")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypeRateSimulationTest,
    "HypeRate.Client.SimulationUsesNormalState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHypeRateSimulationTest::RunTest(const FString& Parameters)
{
    UHypeRateClientComponent* Client = NewObject<UHypeRateClientComponent>();
    Client->BeginSimulation(84);
    TestEqual(TEXT("State is Simulating"), Client->State, EHypeRateConnectionState::Simulating);
    TestTrue(TEXT("Simulation counts as connected"), Client->IsConnected());
    TestEqual(TEXT("Initial heart rate is published"), Client->CurrentHeartRate, 84);
    Client->SetSimulatedHeartRate(132);
    TestEqual(TEXT("Updated heart rate is published"), Client->CurrentHeartRate, 132);
    Client->EndSimulation();
    TestEqual(TEXT("State returns to Disconnected"), Client->State, EHypeRateConnectionState::Disconnected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypeRateWidgetTest,
    "HypeRate.Widget.AcceptsHeartRateValues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHypeRateWidgetTest::RunTest(const FString& Parameters)
{
    UHypeRateHeartRateWidget* Widget = NewObject<UHypeRateHeartRateWidget>();
    Widget->PushHeartRate(96);
    TestEqual(TEXT("Widget exposes the latest heart rate"), Widget->GetCurrentHeartRate(), 96);
    return true;
}

#endif

