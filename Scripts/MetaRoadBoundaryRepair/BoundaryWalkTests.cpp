// Optional regression coverage for OWS issue #174.
// Compile only in an editor module that already depends on licensed MetaRoadEditor.
#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Utils/OpUtils.h"
#include "RoadSplineComponent.h"
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
    // OWS #176: an articulation vertex may recur before the outer face closes.
    for (bool bSharedStart : { false, true })
    {
        MetaRoad::FDynamicGraph2d Graph;
        auto Add = [&](double X, double Y) { return Graph.AppendVertex(FVector2d(X,Y)); };
        auto Join = [&](int A, int B) { Graph.AppendEdge(A,B,1); };
        int32 ExpectedEdges;
        if (!bSharedStart)
        {
            const int A=Add(0,0), B=Add(10,0), C=Add(10,10), D=Add(0,10);
            const int E=Add(20,10), F=Add(20,20), G=Add(10,20);
            Join(A,B); Join(B,C); Join(C,D); Join(D,A);
            Join(C,E); Join(E,F); Join(F,G); Join(G,C);
            ExpectedEdges=8;
        }
        else
        {
            const int A=Add(0,0), B=Add(10,5), C=Add(5,10), D=Add(-10,5), E=Add(-5,10);
            Join(A,B); Join(B,C); Join(C,A);
            Join(A,E); Join(E,D); Join(D,A);
            ExpectedEdges=6;
        }
        const FString Label=bSharedStart ? TEXT("Shared start") : TEXT("Shared non-start");
        TArray<FIndex2i> Boundary;
        TestTrue(Label + TEXT(" closes"), OpUtils::FindBoundary(Graph, {}, Boundary, [](int) { return true; }));
        TestEqual(Label + TEXT(" preserves both lobes"), Boundary.Num(), ExpectedEdges);
        TSet<uint64> Covered;
        for (const FIndex2i E : Boundary)
            Covered.Add((uint64(uint32(FMath::Min(E.A,E.B))) << 32) | uint32(FMath::Max(E.A,E.B)));
        TestEqual(Label + TEXT(" includes every perimeter edge"), Covered.Num(), ExpectedEdges);
        TArray<TArray<FIndex2i>> Boundaries;
        TestEqual(Label + TEXT(" extracts one complete outer walk"),
            OpUtils::FindBoundaries(Graph, {}, Boundaries, [](int) { return true; }), 1);
        if (Boundaries.Num()==1)
            TestEqual(Label + TEXT(" extraction retains both lobes"), Boundaries[0].Num(), ExpectedEdges);
    }
    MetaRoad::FDynamicGraph2d Open;
    const int A=Open.AppendVertex(FVector2d(0,0)), B=Open.AppendVertex(FVector2d(10,0));
    Open.AppendEdge(A,B,1);
    TArray<FIndex2i> OpenBoundary;
    TestFalse(TEXT("Open input terminates without a contour"),
        OpUtils::FindBoundary(Open, {}, OpenBoundary, [](int) { return true; }));

    // OWS #177: a full closed linear loop must return to the same offset point.
    URoadSplineComponent* Loop = NewObject<URoadSplineComponent>();
    Loop->ClearSplinePoints(false);
    for (const FVector P : { FVector(0,0,0), FVector(2000,0,0), FVector(2000,2000,0), FVector(0,2000,0) })
        Loop->AddSplinePoint(P, ESplineCoordinateSpace::Local, false);
    for (int32 I=0; I<4; ++I) Loop->SetSplinePointType(I, ESplinePointType::Linear, false);
    Loop->SetClosedLoop(true, true);
    const double Length=Loop->GetSplineLength();
    for (double Offset : { -400.0, 0.0, 400.0 })
    {
        const auto Start=Loop->GetRoadPosition(0.0, Offset, ESplineCoordinateSpace::Local);
        const auto End=Loop->GetRoadPosition(Length, Offset, ESplineCoordinateSpace::Local);
        TestTrue(FString::Printf(TEXT("Closed linear offset %.0f seam closes"), Offset),
            Start.Location.Equals(End.Location, 0.001));
        TestEqual(TEXT("Closing sample retains longitudinal offset"), End.SOffset, Length);
        TestEqual(TEXT("Closing sample retains lateral offset"), End.ROffset, Offset);
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
