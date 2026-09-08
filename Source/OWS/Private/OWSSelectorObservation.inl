// OWS #182. Uses the existing modular selector stack, not a second vision system.
FString UOWSSelectorComponent::CaptureObservation(bool bPrepareDestinations)
{
	NextObservationAt = FPlatformTime::Seconds() + .5;
	AwarenessActors.Reset();
	ObservedDestinations.Reset();
	ObservationSnapshotId = FGuid::NewGuid();
	ObservationCaptureTime = FPlatformTime::Seconds();
	DestinationPreparationStatus = bPrepareDestinations
		? TEXT("Destination preparation unavailable: no valid observation.")
		: TEXT("Destination preparation not requested.");
	UWorld* World = GetWorld();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!World || !Character) return LatestObservation = TEXT("Observation unavailable: no character world.");
	FVector Origin;
	FRotator Facing;
	Character->GetActorEyesViewPoint(Origin, Facing);
	// Observe this body, never the spectator/player camera or controller rotation.
	Facing = Character->GetActorRotation();
	if (const USkeletalMeshComponent* Mesh = Character->GetMesh())
		if (Mesh->DoesSocketExist(TEXT("head"))) Origin = Mesh->GetSocketLocation(TEXT("head"));
	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	TArray<const FOWSRangeSelector*> Cones;
	FCollisionObjectQueryParams Objects;
	float MaxRange = 0.f;
	for (const FOWSSelectorFunction& Function : SelectorFunctions)
		for (const FOWSRangeSelector& Entry : Function.SelectorStack)
			if (Entry.bEnabled && Entry.Shape == EOWSRangeSelectorShape::Cone)
			{
				Cones.Add(&Entry);
				MaxRange = FMath::Max(MaxRange, FMath::Clamp(Entry.Length, 0.f, 10000.f));
				for (ECollisionChannel Channel : Entry.DetectableObjectTypes) Objects.AddObjectTypesToQuery(Channel);
			}
	auto InView = [&](const FVector& Point)
	{
		const FVector Delta = Point - Origin;
		const double Distance = Delta.Size();
		if (Distance < 1. || Distance > MaxRange) return false;
		for (const FOWSRangeSelector* Cone : Cones)
			if (Distance <= Cone->Length && FVector::DotProduct(Delta / Distance, Forward) >= FMath::Cos(FMath::DegreesToRadians(Cone->HalfAngleDegrees))) return true;
		return false;
	};
	LatestObservation = FString::Printf(TEXT("Observation %s captured UTC %s at button/sensor capture time. Viewpoint is the character's current body-facing head position. Only loaded, collision-visible surfaces were sampled. This is a limited sample, not proof other objects are absent.\n"),
		*ObservationSnapshotId.ToString(), *FDateTime::UtcNow().ToIso8601());
	if (MaxRange <= 0 || Cones.IsEmpty()) return LatestObservation += TEXT("No vision cones enabled.");
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OWSCharacterObservation), true, Character);
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	UNavigationSystemV1* Navigation = bPrepareDestinations ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	const ANavigationData* NavData = Navigation && Movement
		? Navigation->GetNavDataForProps(Movement->GetNavAgentPropertiesRef(), Character->GetActorLocation()) : nullptr;
	int32 DestinationAttempts = 0;
	// Explicit capture only. At most eight projections, forty support probes,
	// eight visibility checks and eight clearance overlaps. Never requests paths
	// or creates navigation data. Missing streamed navigation means no candidate.
	auto PrepareDestination = [&](const FHitResult& VisibleHit)
	{
		if (!bPrepareDestinations || !NavData || !Movement || !Capsule || DestinationAttempts >= 8
			|| !Movement->IsWalkable(VisibleHit) || Cast<APawn>(VisibleHit.GetActor())) return;
		const UPrimitiveComponent* Support = VisibleHit.GetComponent();
		if (!Support || Support->IsSimulatingPhysics()) return;
		const float Radius = Capsule->GetScaledCapsuleRadius();
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		for (const FOWSObservedDestination& Existing : ObservedDestinations)
			if (FVector::DistSquared(Existing.SupportLocation, VisibleHit.ImpactPoint) < FMath::Square(Radius * 2.f)) return;
		++DestinationAttempts;
		FNavLocation NavPoint;
		const FVector Extent(10., 10., 10.);
		if (!Navigation->ProjectPointToNavigation(VisibleHit.ImpactPoint, NavPoint, Extent, NavData)
			|| FVector::DistSquared(NavPoint.Location, VisibleHit.ImpactPoint) > 100.
			|| !InView(NavPoint.Location)) return;
		FHitResult CenterSupport;
		const FVector Offsets[] = { FVector::ZeroVector, FVector(Radius, 0, 0),
			FVector(-Radius, 0, 0), FVector(0, Radius, 0), FVector(0, -Radius, 0) };
		for (int32 I = 0; I < UE_ARRAY_COUNT(Offsets); ++I)
		{
			const FVector Sample = NavPoint.Location + Offsets[I];
			FHitResult Floor;
			if (!World->LineTraceSingleByChannel(Floor, Sample + FVector(0, 0, 10),
				Sample - FVector(0, 0, 10), ECC_Visibility, Params)
				|| Floor.GetComponent() != Support || !Movement->IsWalkable(Floor)) return;
			if (I == 0) CenterSupport = Floor;
		}
		FHitResult Sight;
		if (!InView(CenterSupport.ImpactPoint)
			|| !World->LineTraceSingleByChannel(Sight, Origin,
				CenterSupport.ImpactPoint - FVector(0, 0, 2), ECC_Visibility, Params)
			|| Sight.GetComponent() != Support
			|| FVector::DistSquared(Sight.ImpactPoint, CenterSupport.ImpactPoint) > 4.) return;
		const FVector StandingCenter = CenterSupport.ImpactPoint + FVector(0, 0, HalfHeight + 2.f);
		if (World->OverlapBlockingTestByChannel(StandingCenter, Capsule->GetComponentQuat(),
			Capsule->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius, HalfHeight),
			Params, FCollisionResponseParams(Capsule->GetCollisionResponseToChannels()))) return;
		FOWSObservedDestination& Candidate = ObservedDestinations.AddDefaulted_GetRef();
		Candidate.SnapshotId = ObservationSnapshotId;
		Candidate.CandidateId = ObservedDestinations.Num();
		Candidate.SupportLocation = CenterSupport.ImpactPoint;
		Candidate.CapsuleLocation = StandingCenter;
		Candidate.SupportComponent = VisibleHit.GetComponent();
	};
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, Objects, FCollisionShape::MakeSphere(MaxRange), Params);
	TArray<UPrimitiveComponent*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
		if (UPrimitiveComponent* Component = Overlap.GetComponent())
			if (Component->GetOwner() != Character && Component->IsVisible() && !Component->GetOwner()->IsHidden()) Candidates.AddUnique(Component);
	Candidates.Sort([&](const UPrimitiveComponent& A, const UPrimitiveComponent& B)
	{
		return FVector::DistSquared(Origin, A.Bounds.GetBox().GetClosestPointTo(Origin)) < FVector::DistSquared(Origin, B.Bounds.GetBox().GetClosestPointTo(Origin));
	});
	TSet<TWeakObjectPtr<UPrimitiveComponent>> Seen;
	int32 Traces = 0, Count = 0;
	auto Probe = [&](const FVector& Point)
	{
		if (Traces >= 48 || Count >= 12 || !InView(Point)) return;
		++Traces;
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Origin, Point, ECC_Visibility, Params) || !InView(Hit.ImpactPoint)) return;
		UPrimitiveComponent* Component = Hit.GetComponent();
		AActor* Actor = Hit.GetActor();
		if (!Component || !Actor || Actor->IsHidden() || !Component->IsVisible()) return;
		// Merged road meshes may contain multiple distinct visible standing points.
		PrepareDestination(Hit);
		if (Seen.Contains(Component)) return;
		Seen.Add(Component);
		AwarenessActors.Add(Actor);
		const FVector Delta = Hit.ImpactPoint - Origin;
		const double Distance = Delta.Size() / 100.;
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Delta, Right), FVector::DotProduct(Delta, Forward)));
		FString Kind = Cast<APawn>(Actor) ? TEXT("a character or pawn") : TEXT("an unidentified solid surface");
		if (Actor->ActorHasTag(TEXT("OWS.City.Baked")) && Component->GetFName() == TEXT("Buildings")) Kind = TEXT("a building surface");
		else if (Hit.ImpactNormal.Z > .7) Kind = TEXT("an upward-facing surface; standability not established");
		LatestObservation += FString::Printf(TEXT("Observed surface %d: %s, %.1f metres away, bearing %.0f degrees (negative left, positive right), elevation %.1f metres. %s Business identity, lettering, interiors and exact object identity are unknown.\n"),
			++Count, *Kind, Distance, Yaw, Delta.Z / 100., Distance > 30. ? TEXT("Distant: coarse shape only.") : TEXT("Nearby: generic geometry only."));
	};
	// Volume gathering is primary. A small fixed fan also reaches actual surfaces of
	// merged city meshes whose component origin/bounds do not describe an individual building.
	for (float Yaw : {-60.f, -30.f, 0.f, 30.f, 60.f})
		for (float Pitch : {-20.f, 0.f, 20.f})
			Probe(Origin + (Facing + FRotator(Pitch, Yaw, 0)).Vector() * (FMath::Abs(Yaw) > 30 ? FMath::Min(MaxRange, 3000.f) : MaxRange));
	for (int32 I = 0; I < FMath::Min(Candidates.Num(), 32) && Traces < 48 && Count < 12; ++I)
	{
		const FBox Box = Candidates[I]->Bounds.GetBox();
		Probe(Box.GetClosestPointTo(Origin) + (Box.GetCenter() - Origin).GetSafeNormal() * 2.);
		Probe(Box.GetCenter());
	}
	if (!Count) LatestObservation += TEXT("No identifiable visible surfaces were obtained. Do not invent a scene.");
	if (bPrepareDestinations)
		DestinationPreparationStatus = NavData
			? FString::Printf(TEXT("%d visible endpoint candidates from %d bounded checks. Support, capsule clearance and loaded NavMesh checked at capture time only. Routes and movement are NOT authorized or validated."),
				ObservedDestinations.Num(), DestinationAttempts)
			: TEXT("No endpoint candidates: compatible loaded NavMesh is unavailable. No navigation was created or loaded.");
	return LatestObservation;
}
