// Optional regression coverage for OWS issue #174.
// Compile only in an editor module that already depends on licensed MetaRoadEditor.
#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Utils/OpUtils.h"
#include "RoadMeshBuild/ProceduralPolygon.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOWSMetaRoadBoundaryFilterTest,
    "OWS.External.MetaRoad.BoundaryEdgeFilter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOWSMetaRoadBoundaryFilterTest::RunTest(const FString& Parameters)
{
    using UE::Geometry::FIndex2i;
    // Two separate squares joined by an edge which the boundary walk must ignore.
    // Both endpoints have allowed incident edges, reproducing the old vertex test.
    for (int32 Scenario = 0; Scenario < 4; ++Scenario)
    {
        MetaRoad::FDynamicGraph2d Graph;
        const int A = Graph.AppendVertex(FVector2d(0, 0));
        const int B = Graph.AppendVertex(FVector2d(10, 0));
        const int C = Graph.AppendVertex(FVector2d(10, 10));
        const int D = Graph.AppendVertex(FVector2d(0, 10));
        const int E = Graph.AppendVertex(FVector2d(20, 0));
        const int F = Graph.AppendVertex(FVector2d(30, 0));
        const int G = Graph.AppendVertex(FVector2d(30, 10));
        const int H = Graph.AppendVertex(FVector2d(20, 10));
        const int CenterOnly = MetaRoad::GUIFlags::CenterLine;
        const int Surface = Scenario == 3 ? (CenterOnly | 1) : 1;
        for (const FIndex2i Edge : { FIndex2i(A,B), FIndex2i(B,C), FIndex2i(C,D), FIndex2i(D,A),
                                   FIndex2i(E,F), FIndex2i(F,G), FIndex2i(G,H), FIndex2i(H,E) })
        {
            Graph.AppendEdge(Edge, Surface);
        }
        Graph.AppendEdge(B, E, Scenario == 1 || Scenario == 2 ? Surface : CenterOnly);
        TArray<FIndex2i> Skipped;
        if (Scenario == 1) Skipped.Add(FIndex2i(B,E));
        if (Scenario == 2) Skipped.Add(FIndex2i(E,B));
        const OpUtils::TGIDFilter Filter = [CenterOnly](int Group) { return Group != CenterOnly; };
        TArray<FIndex2i> Boundary;
        const FString Label = FString::Printf(TEXT("Scenario %d"), Scenario);
        TestTrue(Label + TEXT(" closes first square"), OpUtils::FindBoundary(Graph, Skipped, Boundary, Filter));
        TestEqual(Label + TEXT(" has four boundary edges"), Boundary.Num(), 4);
        for (const FIndex2i Edge : Boundary)
        {
            TestFalse(Label + TEXT(" never crosses excluded connector"),
                (Edge.A == B && Edge.B == E) || (Edge.A == E && Edge.B == B));
        }
        TArray<TArray<FIndex2i>> Boundaries;
        TestEqual(Label + TEXT(" extracts both disconnected surfaces"),
            OpUtils::FindBoundaries(Graph, Skipped, Boundaries, Filter), 2);
    }
    return !HasAnyErrors();
}

// Synchronous graph-only check: no map load, Play session, rendering, or latent work.
static FAutoConsoleCommand VerifyMetaRoadBoundaryCommand(
    TEXT("OWS.City.VerifyMetaRoadBoundary"),
    TEXT("Run the graph-only MetaRoad edge filter regression."),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        FAutomationTestFramework& Framework = FAutomationTestFramework::Get();
        if (Framework.GetCurrentTest())
        {
            UE_LOG(LogTemp, Warning, TEXT("OWS #174: another automation test is active; check not started."));
            return;
        }
        Framework.StartTestByName(TEXT("FOWSMetaRoadBoundaryFilterTest"), 0);
        if (!Framework.GetCurrentTest())
        {
            UE_LOG(LogTemp, Error, TEXT("OWS #174: regression test is not registered."));
            return;
        }
        FAutomationTestExecutionInfo Result;
        const bool bPassed = Framework.StopTest(Result);
        for (const FAutomationExecutionEntry& Entry : Result.GetEntries())
        {
            UE_LOG(LogTemp, Display, TEXT("OWS #174: %s"), *Entry.Event.Message);
        }
        UE_LOG(LogTemp, Display, TEXT("OWS #174 boundary regression: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
    }));

#endif
