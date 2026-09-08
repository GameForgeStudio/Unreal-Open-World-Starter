// OWS #182. Uses the existing modular selector stack, not a second vision system.
FString UOWSSelectorComponent::CaptureObservation()
{
	NextObservationAt = FPlatformTime::Seconds() + .5;
	AwarenessActors.Reset();
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
		*FGuid::NewGuid().ToString(), *FDateTime::UtcNow().ToIso8601());
	if (MaxRange <= 0 || Cones.IsEmpty()) return LatestObservation += TEXT("No vision cones enabled.");
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OWSCharacterObservation), true, Character);
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
		if (!Component || !Actor || Actor->IsHidden() || !Component->IsVisible() || Seen.Contains(Component)) return;
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
	return LatestObservation;
}
