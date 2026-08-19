#include "OWSGenerateOpenWorldLabCommandlet.h"

#include "ActorPartition/PartitionActor.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CoreGlobals.h"
#include "EditorWorldUtils.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/CollisionProfile.h"
#include "Engine/TargetPoint.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeDataAccess.h"
#include "LandscapeEdit.h"
#include "LandscapeEditLayer.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "LandscapeStreamingProxy.h"
#include "LandscapeSubsystem.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"
#include "OWSTestLabEnvironment.h"
#include "ShaderCompiler.h"
#include "RenderingThread.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "WorldPartition/RuntimeHashSet/RuntimePartition.h"
#include "WorldPartition/RuntimeHashSet/RuntimePartitionLHGrid.h"
#include "WorldPartition/RuntimeHashSet/WorldPartitionRuntimeHashSet.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#include "WorldPartition/WorldPartitionHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOWSOpenWorldGenerator, Log, All);

namespace OWSOpenWorld
{
	constexpr int32 Resolution = 8129;
	constexpr int32 QuadsPerSide = 8128;
	constexpr int32 ComponentCountPerSide = 32;
	constexpr int32 SectionsPerComponent = 2;
	constexpr int32 QuadsPerSection = 127;
	constexpr int32 QuadsPerComponent = SectionsPerComponent * QuadsPerSection;
	constexpr int32 LandscapeGridSizeInComponents = 2;
	constexpr int32 LandscapeRegionSizeInComponents = 16;
	constexpr int32 ExpectedStreamingProxyCount = 256;
	constexpr float HalfExtentMeters = 4064.0f;
	constexpr float LandscapeScaleCm = 100.0f;
	constexpr int32 RuntimeCellSizeCm = 51200;
	constexpr int32 RuntimeLoadingRangeCm = 204800;
	constexpr float WorldKillZCm = -150000.0f;
	constexpr uint32 DefaultSeed = 1396788053u; // "SAKU"
	static_assert(Resolution == QuadsPerSide + 1);
	static_assert(QuadsPerSide == ComponentCountPerSide * SectionsPerComponent * QuadsPerSection);
	static_assert(ComponentCountPerSide % LandscapeRegionSizeInComponents == 0);
	static_assert(ComponentCountPerSide % LandscapeGridSizeInComponents == 0);
	static_assert(
		(ComponentCountPerSide / LandscapeGridSizeInComponents)
		* (ComponentCountPerSide / LandscapeGridSizeInComponents)
		== ExpectedStreamingProxyCount);

	const TCHAR* const RequiredMapSuffix = TEXT("_WP");
	const TCHAR* const ExpectedTargetMapPackage = TEXT("/Game/OWSPrototype/Maps/L_OWSTestLab_WP");
	const TCHAR* const GeneratedAssetRoot = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated");
	const TCHAR* const TerrainMaterialPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/M_OWSOpenWorldTerrain");
	const TCHAR* const GrassLayerPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/LI_Grass");
	const TCHAR* const AsphaltLayerPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/LI_Asphalt");
	const TCHAR* const GravelLayerPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/LI_Gravel");
	const TCHAR* const DirtLayerPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/LI_Dirt");
	const TCHAR* const CityMaterialPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/M_CityBlock");
	const TCHAR* const ConcreteMaterialPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/M_Concrete");
	const TCHAR* const MarkerMaterialPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/M_RouteMarker");
	const TCHAR* const BridgeMaterialPackage = TEXT("/Game/OWSPrototype/Terrain/OpenWorldGenerated/M_BridgeDeck");

	const FName GeneratedTag(TEXT("OWS.OpenWorld.Generated"));
	const FName LandscapeTag(TEXT("OWS.OpenWorld.Landscape.V1"));
	const FName CampusManagerTag(TEXT("OWS.OpenWorld.CampusManager"));
	const FName CityBlockTag(TEXT("OWS.OpenWorld.CityBlock"));
	const FName RouteMarkerTag(TEXT("OWS.OpenWorld.RouteMarker"));
	const FName DistrictSignTag(TEXT("OWS.OpenWorld.DistrictSign"));
	const FName BridgeTag(TEXT("OWS.OpenWorld.Bridge"));
	const FName RecoveryAnchorTag(TEXT("OWS.OpenWorld.RecoveryAnchor"));
	const FName MultiplayerStartTag(TEXT("OWS.OpenWorld.MultiplayerStart"));

	enum class ESurfaceClass : uint8
	{
		Grass = 0,
		Dirt = 1,
		Gravel = 2,
		Asphalt = 3
	};

	struct FRoutePoint
	{
		FVector2D XY = FVector2D::ZeroVector;
		float HeightMeters = 0.0f;
		float BankDegrees = 0.0f;
	};

	static float SmootherStep(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
	}

	static uint32 HashCoordinates(uint32 Seed, int32 X, int32 Y)
	{
		uint32 H = Seed ^ (static_cast<uint32>(X) * 0x9E3779B9u) ^ (static_cast<uint32>(Y) * 0x85EBCA6Bu);
		H ^= H >> 16;
		H *= 0x7FEB352Du;
		H ^= H >> 15;
		H *= 0x846CA68Bu;
		return H ^ (H >> 16);
	}

	static FGuid StableActorGuid(const FStringView ActorKey)
	{
		return FGuid::NewDeterministicGuid(ActorKey, DefaultSeed);
	}

	class FHeightfield final
	{
	public:
		FHeightfield(uint32 InSeed, int32 InMinX, int32 InMinY, int32 InMaxX, int32 InMaxY)
			: Seed(InSeed)
			, MinIndexX(FMath::Clamp(InMinX, 0, QuadsPerSide))
			, MinIndexY(FMath::Clamp(InMinY, 0, QuadsPerSide))
			, MaxIndexX(FMath::Clamp(InMaxX, 0, QuadsPerSide))
			, MaxIndexY(FMath::Clamp(InMaxY, 0, QuadsPerSide))
		{
			check(MinIndexX <= MaxIndexX && MinIndexY <= MaxIndexY);
			Width = MaxIndexX - MinIndexX + 1;
			Height = MaxIndexY - MinIndexY + 1;
			const int32 SampleCount = Width * Height;
			HeightsMeters.SetNumUninitialized(SampleCount);
			SurfaceClasses.SetNumZeroed(SampleCount);
		}

		void GenerateBaseTerrain()
		{
			TArray<float> XWaveA;
			TArray<float> XWaveB;
			TArray<float> XWaveC;
			TArray<float> YWaveA;
			TArray<float> YWaveB;
			TArray<float> YWaveC;
			TArray<float> Peak1X;
			TArray<float> Peak1Y;
			TArray<float> Peak2X;
			TArray<float> Peak2Y;
			TArray<float> Peak3X;
			TArray<float> Peak3Y;

			XWaveA.SetNumUninitialized(Width);
			XWaveB.SetNumUninitialized(Width);
			XWaveC.SetNumUninitialized(Width);
			YWaveA.SetNumUninitialized(Height);
			YWaveB.SetNumUninitialized(Height);
			YWaveC.SetNumUninitialized(Height);
			Peak1X.SetNumUninitialized(Width);
			Peak1Y.SetNumUninitialized(Height);
			Peak2X.SetNumUninitialized(Width);
			Peak2Y.SetNumUninitialized(Height);
			Peak3X.SetNumUninitialized(Width);
			Peak3Y.SetNumUninitialized(Height);

			const float SeedPhase = static_cast<float>(Seed % 10000u) / 10000.0f * 2.0f * PI;
			for (int32 I = 0; I < Width; ++I)
			{
				const float C = CoordinateMeters(MinIndexX + I);
				XWaveA[I] = FMath::Sin(C * 0.0019f + SeedPhase);
				XWaveB[I] = FMath::Cos(C * 0.0031f - SeedPhase * 0.7f);
				XWaveC[I] = FMath::Sin(C * 0.0067f + 0.31f);
				Peak1X[I] = Gaussian(C, -2920.0f, 760.0f);
				Peak2X[I] = Gaussian(C, -2050.0f, 860.0f);
				Peak3X[I] = Gaussian(C, -1250.0f, 620.0f);
			}
			for (int32 I = 0; I < Height; ++I)
			{
				const float C = CoordinateMeters(MinIndexY + I);
				YWaveA[I] = FMath::Cos(C * 0.0016f - SeedPhase * 0.5f);
				YWaveB[I] = FMath::Sin(C * 0.0037f + SeedPhase * 0.9f);
				YWaveC[I] = FMath::Cos(C * 0.0059f - 0.73f);
				Peak1Y[I] = Gaussian(C, 2750.0f, 690.0f);
				Peak2Y[I] = Gaussian(C, 3320.0f, 780.0f);
				Peak3Y[I] = Gaussian(C, 2250.0f, 730.0f);
			}

			for (int32 Y = 0; Y < Height; ++Y)
			{
				const int32 Row = Y * Width;
				for (int32 X = 0; X < Width; ++X)
				{
					float GeneratedHeight = -2.0f;
					GeneratedHeight += 2.6f * XWaveA[X] * YWaveA[Y];
					GeneratedHeight += 1.5f * XWaveB[X] * YWaveB[Y];
					GeneratedHeight += 0.75f * XWaveC[X] * YWaveC[Y];
					GeneratedHeight += 118.0f * Peak1X[X] * Peak1Y[Y];
					GeneratedHeight += 92.0f * Peak2X[X] * Peak2Y[Y];
					GeneratedHeight += 64.0f * Peak3X[X] * Peak3Y[Y];
					HeightsMeters[Row + X] = FMath::Clamp(GeneratedHeight, -12.0f, 200.0f);
				}
			}
		}

		void StampRectangle(
			const FVector2D& Center,
			const FVector2D& HalfExtents,
			float FalloffMeters,
			float TargetHeightMeters,
			ESurfaceClass Surface,
			bool bPaintCore)
		{
			const int32 MinX = FMath::Max(ToIndexFloor(Center.X - HalfExtents.X - FalloffMeters), MinIndexX);
			const int32 MaxX = FMath::Min(ToIndexCeil(Center.X + HalfExtents.X + FalloffMeters), MaxIndexX);
			const int32 MinY = FMath::Max(ToIndexFloor(Center.Y - HalfExtents.Y - FalloffMeters), MinIndexY);
			const int32 MaxY = FMath::Min(ToIndexCeil(Center.Y + HalfExtents.Y + FalloffMeters), MaxIndexY);
			if (MinX > MaxX || MinY > MaxY)
			{
				return;
			}

			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				const float WorldY = CoordinateMeters(Y);
				for (int32 X = MinX; X <= MaxX; ++X)
				{
					const float WorldX = CoordinateMeters(X);
					const float OutsideX = FMath::Max(FMath::Abs(WorldX - Center.X) - HalfExtents.X, 0.0f);
					const float OutsideY = FMath::Max(FMath::Abs(WorldY - Center.Y) - HalfExtents.Y, 0.0f);
					const float OutsideDistance = FVector2D(OutsideX, OutsideY).Size();
					if (OutsideDistance > FalloffMeters)
					{
						continue;
					}

					const float Alpha = OutsideDistance <= KINDA_SMALL_NUMBER
						? 1.0f
						: SmootherStep(1.0f - OutsideDistance / FalloffMeters);
					const int32 Index = LocalIndex(X, Y);
					HeightsMeters[Index] = FMath::Lerp(HeightsMeters[Index], TargetHeightMeters, Alpha);
					if (bPaintCore && OutsideDistance <= KINDA_SMALL_NUMBER)
					{
						PromoteSurface(Index, Surface);
					}
				}
			}
		}

		void StampRoute(
			const TArray<FRoutePoint>& Points,
			float CoreHalfWidthMeters,
			float OuterHalfWidthMeters,
			ESurfaceClass CoreSurface,
			ESurfaceClass ShoulderSurface,
			float Strength = 1.0f)
		{
			check(OuterHalfWidthMeters > CoreHalfWidthMeters);
			for (int32 SegmentIndex = 0; SegmentIndex + 1 < Points.Num(); ++SegmentIndex)
			{
				const FRoutePoint& A = Points[SegmentIndex];
				const FRoutePoint& B = Points[SegmentIndex + 1];
				const FVector2D Delta = B.XY - A.XY;
				const float LengthSquared = Delta.SizeSquared();
				if (LengthSquared < 1.0f)
				{
					continue;
				}

				const float Length = FMath::Sqrt(LengthSquared);
				const FVector2D Tangent = Delta / Length;
				const FVector2D Normal(-Tangent.Y, Tangent.X);
				const int32 MinX = FMath::Max(ToIndexFloor(FMath::Min(A.XY.X, B.XY.X) - OuterHalfWidthMeters), MinIndexX);
				const int32 MaxX = FMath::Min(ToIndexCeil(FMath::Max(A.XY.X, B.XY.X) + OuterHalfWidthMeters), MaxIndexX);
				const int32 MinY = FMath::Max(ToIndexFloor(FMath::Min(A.XY.Y, B.XY.Y) - OuterHalfWidthMeters), MinIndexY);
				const int32 MaxY = FMath::Min(ToIndexCeil(FMath::Max(A.XY.Y, B.XY.Y) + OuterHalfWidthMeters), MaxIndexY);
				if (MinX > MaxX || MinY > MaxY)
				{
					continue;
				}

				for (int32 Y = MinY; Y <= MaxY; ++Y)
				{
					const float WorldY = CoordinateMeters(Y);
					for (int32 X = MinX; X <= MaxX; ++X)
					{
						const FVector2D Sample(CoordinateMeters(X), WorldY);
						const float T = FMath::Clamp(FVector2D::DotProduct(Sample - A.XY, Delta) / LengthSquared, 0.0f, 1.0f);
						const FVector2D Closest = A.XY + Delta * T;
						const float SignedDistance = FVector2D::DotProduct(Sample - Closest, Normal);
						const float Distance = FMath::Abs(SignedDistance);
						if (Distance > OuterHalfWidthMeters)
						{
							continue;
						}

						const float Blend = Distance <= CoreHalfWidthMeters
							? 1.0f
							: SmootherStep(1.0f - (Distance - CoreHalfWidthMeters) / (OuterHalfWidthMeters - CoreHalfWidthMeters));
						const float BankDegrees = FMath::Lerp(A.BankDegrees, B.BankDegrees, T);
						const float BankOffset = FMath::Tan(FMath::DegreesToRadians(BankDegrees)) * SignedDistance;
						const float TargetHeight = FMath::Lerp(A.HeightMeters, B.HeightMeters, T) + BankOffset;
						const int32 Index = LocalIndex(X, Y);
						HeightsMeters[Index] = FMath::Lerp(HeightsMeters[Index], TargetHeight, Blend * Strength);
						if (Distance <= CoreHalfWidthMeters)
						{
							PromoteSurface(Index, CoreSurface);
						}
						else if (Blend > 0.35f)
						{
							PromoteSurface(Index, ShoulderSurface);
						}
					}
				}
			}
		}

		void AddPerimeterCatchApronAndBerm()
		{
			constexpr float ApronWidthMeters = 180.0f;
			constexpr float BermWidthMeters = 90.0f;
			for (int32 Y = MinIndexY; Y <= MaxIndexY; ++Y)
			{
				const float EdgeY = FMath::Min(static_cast<float>(Y), static_cast<float>(QuadsPerSide - Y));
				for (int32 X = MinIndexX; X <= MaxIndexX; ++X)
				{
					const float EdgeX = FMath::Min(static_cast<float>(X), static_cast<float>(QuadsPerSide - X));
					const float EdgeDistance = FMath::Min(EdgeX, EdgeY);
					if (EdgeDistance >= ApronWidthMeters)
					{
						continue;
					}

					const int32 Index = LocalIndex(X, Y);
					const float ApronAlpha = SmootherStep(1.0f - EdgeDistance / ApronWidthMeters);
					const float BermAlpha = EdgeDistance < BermWidthMeters
						? SmootherStep(1.0f - EdgeDistance / BermWidthMeters)
						: 0.0f;
					const float CatchHeight = -4.0f + 24.0f * BermAlpha;
					HeightsMeters[Index] = FMath::Lerp(HeightsMeters[Index], CatchHeight, ApronAlpha);
					PromoteSurface(Index, ESurfaceClass::Gravel);
				}
			}
		}

		float SampleHeightMeters(const FVector2D& XY) const
		{
			const float FractionalX = FMath::Clamp(XY.X + HalfExtentMeters, 0.0f, static_cast<float>(QuadsPerSide));
			const float FractionalY = FMath::Clamp(XY.Y + HalfExtentMeters, 0.0f, static_cast<float>(QuadsPerSide));
			const int32 X0 = FMath::FloorToInt(FractionalX);
			const int32 Y0 = FMath::FloorToInt(FractionalY);
			const int32 X1 = FMath::Min(X0 + 1, QuadsPerSide);
			const int32 Y1 = FMath::Min(Y0 + 1, QuadsPerSide);
			const float TX = FractionalX - X0;
			const float TY = FractionalY - Y0;
			check(Contains(X0, Y0) && Contains(X1, Y1));
			const float H0 = FMath::Lerp(GetHeightMeters(X0, Y0), GetHeightMeters(X1, Y0), TX);
			const float H1 = FMath::Lerp(GetHeightMeters(X0, Y1), GetHeightMeters(X1, Y1), TX);
			return FMath::Lerp(H0, H1, TY);
		}

		ESurfaceClass SampleSurface(const FVector2D& XY) const
		{
			const int32 X = FMath::Clamp(FMath::RoundToInt(XY.X + HalfExtentMeters), 0, QuadsPerSide);
			const int32 Y = FMath::Clamp(FMath::RoundToInt(XY.Y + HalfExtentMeters), 0, QuadsPerSide);
			check(Contains(X, Y));
			return static_cast<ESurfaceClass>(SurfaceClasses[LocalIndex(X, Y)]);
		}

		void BuildHeightData(int32 WriteMinX, int32 WriteMinY, int32 WriteMaxX, int32 WriteMaxY, TArray<uint16>& OutHeights) const
		{
			CheckWriteBounds(WriteMinX, WriteMinY, WriteMaxX, WriteMaxY);
			const int32 WriteWidth = WriteMaxX - WriteMinX + 1;
			const int32 WriteHeight = WriteMaxY - WriteMinY + 1;
			OutHeights.SetNumUninitialized(WriteWidth * WriteHeight);
			for (int32 Y = WriteMinY; Y <= WriteMaxY; ++Y)
			{
				for (int32 X = WriteMinX; X <= WriteMaxX; ++X)
				{
					const int32 OutputIndex = (Y - WriteMinY) * WriteWidth + (X - WriteMinX);
					OutHeights[OutputIndex] = EncodeHeight(GetHeightMeters(X, Y));
				}
			}
		}

		void BuildNormalData(int32 WriteMinX, int32 WriteMinY, int32 WriteMaxX, int32 WriteMaxY, TArray<uint16>& OutNormals) const
		{
			CheckWriteBounds(WriteMinX, WriteMinY, WriteMaxX, WriteMaxY);
			const int32 WriteWidth = WriteMaxX - WriteMinX + 1;
			const int32 WriteHeight = WriteMaxY - WriteMinY + 1;
			OutNormals.SetNumUninitialized(WriteWidth * WriteHeight);
			for (int32 Y = WriteMinY; Y <= WriteMaxY; ++Y)
			{
				for (int32 X = WriteMinX; X <= WriteMaxX; ++X)
				{
					const float Left = GetHeightMeters(FMath::Max(X - 1, MinIndexX), Y);
					const float Right = GetHeightMeters(FMath::Min(X + 1, MaxIndexX), Y);
					const float Down = GetHeightMeters(X, FMath::Max(Y - 1, MinIndexY));
					const float Up = GetHeightMeters(X, FMath::Min(Y + 1, MaxIndexY));
					const FVector Normal(-(Right - Left) * 0.5f, -(Up - Down) * 0.5f, 1.0f);
					const FVector UnitNormal = Normal.GetSafeNormal();
					const uint8 PackedX = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(127.5f * (UnitNormal.X + 1.0f)), 0, 255));
					const uint8 PackedY = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(127.5f * (UnitNormal.Y + 1.0f)), 0, 255));
					const int32 OutputIndex = (Y - WriteMinY) * WriteWidth + (X - WriteMinX);
					OutNormals[OutputIndex] = static_cast<uint16>((static_cast<uint16>(PackedX) << 8) | PackedY);
				}
			}
		}

		void BuildSurfaceData(int32 WriteMinX, int32 WriteMinY, int32 WriteMaxX, int32 WriteMaxY, ESurfaceClass Surface, TArray<uint8>& OutWeights) const
		{
			CheckWriteBounds(WriteMinX, WriteMinY, WriteMaxX, WriteMaxY);
			const int32 WriteWidth = WriteMaxX - WriteMinX + 1;
			const int32 WriteHeight = WriteMaxY - WriteMinY + 1;
			OutWeights.SetNumUninitialized(WriteWidth * WriteHeight);
			for (int32 Y = WriteMinY; Y <= WriteMaxY; ++Y)
			{
				for (int32 X = WriteMinX; X <= WriteMaxX; ++X)
				{
					const int32 OutputIndex = (Y - WriteMinY) * WriteWidth + (X - WriteMinX);
					OutWeights[OutputIndex] = static_cast<ESurfaceClass>(SurfaceClasses[LocalIndex(X, Y)]) == Surface ? 255 : 0;
				}
			}
		}

	private:
		bool Contains(int32 X, int32 Y) const
		{
			return X >= MinIndexX && X <= MaxIndexX && Y >= MinIndexY && Y <= MaxIndexY;
		}

		int32 LocalIndex(int32 X, int32 Y) const
		{
			check(Contains(X, Y));
			return (Y - MinIndexY) * Width + (X - MinIndexX);
		}

		float GetHeightMeters(int32 X, int32 Y) const
		{
			return HeightsMeters[LocalIndex(X, Y)];
		}

		void CheckWriteBounds(int32 WriteMinX, int32 WriteMinY, int32 WriteMaxX, int32 WriteMaxY) const
		{
			check(WriteMinX <= WriteMaxX && WriteMinY <= WriteMaxY);
			check(Contains(WriteMinX, WriteMinY) && Contains(WriteMaxX, WriteMaxY));
		}

		static uint16 EncodeHeight(float HeightMeters)
		{
			const int32 EncodedHeight = FMath::RoundToInt(32768.0f + FMath::Clamp(HeightMeters, -255.0f, 255.0f) * 128.0f);
			return static_cast<uint16>(FMath::Clamp(EncodedHeight, 0, 65535));
		}

		static float CoordinateMeters(int32 Index)
		{
			return static_cast<float>(Index) - HalfExtentMeters;
		}

		static int32 ToIndexFloor(float Coordinate)
		{
			return FMath::Clamp(FMath::FloorToInt(Coordinate + HalfExtentMeters), 0, QuadsPerSide);
		}

		static int32 ToIndexCeil(float Coordinate)
		{
			return FMath::Clamp(FMath::CeilToInt(Coordinate + HalfExtentMeters), 0, QuadsPerSide);
		}

		static float Gaussian(float Value, float Mean, float Sigma)
		{
			const float Normalized = (Value - Mean) / Sigma;
			return FMath::Exp(-0.5f * Normalized * Normalized);
		}

		void PromoteSurface(int32 Index, ESurfaceClass Surface)
		{
			SurfaceClasses[Index] = FMath::Max(SurfaceClasses[Index], static_cast<uint8>(Surface));
		}

		uint32 Seed;
		int32 MinIndexX;
		int32 MinIndexY;
		int32 MaxIndexX;
		int32 MaxIndexY;
		int32 Width = 0;
		int32 Height = 0;
		TArray<float> HeightsMeters;
		TArray<uint8> SurfaceClasses;
	};

	static TArray<FRoutePoint> BuildHighSpeedOval()
	{
		TArray<FRoutePoint> Route;
		Route.Reserve(70);
		Route.Add({ FVector2D(-3000.0f, -1450.0f), -3.0f, 0.0f });
		Route.Add({ FVector2D(3000.0f, -1450.0f), -3.0f, 0.0f });

		for (int32 Step = 1; Step <= 32; ++Step)
		{
			const float Angle = FMath::Lerp(90.0f, -90.0f, static_cast<float>(Step) / 32.0f);
			const float Radians = FMath::DegreesToRadians(Angle);
			Route.Add({
				FVector2D(3000.0f + 550.0f * FMath::Cos(Radians), -2000.0f + 550.0f * FMath::Sin(Radians)),
				-3.0f,
				8.0f
			});
		}

		Route.Add({ FVector2D(-3000.0f, -2550.0f), -3.0f, 0.0f });
		for (int32 Step = 1; Step <= 32; ++Step)
		{
			const float Angle = FMath::Lerp(-90.0f, -270.0f, static_cast<float>(Step) / 32.0f);
			const float Radians = FMath::DegreesToRadians(Angle);
			Route.Add({
				FVector2D(-3000.0f + 550.0f * FMath::Cos(Radians), -2000.0f + 550.0f * FMath::Sin(Radians)),
				-3.0f,
				8.0f
			});
		}
		return Route;
	}

	static TArray<FRoutePoint> BuildMountainPass()
	{
		return {
			{ FVector2D(-500.0f, 500.0f), 1.0f, 0.0f },
			{ FVector2D(-1100.0f, 900.0f), 18.0f, 2.0f },
			{ FVector2D(-1900.0f, 1300.0f), 52.0f, -4.0f },
			{ FVector2D(-2900.0f, 1900.0f), 105.0f, 6.0f },
			{ FVector2D(-2450.0f, 2850.0f), 150.0f, -7.0f },
			{ FVector2D(-1400.0f, 3450.0f), 112.0f, 5.0f },
			{ FVector2D(-700.0f, 3000.0f), 60.0f, -3.0f }
		};
	}

	static TArray<FRoutePoint> BuildOffRoadLoop()
	{
		return {
			{ FVector2D(500.0f, 500.0f), 5.0f, 0.0f },
			{ FVector2D(1400.0f, 100.0f), 12.0f, 0.0f },
			{ FVector2D(2500.0f, 200.0f), 28.0f, 0.0f },
			{ FVector2D(3500.0f, 500.0f), 48.0f, 0.0f },
			{ FVector2D(3600.0f, 0.0f), 24.0f, 0.0f },
			{ FVector2D(3000.0f, -600.0f), 15.0f, 0.0f },
			{ FVector2D(2000.0f, -900.0f), 2.0f, 0.0f },
			{ FVector2D(1000.0f, -600.0f), -2.0f, 0.0f },
			{ FVector2D(500.0f, -200.0f), 1.0f, 0.0f },
			{ FVector2D(500.0f, 500.0f), 5.0f, 0.0f }
		};
	}

	static TArray<FRoutePoint> BuildHandlingLoop()
	{
		return {
			{ FVector2D(-500.0f, 400.0f), 1.0f, 0.0f },
			{ FVector2D(-1300.0f, 450.0f), 5.0f, 4.0f },
			{ FVector2D(-2200.0f, 100.0f), 1.5f, -6.0f },
			{ FVector2D(-2600.0f, -650.0f), -2.5f, 7.0f },
			{ FVector2D(-1800.0f, -1050.0f), 2.0f, -5.0f },
			{ FVector2D(-900.0f, -700.0f), -1.0f, 6.0f },
			{ FVector2D(-500.0f, 0.0f), -0.5f, -3.0f },
			{ FVector2D(-500.0f, 400.0f), 1.0f, 0.0f }
		};
	}

	static void StampJumpCorridor(FHeightfield& Heightfield, float LaneY, float AngleDegrees, int32 LaneIndex)
	{
		const float BaseHeight = -5.0f;
		const float RampStartX = -1050.0f;
		const float LipX = -970.0f;
		const float LipHeight = BaseHeight + FMath::Tan(FMath::DegreesToRadians(AngleDegrees)) * (LipX - RampStartX);
		const float LandingStartX = LaneIndex == 0 ? -650.0f : (LaneIndex == 1 ? -400.0f : 0.0f);
		const float LandingCrown = LaneIndex == 0 ? 1.0f : (LaneIndex == 1 ? 11.0f : 24.0f);
		const float LandingEndX = LaneIndex == 0 ? -350.0f : (LaneIndex == 1 ? 0.0f : 420.0f);

		Heightfield.StampRoute(
			{
				{ FVector2D(-3550.0f, LaneY), BaseHeight, 0.0f },
				{ FVector2D(RampStartX, LaneY), BaseHeight, 0.0f }
			},
			8.0f,
			20.0f,
			ESurfaceClass::Asphalt,
			ESurfaceClass::Gravel);

		Heightfield.StampRoute(
			{
				{ FVector2D(RampStartX, LaneY), BaseHeight, 0.0f },
				{ FVector2D(LipX, LaneY), LipHeight, 0.0f }
			},
			8.0f,
			20.0f,
			ESurfaceClass::Asphalt,
			ESurfaceClass::Gravel);

		Heightfield.StampRoute(
			{
				{ FVector2D(LandingStartX, LaneY), LandingCrown, 0.0f },
				{ FVector2D(LandingEndX, LaneY), BaseHeight, 0.0f },
				{ FVector2D(950.0f, LaneY), BaseHeight, 0.0f }
			},
			10.0f,
			24.0f,
			ESurfaceClass::Asphalt,
			ESurfaceClass::Gravel);

		Heightfield.StampRoute(
			{
				{ FVector2D(950.0f, LaneY), BaseHeight, 0.0f },
				{ FVector2D(3500.0f, LaneY), BaseHeight, 0.0f }
			},
			14.0f,
			34.0f,
			ESurfaceClass::Gravel,
			ESurfaceClass::Gravel);
	}

	static void BuildTerrainLayout(FHeightfield& Heightfield)
	{
		Heightfield.GenerateBaseTerrain();

		// The existing 180 m x 120 m runtime-built foundation has its top at Z=0.
		// Keep Landscape collision 50 cm beneath it, with a broad C2 transition.
		Heightfield.StampRectangle(
			FVector2D(15.0f, 0.0f),
			FVector2D(90.0f, 60.0f),
			420.0f,
			-0.5f,
			ESurfaceClass::Grass,
			false);

		// A broad, flat urban mesa keeps city-block collision predictable.
		Heightfield.StampRectangle(
			FVector2D(2150.0f, 2150.0f),
			FVector2D(1450.0f, 1450.0f),
			260.0f,
			8.0f,
			ESurfaceClass::Grass,
			false);

		const TArray<FRoutePoint> Oval = BuildHighSpeedOval();
		Heightfield.StampRoute(Oval, 11.0f, 24.0f, ESurfaceClass::Asphalt, ESurfaceClass::Gravel);

		const float UrbanStreets[] = { 1000.0f, 1450.0f, 1900.0f, 2350.0f, 2800.0f, 3250.0f };
		for (float Street : UrbanStreets)
		{
			const float HalfWidth = FMath::IsNearlyEqual(Street, 1900.0f) ? 14.0f : 9.0f;
			Heightfield.StampRoute(
				{ { FVector2D(800.0f, Street), 8.0f, 0.0f }, { FVector2D(3500.0f, Street), 8.0f, 0.0f } },
				HalfWidth,
				HalfWidth + 8.0f,
				ESurfaceClass::Asphalt,
				ESurfaceClass::Gravel);
			Heightfield.StampRoute(
				{ { FVector2D(Street, 800.0f), 8.0f, 0.0f }, { FVector2D(Street, 3500.0f), 8.0f, 0.0f } },
				HalfWidth,
				HalfWidth + 8.0f,
				ESurfaceClass::Asphalt,
				ESurfaceClass::Gravel);
		}

		Heightfield.StampRoute(BuildMountainPass(), 6.0f, 16.0f, ESurfaceClass::Asphalt, ESurfaceClass::Gravel);
		Heightfield.StampRoute(BuildOffRoadLoop(), 4.0f, 13.0f, ESurfaceClass::Dirt, ESurfaceClass::Gravel, 0.82f);
		Heightfield.StampRoute(BuildHandlingLoop(), 6.0f, 15.0f, ESurfaceClass::Asphalt, ESurfaceClass::Gravel);

		// Campus access routes tie the retained lab district into every major test zone.
		Heightfield.StampRoute(
			{
				{ FVector2D(80.0f, 0.0f), -0.5f, 0.0f },
				{ FVector2D(450.0f, 250.0f), 1.0f, 0.0f },
				{ FVector2D(800.0f, 800.0f), 8.0f, 0.0f },
				{ FVector2D(1000.0f, 1000.0f), 8.0f, 0.0f }
			},
			7.0f,
			18.0f,
			ESurfaceClass::Asphalt,
			ESurfaceClass::Gravel);
		Heightfield.StampRoute(
			{
				{ FVector2D(-80.0f, -20.0f), -0.5f, 0.0f },
				{ FVector2D(-500.0f, -350.0f), -1.0f, 0.0f },
				{ FVector2D(-1200.0f, -1000.0f), -2.0f, 0.0f },
				{ FVector2D(-1800.0f, -1450.0f), -3.0f, 0.0f }
			},
			7.0f,
			18.0f,
			ESurfaceClass::Asphalt,
			ESurfaceClass::Gravel);

		StampJumpCorridor(Heightfield, -2950.0f, 8.0f, 0);
		StampJumpCorridor(Heightfield, -3200.0f, 15.0f, 1);
		StampJumpCorridor(Heightfield, -3450.0f, 25.0f, 2);

		Heightfield.AddPerimeterCatchApronAndBerm();
	}

	static void SampleGeneratedTerrain(uint32 Seed, const FVector2D& XY, float& OutHeightMeters, ESurfaceClass& OutSurface)
	{
		const float FractionalX = FMath::Clamp(XY.X + HalfExtentMeters, 0.0f, static_cast<float>(QuadsPerSide));
		const float FractionalY = FMath::Clamp(XY.Y + HalfExtentMeters, 0.0f, static_cast<float>(QuadsPerSide));
		const int32 X0 = FMath::FloorToInt(FractionalX);
		const int32 Y0 = FMath::FloorToInt(FractionalY);
		const int32 X1 = FMath::Min(X0 + 1, QuadsPerSide);
		const int32 Y1 = FMath::Min(Y0 + 1, QuadsPerSide);
		FHeightfield Heightfield(Seed, X0, Y0, X1, Y1);
		BuildTerrainLayout(Heightfield);
		OutHeightMeters = Heightfield.SampleHeightMeters(XY);
		OutSurface = Heightfield.SampleSurface(XY);
	}

	static float SampleGeneratedHeightMeters(uint32 Seed, const FVector2D& XY)
	{
		float HeightMeters = 0.0f;
		ESurfaceClass Surface = ESurfaceClass::Grass;
		SampleGeneratedTerrain(Seed, XY, HeightMeters, Surface);
		return HeightMeters;
	}

	template<typename AssetType>
	static AssetType* CreateAsset(const FString& PackageName, TArray<UPackage*>& OutPackages)
	{
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		const FString ObjectName = FPackageName::GetLongPackageAssetName(PackageName);
		AssetType* Asset = NewObject<AssetType>(Package, *ObjectName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		OutPackages.Add(Package);
		return Asset;
	}

	static void ConnectMaterialProperty(UMaterial* Material, UMaterialExpression* Expression, EMaterialProperty Property)
	{
		FExpressionInput* Input = Material->GetExpressionInputForProperty(Property);
		check(Input);
		Input->Expression = Expression;
	}

	static UMaterial* CreateSolidMaterial(
		const FString& PackageName,
		const FLinearColor& Color,
		float Roughness,
		TArray<UPackage*>& OutPackages)
	{
		UMaterial* Material = CreateAsset<UMaterial>(PackageName, OutPackages);
		if (!Material)
		{
			return nullptr;
		}

		UMaterialExpressionVectorParameter* ColorExpression = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorExpression->ParameterName = TEXT("Color");
		ColorExpression->DefaultValue = Color;
		ColorExpression->MaterialExpressionEditorX = -260;
		Material->GetExpressionCollection().AddExpression(ColorExpression);
		ConnectMaterialProperty(Material, ColorExpression, MP_BaseColor);

		UMaterialExpressionConstant* RoughnessExpression = NewObject<UMaterialExpressionConstant>(Material);
		RoughnessExpression->R = Roughness;
		RoughnessExpression->MaterialExpressionEditorX = -260;
		RoughnessExpression->MaterialExpressionEditorY = 100;
		Material->GetExpressionCollection().AddExpression(RoughnessExpression);
		ConnectMaterialProperty(Material, RoughnessExpression, MP_Roughness);

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return Material;
	}

	static UMaterial* CreateTerrainMaterial(TArray<UPackage*>& OutPackages)
	{
		UMaterial* Material = CreateAsset<UMaterial>(TerrainMaterialPackage, OutPackages);
		if (!Material)
		{
			return nullptr;
		}

		UMaterialExpressionLandscapeLayerBlend* Blend = NewObject<UMaterialExpressionLandscapeLayerBlend>(Material);
		Blend->MaterialExpressionEditorX = -300;
		const struct FLayerDefinition
		{
			FName Name;
			FVector Color;
			float PreviewWeight;
		} Definitions[] = {
			{ TEXT("Grass"), FVector(0.07, 0.24, 0.08), 1.0f },
			{ TEXT("Asphalt"), FVector(0.035, 0.04, 0.045), 0.0f },
			{ TEXT("Gravel"), FVector(0.24, 0.22, 0.18), 0.0f },
			{ TEXT("Dirt"), FVector(0.23, 0.10, 0.045), 0.0f }
		};
		for (const FLayerDefinition& Definition : Definitions)
		{
			FLayerBlendInput& Layer = Blend->Layers.AddDefaulted_GetRef();
			Layer.LayerName = Definition.Name;
			Layer.BlendType = LB_WeightBlend;
			Layer.PreviewWeight = Definition.PreviewWeight;
			Layer.ConstLayerInput = Definition.Color;
		}
		Material->GetExpressionCollection().AddExpression(Blend);
		ConnectMaterialProperty(Material, Blend, MP_BaseColor);

		UMaterialExpressionConstant* Roughness = NewObject<UMaterialExpressionConstant>(Material);
		Roughness->R = 0.88f;
		Roughness->MaterialExpressionEditorX = -300;
		Roughness->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(Roughness);
		ConnectMaterialProperty(Material, Roughness, MP_Roughness);

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return Material;
	}

	static ULandscapeLayerInfoObject* CreateLayerInfo(
		const FString& PackageName,
		FName LayerName,
		const FLinearColor& DebugColor,
		TArray<UPackage*>& OutPackages)
	{
		ULandscapeLayerInfoObject* LayerInfo = CreateAsset<ULandscapeLayerInfoObject>(PackageName, OutPackages);
		if (!LayerInfo)
		{
			return nullptr;
		}

		LayerInfo->SetLayerName(LayerName, false);
		LayerInfo->SetBlendMethod(ELandscapeTargetLayerBlendMethod::FinalWeightBlending, false);
		LayerInfo->SetLayerUsageDebugColor(DebugColor, false, EPropertyChangeType::ValueSet);
		LayerInfo->MarkPackageDirty();
		return LayerInfo;
	}

	struct FGeneratedAssets
	{
		UMaterial* TerrainMaterial = nullptr;
		UMaterial* CityMaterial = nullptr;
		UMaterial* ConcreteMaterial = nullptr;
		UMaterial* MarkerMaterial = nullptr;
		UMaterial* BridgeMaterial = nullptr;
		ULandscapeLayerInfoObject* GrassLayer = nullptr;
		ULandscapeLayerInfoObject* AsphaltLayer = nullptr;
		ULandscapeLayerInfoObject* GravelLayer = nullptr;
		ULandscapeLayerInfoObject* DirtLayer = nullptr;
		TArray<UPackage*> Packages;
	};

	static bool CreateGeneratedAssets(FGeneratedAssets& OutAssets)
	{
		OutAssets.TerrainMaterial = CreateTerrainMaterial(OutAssets.Packages);
		OutAssets.CityMaterial = CreateSolidMaterial(CityMaterialPackage, FLinearColor(0.10f, 0.17f, 0.25f), 0.72f, OutAssets.Packages);
		OutAssets.ConcreteMaterial = CreateSolidMaterial(ConcreteMaterialPackage, FLinearColor(0.28f, 0.30f, 0.32f), 0.84f, OutAssets.Packages);
		OutAssets.MarkerMaterial = CreateSolidMaterial(MarkerMaterialPackage, FLinearColor(1.0f, 0.12f, 0.02f), 0.55f, OutAssets.Packages);
		OutAssets.BridgeMaterial = CreateSolidMaterial(BridgeMaterialPackage, FLinearColor(0.15f, 0.18f, 0.21f), 0.68f, OutAssets.Packages);
		OutAssets.GrassLayer = CreateLayerInfo(GrassLayerPackage, TEXT("Grass"), FLinearColor::Green, OutAssets.Packages);
		OutAssets.AsphaltLayer = CreateLayerInfo(AsphaltLayerPackage, TEXT("Asphalt"), FLinearColor::Black, OutAssets.Packages);
		OutAssets.GravelLayer = CreateLayerInfo(GravelLayerPackage, TEXT("Gravel"), FLinearColor(0.5f, 0.5f, 0.5f), OutAssets.Packages);
		OutAssets.DirtLayer = CreateLayerInfo(DirtLayerPackage, TEXT("Dirt"), FLinearColor(0.35f, 0.12f, 0.03f), OutAssets.Packages);

		return OutAssets.TerrainMaterial
			&& OutAssets.CityMaterial
			&& OutAssets.ConcreteMaterial
			&& OutAssets.MarkerMaterial
			&& OutAssets.BridgeMaterial
			&& OutAssets.GrassLayer
			&& OutAssets.AsphaltLayer
			&& OutAssets.GravelLayer
			&& OutAssets.DirtLayer;
	}

	static void AddGeneratedTags(AActor* Actor, const FName CategoryTag)
	{
		Actor->Tags.AddUnique(GeneratedTag);
		Actor->Tags.AddUnique(CategoryTag);
		Actor->SetIsSpatiallyLoaded(true);
	}

	static AStaticMeshActor* SpawnBox(
		UWorld* World,
		UStaticMesh* CubeMesh,
		UMaterialInterface* Material,
		const FString& ActorName,
		const FVector& CenterMeters,
		const FVector& DimensionsMeters,
		const FRotator& Rotation,
		const FName CategoryTag,
		const FName Folder)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = FName(*ActorName);
		SpawnParameters.OverrideActorGuid = StableActorGuid(ActorName);
		SpawnParameters.NameMode =
			FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
		SpawnParameters.ObjectFlags |= RF_Transactional;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
			CenterMeters * 100.0f,
			Rotation,
			SpawnParameters);
		if (!Actor)
		{
			return nullptr;
		}

		Actor->SetActorLabel(ActorName);
		Actor->SetFolderPath(Folder);
		AddGeneratedTags(Actor, CategoryTag);
		Actor->SetActorScale3D(DimensionsMeters);
		UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->SetStaticMesh(CubeMesh);
		MeshComponent->SetMaterial(0, Material);
		MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		MeshComponent->SetGenerateOverlapEvents(false);
		Actor->MarkPackageDirty();
		return Actor;
	}

	static AStaticMeshActor* SpawnSegment(
		UWorld* World,
		UStaticMesh* CubeMesh,
		UMaterialInterface* Material,
		const FString& ActorName,
		const FVector& StartMeters,
		const FVector& EndMeters,
		float WidthMeters,
		float ThicknessMeters,
		const FName CategoryTag,
		const FName Folder)
	{
		const FVector Delta = EndMeters - StartMeters;
		const float LengthMeters = Delta.Size();
		if (LengthMeters < KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		return SpawnBox(
			World,
			CubeMesh,
			Material,
			ActorName,
			(StartMeters + EndMeters) * 0.5f,
			FVector(LengthMeters, WidthMeters, ThicknessMeters),
			Delta.Rotation(),
			CategoryTag,
			Folder);
	}

	static bool SpawnDistrictSign(
		UWorld* World,
		UStaticMesh* CubeMesh,
		UMaterialInterface* BackingMaterial,
		const FString& Identifier,
		const FString& Text,
		const FVector& LocationMeters,
		float YawDegrees)
	{
		const FName Folder(TEXT("OWSOpenWorld/Signs"));
		const FString BackingName = FString::Printf(TEXT("OWS_SignBacking_%s"), *Identifier);
		AStaticMeshActor* Backing = SpawnBox(
			World,
			CubeMesh,
			BackingMaterial,
			BackingName,
			LocationMeters,
			FVector(0.25f, 9.0f, 3.0f),
			FRotator(0.0f, YawDegrees, 0.0f),
			DistrictSignTag,
			Folder);
		if (!Backing)
		{
			return false;
		}

		const FString TextActorName = FString::Printf(TEXT("OWS_SignText_%s"), *Identifier);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = FName(*TextActorName);
		SpawnParameters.OverrideActorGuid = StableActorGuid(TextActorName);
		SpawnParameters.NameMode =
			FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
		SpawnParameters.ObjectFlags |= RF_Transactional;
		ATextRenderActor* TextActor = World->SpawnActor<ATextRenderActor>(
			LocationMeters * 100.0f + FRotator(0.0f, YawDegrees, 0.0f).RotateVector(FVector(15.0f, 0.0f, 0.0f)),
			FRotator(0.0f, YawDegrees, 0.0f),
			SpawnParameters);
		if (!TextActor)
		{
			return false;
		}

		TextActor->SetActorLabel(FString::Printf(TEXT("OWS Sign - %s"), *Text));
		TextActor->SetFolderPath(Folder);
		AddGeneratedTags(TextActor, DistrictSignTag);
		UTextRenderComponent* TextComponent = TextActor->GetTextRender();
		TextComponent->SetText(FText::FromString(Text));
		TextComponent->SetHorizontalAlignment(EHTA_Center);
		TextComponent->SetVerticalAlignment(EVRTA_TextCenter);
		TextComponent->SetWorldSize(110.0f);
		TextComponent->SetTextRenderColor(FColor(255, 236, 64));
		TextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TextActor->MarkPackageDirty();
		return true;
	}

	static bool SpawnCityBlocks(
		UWorld* World,
		UStaticMesh* CubeMesh,
		UMaterialInterface* CityMaterial,
		uint32 Seed)
	{
		const float Streets[] = { 1000.0f, 1450.0f, 1900.0f, 2350.0f, 2800.0f, 3250.0f };
		const FName Folder(TEXT("OWSOpenWorld/Urban/Blocks"));
		for (int32 BlockY = 0; BlockY < 5; ++BlockY)
		{
			for (int32 BlockX = 0; BlockX < 5; ++BlockX)
			{
				const FVector2D BlockCenter(
					0.5f * (Streets[BlockX] + Streets[BlockX + 1]),
					0.5f * (Streets[BlockY] + Streets[BlockY + 1]));
				for (int32 Quadrant = 0; Quadrant < 4; ++Quadrant)
				{
					const int32 OffsetXSign = (Quadrant & 1) == 0 ? -1 : 1;
					const int32 OffsetYSign = (Quadrant & 2) == 0 ? -1 : 1;
					const uint32 H = HashCoordinates(Seed, BlockX * 7 + Quadrant, BlockY * 11 + Quadrant);
					const float Height = 34.0f + static_cast<float>(H % 7800u) / 100.0f;
					const float Width = 128.0f + static_cast<float>((H >> 8) % 2600u) / 100.0f;
					const float Depth = 128.0f + static_cast<float>((H >> 16) % 2600u) / 100.0f;
					const FVector Location(
						BlockCenter.X + OffsetXSign * 99.0f,
						BlockCenter.Y + OffsetYSign * 99.0f,
						8.0f + Height * 0.5f);
					const FString Name = FString::Printf(
						TEXT("OWS_CityBlock_%02d_%02d_%c"),
						BlockX,
						BlockY,
						TEXT('A') + Quadrant);
					if (!SpawnBox(
						World,
						CubeMesh,
						CityMaterial,
						Name,
						Location,
						FVector(Width, Depth, Height),
						FRotator::ZeroRotator,
						CityBlockTag,
						Folder))
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	static bool SpawnBridgeAndUnderpass(
		UWorld* World,
		UStaticMesh* CubeMesh,
		UMaterialInterface* BridgeMaterial,
		UMaterialInterface* ConcreteMaterial)
	{
		const FName Folder(TEXT("OWSOpenWorld/Urban/Bridge"));
		const FVector RampSouthStart(1900.0f, 1350.0f, 8.25f);
		const FVector RampSouthEnd(1900.0f, 1705.0f, 15.25f);
		const FVector DeckNorth(1900.0f, 2095.0f, 15.25f);
		const FVector RampNorthEnd(1900.0f, 2450.0f, 8.25f);
		if (!SpawnSegment(World, CubeMesh, BridgeMaterial, TEXT("OWS_Bridge_SouthRamp"), RampSouthStart, RampSouthEnd, 14.0f, 0.5f, BridgeTag, Folder)
			|| !SpawnSegment(World, CubeMesh, BridgeMaterial, TEXT("OWS_Bridge_Deck"), RampSouthEnd, DeckNorth, 14.0f, 0.5f, BridgeTag, Folder)
			|| !SpawnSegment(World, CubeMesh, BridgeMaterial, TEXT("OWS_Bridge_NorthRamp"), DeckNorth, RampNorthEnd, 14.0f, 0.5f, BridgeTag, Folder))
		{
			return false;
		}

		if (!SpawnBox(World, CubeMesh, ConcreteMaterial, TEXT("OWS_Bridge_SouthAbutment"), FVector(1900.0f, 1690.0f, 11.5f), FVector(14.0f, 2.0f, 7.0f), FRotator::ZeroRotator, BridgeTag, Folder)
			|| !SpawnBox(World, CubeMesh, ConcreteMaterial, TEXT("OWS_Bridge_NorthAbutment"), FVector(1900.0f, 2110.0f, 11.5f), FVector(14.0f, 2.0f, 7.0f), FRotator::ZeroRotator, BridgeTag, Folder))
		{
			return false;
		}

		// Long, low rails make the elevated crossing visibly readable and catch
		// minor steering errors without obstructing the at-grade east-west road.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const float X = 1900.0f + Side * 7.25f;
			if (!SpawnSegment(World, CubeMesh, ConcreteMaterial,
				FString::Printf(TEXT("OWS_Bridge_Rail_%s"), Side < 0 ? TEXT("West") : TEXT("East")),
				FVector(X, 1705.0f, 15.85f),
				FVector(X, 2095.0f, 15.85f),
				0.28f,
				0.85f,
				BridgeTag,
				Folder))
			{
				return false;
			}
		}
		return true;
	}

	static bool SpawnRouteMarkers(
		UWorld* World,
		uint32 Seed,
		UStaticMesh* CubeMesh,
		UMaterialInterface* MarkerMaterial)
	{
		const FName Folder(TEXT("OWSOpenWorld/Routes/Markers"));
		int32 MarkerIndex = 0;
		auto SpawnMarker = [&](const FVector2D& XY, const FString& Prefix) -> bool
		{
			const float Z = SampleGeneratedHeightMeters(Seed, XY) + 1.0f;
			return SpawnBox(
				World,
				CubeMesh,
				MarkerMaterial,
				FString::Printf(TEXT("OWS_%s_Marker_%03d"), *Prefix, MarkerIndex++),
				FVector(XY.X, XY.Y, Z),
				FVector(0.35f, 0.35f, 2.0f),
				FRotator::ZeroRotator,
				RouteMarkerTag,
				Folder) != nullptr;
		};

		for (float X = -3000.0f; X <= 3000.0f; X += 500.0f)
		{
			if (!SpawnMarker(FVector2D(X, -1425.0f), TEXT("OvalNorth"))
				|| !SpawnMarker(FVector2D(X, -2575.0f), TEXT("OvalSouth")))
			{
				return false;
			}
		}

		for (int32 Step = 0; Step < 12; ++Step)
		{
			const float Angle = static_cast<float>(Step) / 12.0f * 2.0f * PI;
			const FVector2D Offset(578.0f * FMath::Cos(Angle), 578.0f * FMath::Sin(Angle));
			if (!SpawnMarker(FVector2D(3000.0f, -2000.0f) + Offset, TEXT("OvalEast"))
				|| !SpawnMarker(FVector2D(-3000.0f, -2000.0f) + Offset, TEXT("OvalWest")))
			{
				return false;
			}
		}

		const float JumpLanes[] = { -2950.0f, -3200.0f, -3450.0f };
		for (int32 Lane = 0; Lane < 3; ++Lane)
		{
			for (float X = -3400.0f; X <= 3300.0f; X += 650.0f)
			{
				if (!SpawnMarker(FVector2D(X, JumpLanes[Lane] - 14.0f), FString::Printf(TEXT("Jump%d"), Lane + 1)))
				{
					return false;
				}
			}
		}

		const TArray<FRoutePoint> Mountain = BuildMountainPass();
		const TArray<FRoutePoint> OffRoad = BuildOffRoadLoop();
		const TArray<FRoutePoint> Handling = BuildHandlingLoop();
		auto SpawnPolylineMarkers = [&SpawnMarker](const TArray<FRoutePoint>& Route, float LateralOffset, const FString& Prefix)
		{
			for (int32 Index = 0; Index < Route.Num(); ++Index)
			{
				const FVector2D Previous = Route[FMath::Max(Index - 1, 0)].XY;
				const FVector2D Next = Route[FMath::Min(Index + 1, Route.Num() - 1)].XY;
				const FVector2D Tangent = (Next - Previous).GetSafeNormal();
				const FVector2D Normal(-Tangent.Y, Tangent.X);
				if (!SpawnMarker(Route[Index].XY + Normal * LateralOffset, Prefix))
				{
					return false;
				}
			}
			return true;
		};
		if (!SpawnPolylineMarkers(Mountain, 11.5f, TEXT("Mountain"))
			|| !SpawnPolylineMarkers(OffRoad, 9.0f, TEXT("OffRoad"))
			|| !SpawnPolylineMarkers(Handling, 11.0f, TEXT("Handling")))
		{
			return false;
		}
		return true;
	}

	static bool SpawnSignsAndRecoveryAnchors(
		UWorld* World,
		uint32 Seed,
		UStaticMesh* CubeMesh,
		UMaterialInterface* MarkerMaterial)
	{
		struct FSignDefinition
		{
			const TCHAR* Identifier;
			const TCHAR* Text;
			FVector2D XY;
			float Yaw;
		};
		const FSignDefinition Signs[] = {
			{ TEXT("Campus"), TEXT("SYSTEMS CAMPUS"), FVector2D(0.0f, 78.0f), 90.0f },
			{ TEXT("Oval"), TEXT("6 KM HIGH-SPEED OVAL"), FVector2D(-1750.0f, -1390.0f), -90.0f },
			{ TEXT("Jumps"), TEXT("JUMP CORRIDORS 8 / 15 / 25 DEG"), FVector2D(-3250.0f, -2820.0f), -90.0f },
			{ TEXT("Urban"), TEXT("URBAN GRID + UNDERPASS"), FVector2D(900.0f, 900.0f), 45.0f },
			{ TEXT("Mountain"), TEXT("MOUNTAIN PASS"), FVector2D(-650.0f, 600.0f), 35.0f },
			{ TEXT("OffRoad"), TEXT("OFF-ROAD ENDURANCE LOOP"), FVector2D(650.0f, 400.0f), 25.0f },
			{ TEXT("Handling"), TEXT("ROLLING HANDLING CIRCUIT"), FVector2D(-650.0f, 350.0f), 150.0f }
		};
		for (const FSignDefinition& Sign : Signs)
		{
			const FVector Location(Sign.XY.X, Sign.XY.Y, SampleGeneratedHeightMeters(Seed, Sign.XY) + 2.2f);
			if (!SpawnDistrictSign(World, CubeMesh, MarkerMaterial, Sign.Identifier, Sign.Text, Location, Sign.Yaw))
			{
				return false;
			}
		}

		const FVector2D AnchorLocations[] = {
			FVector2D(0.0f, 0.0f),
			FVector2D(-1800.0f, -1450.0f),
			FVector2D(1000.0f, 1000.0f),
			FVector2D(-1100.0f, 900.0f),
			FVector2D(1000.0f, -600.0f),
			FVector2D(-1300.0f, 450.0f),
			FVector2D(-3300.0f, -3200.0f)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(AnchorLocations); ++Index)
		{
			FActorSpawnParameters SpawnParameters;
			const FString ActorName = FString::Printf(TEXT("OWS_RecoveryAnchor_%02d"), Index);
			SpawnParameters.Name = FName(*ActorName);
			SpawnParameters.OverrideActorGuid = StableActorGuid(ActorName);
			SpawnParameters.NameMode =
				FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
			SpawnParameters.ObjectFlags |= RF_Transactional;
			const FVector2D XY = AnchorLocations[Index];
			ATargetPoint* Anchor = World->SpawnActor<ATargetPoint>(
				FVector(XY.X, XY.Y, SampleGeneratedHeightMeters(Seed, XY) + 1.0f) * 100.0f,
				FRotator::ZeroRotator,
				SpawnParameters);
			if (!Anchor)
			{
				return false;
			}
			Anchor->SetActorLabel(FString::Printf(TEXT("OWS Recovery Anchor %02d"), Index));
			Anchor->SetFolderPath(TEXT("OWSOpenWorld/Recovery"));
			AddGeneratedTags(Anchor, RecoveryAnchorTag);
			Anchor->MarkPackageDirty();
		}
		return true;
	}

	static bool SpawnMultiplayerStarts(UWorld* World)
	{
		// These supplement the converted map's original primary PlayerStart.
		// All four sit on the retained central foundation, whose top is Z=0.
		const FVector LocationsCm[] = {
			FVector(-5500.0f, -3500.0f, 110.0f),
			FVector(-5500.0f, 3500.0f, 110.0f),
			FVector(5500.0f, -3500.0f, 110.0f),
			FVector(5500.0f, 3500.0f, 110.0f)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(LocationsCm); ++Index)
		{
			FActorSpawnParameters SpawnParameters;
			const FString ActorName = FString::Printf(TEXT("OWS_MultiplayerStart_%02d"), Index + 1);
			SpawnParameters.Name = FName(*ActorName);
			SpawnParameters.OverrideActorGuid = StableActorGuid(ActorName);
			SpawnParameters.NameMode =
				FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
			SpawnParameters.ObjectFlags |= RF_Transactional;
			APlayerStart* Start = World->SpawnActor<APlayerStart>(LocationsCm[Index], FRotator::ZeroRotator, SpawnParameters);
			if (!Start)
			{
				return false;
			}
			Start->SetActorLabel(FString::Printf(TEXT("OWS Multiplayer Start %02d"), Index + 1));
			Start->SetFolderPath(TEXT("OWSOpenWorld/PlayerStarts"));
			AddGeneratedTags(Start, MultiplayerStartTag);
			Start->MarkPackageDirty();
		}
		return true;
	}

	static bool ConfigureWorldPartition(UWorld* World, FString& OutError)
	{
		UWorldPartition* WorldPartition = World->GetWorldPartition();
		if (!WorldPartition)
		{
			OutError = TEXT("The target map is not World Partition enabled.");
			return false;
		}

		WorldPartition->Modify();
		WorldPartition->SetEnableStreaming(true);
		UWorldPartitionRuntimeHashSet* HashSet = Cast<UWorldPartitionRuntimeHashSet>(WorldPartition->RuntimeHash);
		if (!HashSet)
		{
			OutError = TEXT("The target map does not use UE 5.8's Runtime Hash Set.");
			return false;
		}

		const URuntimePartition* ResolvedPartition = HashSet->ResolveRuntimePartition(HashSet->GetDefaultGrid(), true);
		URuntimePartitionLHGrid* MainGrid = const_cast<URuntimePartitionLHGrid*>(Cast<URuntimePartitionLHGrid>(ResolvedPartition));
		if (!MainGrid)
		{
			OutError = TEXT("The Runtime Hash Set's main partition is not an LHGrid.");
			return false;
		}

		FIntProperty* LoadingRangeProperty = FindFProperty<FIntProperty>(URuntimePartition::StaticClass(), TEXT("LoadingRange"));
		FUInt32Property* CellSizeProperty = FindFProperty<FUInt32Property>(URuntimePartitionLHGrid::StaticClass(), TEXT("CellSize"));
		if (!LoadingRangeProperty || !CellSizeProperty)
		{
			OutError = TEXT("UE 5.8 Runtime Hash Set properties LoadingRange/CellSize were not found with their expected types.");
			return false;
		}

		MainGrid->Modify();
		MainGrid->PreEditChange(LoadingRangeProperty);
		LoadingRangeProperty->SetPropertyValue_InContainer(MainGrid, RuntimeLoadingRangeCm);
		FPropertyChangedEvent LoadingRangeEvent(LoadingRangeProperty, EPropertyChangeType::ValueSet);
		MainGrid->PostEditChangeProperty(LoadingRangeEvent);

		MainGrid->PreEditChange(CellSizeProperty);
		CellSizeProperty->SetPropertyValue_InContainer(MainGrid, RuntimeCellSizeCm);
		FPropertyChangedEvent CellSizeEvent(CellSizeProperty, EPropertyChangeType::ValueSet);
		MainGrid->PostEditChangeProperty(CellSizeEvent);
		HashSet->MarkPackageDirty();
		return true;
	}

	static bool MakeCampusManagerAlwaysLoaded(UWorld* World, FString& OutError)
	{
		UWorldPartition* WorldPartition = World->GetWorldPartition();
		FWorldPartitionHelpers::FForEachActorWithLoadingParams Params;
		Params.bKeepReferences = true;
		Params.ActorClasses = { AOWSTestLabEnvironment::StaticClass() };
		FWorldPartitionHelpers::FForEachActorWithLoadingResult Result;
		int32 ManagerCount = 0;
		FWorldPartitionHelpers::ForEachActorWithLoading(
			WorldPartition,
			[&ManagerCount](const FWorldPartitionActorDescInstance* ActorDescInstance)
			{
				if (AOWSTestLabEnvironment* Manager = Cast<AOWSTestLabEnvironment>(ActorDescInstance->GetActor()))
				{
					Manager->Modify();
					Manager->SetIsSpatiallyLoaded(false);
					Manager->Tags.AddUnique(CampusManagerTag);
					Manager->MarkPackageDirty();
					++ManagerCount;
				}
				return true;
			},
			Params,
			Result);

		if (ManagerCount != 1)
		{
			OutError = FString::Printf(TEXT("Expected exactly one AOWSTestLabEnvironment campus manager; found %d."), ManagerCount);
			return false;
		}
		return true;
	}

	static bool EnsureLandscapeComponents(
		ULandscapeInfo* LandscapeInfo,
		ULandscapeSubsystem* LandscapeSubsystem,
		int32 ComponentMinX,
		int32 ComponentMinY,
		int32 ComponentMaxX,
		int32 ComponentMaxY,
		FString& OutError)
	{
		check(LandscapeInfo && LandscapeSubsystem);
		LandscapeInfo->Modify();
		TArray<ULandscapeComponent*> NewComponents;
		for (int32 ComponentY = ComponentMinY; ComponentY <= ComponentMaxY; ++ComponentY)
		{
			for (int32 ComponentX = ComponentMinX; ComponentX <= ComponentMaxX; ++ComponentX)
			{
				const FIntPoint ComponentCoordinate(ComponentX, ComponentY);
				if (LandscapeInfo->XYtoComponentMap.Contains(ComponentCoordinate))
				{
					continue;
				}

				const FIntPoint ComponentBase = ComponentCoordinate * LandscapeInfo->ComponentSizeQuads;
				ALandscapeProxy* Proxy = LandscapeSubsystem->FindOrAddLandscapeProxy(LandscapeInfo, ComponentBase);
				if (!Proxy)
				{
					OutError = FString::Printf(TEXT("Could not create Landscape proxy for component (%d,%d)."), ComponentX, ComponentY);
					return false;
				}

				ULandscapeComponent* Component = NewObject<ULandscapeComponent>(Proxy, NAME_None, RF_Transactional);
				if (!Component)
				{
					OutError = FString::Printf(TEXT("Could not create Landscape component (%d,%d)."), ComponentX, ComponentY);
					return false;
				}

				Component->Init(
					ComponentBase.X,
					ComponentBase.Y,
					Proxy->ComponentSizeQuads,
					Proxy->NumSubsections,
					Proxy->SubsectionSizeQuads);
				const int32 ComponentVerts = (Component->SubsectionSizeQuads + 1) * Component->NumSubsections;
				TArray<FColor> DefaultHeightData;
				DefaultHeightData.Init(LandscapeDataAccess::GetDefaultPackedHeightColor(), FMath::Square(ComponentVerts));
				Component->InitHeightmapData(DefaultHeightData, true);
				Component->UpdateMaterialInstances();
				LandscapeInfo->XYtoComponentMap.Add(ComponentCoordinate, Component);
				LandscapeInfo->XYtoAddCollisionMap.Remove(ComponentCoordinate);
				NewComponents.Add(Component);
			}
		}

		for (ULandscapeComponent* Component : NewComponents)
		{
			Component->RegisterComponent();
		}

		ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
		for (ULandscapeComponent* Component : NewComponents)
		{
			if (Landscape)
			{
				const TArray<ULandscapeComponent*> ComponentsUsingHeightmap = { Component };
				for (const ULandscapeEditLayerBase* EditLayer : Landscape->GetEditLayersConst())
				{
					TMap<UTexture2D*, UTexture2D*> CreatedHeightmapTextures;
					Component->AddDefaultLayerData(EditLayer->GetGuid(), ComponentsUsingHeightmap, CreatedHeightmapTextures);
				}
			}
			Component->UpdateCachedBounds();
			Component->UpdateBounds();
			Component->MarkRenderStateDirty();
			Component->MarkPackageDirty();
		}
		return true;
	}

	static bool ApplyLandscapeRegion(
		ULandscapeInfo* LandscapeInfo,
		const FGeneratedAssets& Assets,
		uint32 Seed,
		int32 ComponentMinX,
		int32 ComponentMinY,
		int32 ComponentMaxX,
		int32 ComponentMaxY,
		FString& OutError)
	{
		ALandscape* Landscape = LandscapeInfo ? LandscapeInfo->LandscapeActor.Get() : nullptr;
		const ULandscapeEditLayerBase* BaseEditLayer = Landscape ? Landscape->GetEditLayerConst(0) : nullptr;
		if (!BaseEditLayer)
		{
			OutError = TEXT("The generated Landscape has no persistent base edit layer for regional writes.");
			return false;
		}
		const FGuid BaseEditLayerGuid = BaseEditLayer->GetGuid();
		const int32 WriteMinX = ComponentMinX * QuadsPerComponent;
		const int32 WriteMinY = ComponentMinY * QuadsPerComponent;
		const int32 WriteMaxX = (ComponentMaxX + 1) * QuadsPerComponent;
		const int32 WriteMaxY = (ComponentMaxY + 1) * QuadsPerComponent;
		const int32 HaloMinX = FMath::Max(WriteMinX - 1, 0);
		const int32 HaloMinY = FMath::Max(WriteMinY - 1, 0);
		const int32 HaloMaxX = FMath::Min(WriteMaxX + 1, QuadsPerSide);
		const int32 HaloMaxY = FMath::Min(WriteMaxY + 1, QuadsPerSide);

		UE_LOG(LogOWSOpenWorldGenerator, Display,
			TEXT("Generating Landscape region components X=%d..%d Y=%d..%d (vertices %d..%d, %d..%d)..."),
			ComponentMinX, ComponentMaxX, ComponentMinY, ComponentMaxY,
			WriteMinX, WriteMaxX, WriteMinY, WriteMaxY);
		FHeightfield RegionHeightfield(Seed, HaloMinX, HaloMinY, HaloMaxX, HaloMaxY);
		BuildTerrainLayout(RegionHeightfield);
		const int32 WriteStride = WriteMaxX - WriteMinX + 1;

		{
			TArray<uint16> HeightData;
			TArray<uint16> NormalData;
			RegionHeightfield.BuildHeightData(WriteMinX, WriteMinY, WriteMaxX, WriteMaxY, HeightData);
			RegionHeightfield.BuildNormalData(WriteMinX, WriteMinY, WriteMaxX, WriteMaxY, NormalData);
			FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo, BaseEditLayerGuid, true);
			LandscapeEdit.SetHeightData(
				WriteMinX, WriteMinY, WriteMaxX, WriteMaxY,
				HeightData.GetData(), WriteStride, false, NormalData.GetData());
			LandscapeEdit.Flush();
		}

		const struct FSurfaceLayer
		{
			ESurfaceClass Surface;
			ULandscapeLayerInfoObject* LayerInfo;
		} SurfaceLayers[] = {
			{ ESurfaceClass::Grass, Assets.GrassLayer },
			{ ESurfaceClass::Asphalt, Assets.AsphaltLayer },
			{ ESurfaceClass::Gravel, Assets.GravelLayer },
			{ ESurfaceClass::Dirt, Assets.DirtLayer }
		};
		for (const FSurfaceLayer& SurfaceLayer : SurfaceLayers)
		{
			if (!SurfaceLayer.LayerInfo)
			{
				OutError = TEXT("A generated Landscape layer-info asset was unavailable while applying a region.");
				return false;
			}
			TArray<uint8> WeightData;
			RegionHeightfield.BuildSurfaceData(
				WriteMinX, WriteMinY, WriteMaxX, WriteMaxY, SurfaceLayer.Surface, WeightData);
			FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo, BaseEditLayerGuid, true);
			LandscapeEdit.SetAlphaData(
				SurfaceLayer.LayerInfo,
				WriteMinX, WriteMinY, WriteMaxX, WriteMaxY,
				WeightData.GetData(), WriteStride,
				ELandscapeLayerPaintingRestriction::None);
			LandscapeEdit.Flush();
		}

		LandscapeInfo->ForceLayersFullUpdate();
		if (ALandscapeProxy* LandscapeProxy = LandscapeInfo->GetLandscapeProxy())
		{
			UMaterialInterface::SubmitRemainingJobsForWorld(LandscapeProxy->GetWorld());
		}
		FAssetCompilingManager::Get().FinishAllCompilation();
		FAssetCompilingManager::Get().ProcessAsyncTasks();
		FlushRenderingCommands();
		return true;
	}

	static bool SaveDirtyLandscapeProxyPackages(
		UWorld* World,
		ULandscapeInfo* LandscapeInfo,
		FString& OutError)
	{
		TArray<UPackage*> DirtyProxyPackages;
		LandscapeInfo->ForEachLandscapeProxy([&DirtyProxyPackages](ALandscapeProxy* Proxy)
		{
			if (Proxy && Proxy->IsA<ALandscapeStreamingProxy>() && Proxy->GetPackage()->IsDirty())
			{
				DirtyProxyPackages.AddUnique(Proxy->GetPackage());
			}
			return true;
		});

		if (DirtyProxyPackages.IsEmpty())
		{
			return true;
		}

		DirtyProxyPackages.Sort([](const UPackage& A, const UPackage& B)
		{
			return A.GetName() < B.GetName();
		});
		UWorldPartition::FDisableNonDirtyActorTrackingScope TrackingScope(World->GetWorldPartition(), true);
		if (!UEditorLoadingAndSavingUtils::SavePackages(DirtyProxyPackages, true))
		{
			OutError = TEXT("Failed to save one or more dirty Landscape streaming proxy packages.");
			return false;
		}
		return true;
	}

	static ALandscape* ImportLandscape(
		UWorld* World,
		const FGeneratedAssets& Assets,
		uint32 Seed,
		FString& OutError)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = TEXT("OWSOpenWorldLandscape");
		SpawnParameters.OverrideActorGuid = StableActorGuid(TEXT("OWSOpenWorldLandscape"));
		SpawnParameters.NameMode =
			FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
		SpawnParameters.ObjectFlags |= RF_Transactional;
		ALandscape* Landscape = World->SpawnActor<ALandscape>(
			FVector(-HalfExtentMeters * 100.0f, -HalfExtentMeters * 100.0f, 0.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!Landscape)
		{
			OutError = TEXT("Could not spawn the Landscape actor.");
			return nullptr;
		}

		Landscape->SetActorLabel(TEXT("OWS Open World Landscape 8129"));
		Landscape->SetActorScale3D(FVector(LandscapeScaleCm, LandscapeScaleCm, LandscapeScaleCm));
		Landscape->SetIsSpatiallyLoaded(false);
		Landscape->Tags.AddUnique(GeneratedTag);
		Landscape->Tags.AddUnique(LandscapeTag);
		Landscape->Tags.AddUnique(FName(*FString::Printf(TEXT("OWS.OpenWorld.Seed.%u"), Seed)));
		// Generation runs in a fully initialized editor world, after the generated
		// material's shader jobs have been completed and rendering commands flushed.
		// Bind the project material before creating regional proxy components so each
		// component serializes a valid material-instance set on its first save.
		Landscape->LandscapeMaterial = Assets.TerrainMaterial;
		Landscape->CollisionMipLevel = 0;
		Landscape->SimpleCollisionMipLevel = 1;
		Landscape->StaticLightingLOD = 3;

		// UE's World Partition landscape UI creates oversized landscapes in bounded
		// regions. Seed a single 2x2-component proxy, then add and populate four
		// 16x16-component regions through the same component/edit-data APIs. This
		// avoids ever allocating an 8129x8129 height array plus four global weight arrays.
		constexpr int32 InitialComponentCount = LandscapeGridSizeInComponents;
		constexpr int32 InitialQuads = InitialComponentCount * QuadsPerComponent;
		TArray<uint16> InitialHeightData;
		InitialHeightData.Init(LandscapeDataAccess::GetTexHeight(0.0f), FMath::Square(InitialQuads + 1));
		TMap<FGuid, TArray<uint16>> HeightDataPerLayer;
		HeightDataPerLayer.Add(FGuid(), MoveTemp(InitialHeightData));
		TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialDataPerLayer;
		MaterialDataPerLayer.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

		Landscape->Import(
			FGuid::NewDeterministicGuid(TEXT("OWS.OpenWorld.LandscapeGuid"), Seed),
			0,
			0,
			InitialQuads,
			InitialQuads,
			SectionsPerComponent,
			QuadsPerSection,
			HeightDataPerLayer,
			TEXT(""),
			MaterialDataPerLayer,
			ELandscapeImportAlphamapType::Additive,
			TArrayView<const FLandscapeLayer>());

		// Landscape import creates material instances and render resources. Drain
		// their deferred shader and render work before repartitioning components;
		// otherwise the next region can mutate proxies still referenced by the
		// render thread.
		UMaterialInterface::SubmitRemainingJobsForWorld(World);
		FAssetCompilingManager::Get().FinishAllCompilation();
		FAssetCompilingManager::Get().ProcessAsyncTasks();
		FlushRenderingCommands();

		ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
		if (!LandscapeInfo)
		{
			OutError = TEXT("Landscape import did not create a LandscapeInfo.");
			return nullptr;
		}

		LandscapeInfo->UpdateLayerInfoMap(Landscape);
		ULandscapeLayerInfoObject* LayerInfos[] = {
			Assets.GrassLayer,
			Assets.AsphaltLayer,
			Assets.GravelLayer,
			Assets.DirtLayer
		};
		for (ULandscapeLayerInfoObject* LayerInfo : LayerInfos)
		{
			Landscape->AddTargetLayer(LayerInfo->GetLayerName(), FLandscapeTargetLayerSettings(LayerInfo));
		}
		LandscapeInfo->UpdateLayerInfoMap(Landscape);

		ULandscapeSubsystem* LandscapeSubsystem = World->GetSubsystem<ULandscapeSubsystem>();
		if (!LandscapeSubsystem)
		{
			OutError = TEXT("Landscape subsystem was unavailable after import.");
			return nullptr;
		}
		LandscapeSubsystem->ChangeGridSize(LandscapeInfo, LandscapeGridSizeInComponents);

		for (int32 RegionComponentY = 0; RegionComponentY < ComponentCountPerSide; RegionComponentY += LandscapeRegionSizeInComponents)
		{
			for (int32 RegionComponentX = 0; RegionComponentX < ComponentCountPerSide; RegionComponentX += LandscapeRegionSizeInComponents)
			{
				const int32 RegionMaxComponentX = FMath::Min(
					RegionComponentX + LandscapeRegionSizeInComponents - 1,
					ComponentCountPerSide - 1);
				const int32 RegionMaxComponentY = FMath::Min(
					RegionComponentY + LandscapeRegionSizeInComponents - 1,
					ComponentCountPerSide - 1);
				if (!EnsureLandscapeComponents(
					LandscapeInfo,
					LandscapeSubsystem,
					RegionComponentX,
					RegionComponentY,
					RegionMaxComponentX,
					RegionMaxComponentY,
					OutError)
					|| !ApplyLandscapeRegion(
						LandscapeInfo,
						Assets,
						Seed,
						RegionComponentX,
						RegionComponentY,
						RegionMaxComponentX,
						RegionMaxComponentY,
						OutError))
				{
					return nullptr;
				}

				{
					const int32 ExpectedMinX = RegionComponentX * QuadsPerComponent;
					const int32 ExpectedMinY = RegionComponentY * QuadsPerComponent;
					const int32 ExpectedMaxX = (RegionMaxComponentX + 1) * QuadsPerComponent;
					const int32 ExpectedMaxY = (RegionMaxComponentY + 1) * QuadsPerComponent;
					const int32 ExpectedRegionComponentCount =
						(RegionMaxComponentX - RegionComponentX + 1)
						* (RegionMaxComponentY - RegionComponentY + 1);
					const int32 ExpectedRegionProxyCount =
						((RegionMaxComponentX - RegionComponentX + 1) / LandscapeGridSizeInComponents)
						* ((RegionMaxComponentY - RegionComponentY + 1) / LandscapeGridSizeInComponents);

					int32 MinX = 0;
					int32 MinY = 0;
					int32 MaxX = 0;
					int32 MaxY = 0;
					const bool bHasLandscapeExtent = LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY);
					if (!bHasLandscapeExtent
						|| MinX != ExpectedMinX
						|| MinY != ExpectedMinY
						|| MaxX != ExpectedMaxX
						|| MaxY != ExpectedMaxY)
					{
						OutError = FString::Printf(
							TEXT("Landscape regional extent is invalid before save (valid=%s, actual=[%d,%d]-[%d,%d], expected=[%d,%d]-[%d,%d])."),
							bHasLandscapeExtent ? TEXT("true") : TEXT("false"),
							MinX,
							MinY,
							MaxX,
							MaxY,
							ExpectedMinX,
							ExpectedMinY,
							ExpectedMaxX,
							ExpectedMaxY);
						return nullptr;
					}

					int32 ComponentCount = 0;
					LandscapeInfo->ForAllLandscapeComponents([&ComponentCount](ULandscapeComponent*)
					{
						++ComponentCount;
					});
					const TArray<TWeakObjectPtr<ALandscapeStreamingProxy>>& StreamingProxies =
						LandscapeInfo->GetSortedStreamingProxies();
					if (ComponentCount != ExpectedRegionComponentCount
						|| StreamingProxies.Num() != ExpectedRegionProxyCount
						|| Landscape->LandscapeComponents.Num() != 0)
					{
						OutError = FString::Printf(
							TEXT("Landscape regional structure mismatch before save (components=%d/%d, proxies=%d/%d, root-components=%d)."),
							ComponentCount,
							ExpectedRegionComponentCount,
							StreamingProxies.Num(),
							ExpectedRegionProxyCount,
							Landscape->LandscapeComponents.Num());
						return nullptr;
					}

					for (const TWeakObjectPtr<ALandscapeStreamingProxy>& WeakProxy : StreamingProxies)
					{
						ALandscapeStreamingProxy* Proxy = WeakProxy.Get();
						if (!Proxy
							|| Proxy->LandscapeComponents.Num()
								!= LandscapeGridSizeInComponents * LandscapeGridSizeInComponents)
						{
							OutError = TEXT("A regional Landscape streaming proxy is missing or does not own exactly four components before save.");
							return nullptr;
						}
						Proxy->Modify();
						Proxy->CollisionMipLevel = 0;
						Proxy->SimpleCollisionMipLevel = 1;
						Proxy->MarkPackageDirty();
					}
					LandscapeInfo->RecreateCollisionComponents();
					Landscape->MarkPackageDirty();
				}

				if (!SaveDirtyLandscapeProxyPackages(World, LandscapeInfo, OutError))
				{
					return nullptr;
				}
			}
		}

		Landscape->MarkPackageDirty();
		return Landscape;
	}

	static const TCHAR* const GeneratedPackageNames[] = {
		TerrainMaterialPackage,
		GrassLayerPackage,
		AsphaltLayerPackage,
		GravelLayerPackage,
		DirtLayerPackage,
		CityMaterialPackage,
		ConcreteMaterialPackage,
		MarkerMaterialPackage,
		BridgeMaterialPackage
	};

	static bool PreflightGeneration(UWorld* World, FString& OutError)
	{
		if (!World || !World->PersistentLevel || !World->GetWorldPartition())
		{
			OutError = TEXT("The requested map is not a loaded World Partition world.");
			return false;
		}
		if (!World->GetWorldPartition()->IsInitialized() || !World->PersistentLevel->IsUsingExternalActors())
		{
			OutError = TEXT("The target World Partition/One File Per Actor world is not fully initialized.");
			return false;
		}
		if (IsRunningCommandlet() && !IsAllowCommandletRendering())
		{
			OutError = TEXT("Landscape generation requires -AllowCommandletRendering.");
			return false;
		}

		for (const TCHAR* PackageName : GeneratedPackageNames)
		{
			if (FPackageName::DoesPackageExist(PackageName))
			{
				OutError = FString::Printf(TEXT("Refusing overwrite: generated package already exists: %s"), PackageName);
				return false;
			}
		}

		int32 LandscapeActorDescriptorCount = 0;
		FWorldPartitionHelpers::ForEachActorDescInstance<ALandscapeProxy>(
			World->GetWorldPartition(),
			[&LandscapeActorDescriptorCount](const FWorldPartitionActorDescInstance*)
			{
				++LandscapeActorDescriptorCount;
				return true;
			});
		if (LandscapeActorDescriptorCount > 0)
		{
			OutError = FString::Printf(
				TEXT("Refusing overwrite: target map already contains %d Landscape actor descriptor(s)."),
				LandscapeActorDescriptorCount);
			return false;
		}

		return true;
	}

	static bool ValidateRuntimeGrid(UWorld* World, FString& OutError)
	{
		UWorldPartition* WorldPartition = World ? World->GetWorldPartition() : nullptr;
		if (!WorldPartition || !WorldPartition->IsStreamingEnabled())
		{
			OutError = TEXT("World Partition streaming is not enabled.");
			return false;
		}

		const UWorldPartitionRuntimeHashSet* HashSet = Cast<UWorldPartitionRuntimeHashSet>(WorldPartition->RuntimeHash);
		if (!HashSet)
		{
			OutError = TEXT("Runtime Hash Set is missing.");
			return false;
		}

		const URuntimePartitionLHGrid* MainGrid = Cast<URuntimePartitionLHGrid>(
			HashSet->ResolveRuntimePartition(HashSet->GetDefaultGrid(), true));
		if (!MainGrid)
		{
			OutError = TEXT("Runtime Hash Set main LHGrid is missing.");
			return false;
		}

		if (MainGrid->GetCellSize() != RuntimeCellSizeCm || MainGrid->LoadingRange != RuntimeLoadingRangeCm)
		{
			OutError = FString::Printf(
				TEXT("Runtime grid mismatch: cell=%u cm, loading=%d cm; expected %d/%d."),
				MainGrid->GetCellSize(),
				MainGrid->LoadingRange,
				RuntimeCellSizeCm,
				RuntimeLoadingRangeCm);
			return false;
		}
		return true;
	}

	static bool HasRequiredBlockingCollision(const UPrimitiveComponent* Component)
	{
		if (!Component
			|| Component->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics
			|| Component->GetCollisionObjectType() != ECC_WorldStatic)
		{
			return false;
		}

		const ECollisionChannel RequiredBlockedChannels[] = {
			ECC_Visibility,
			ECC_Camera,
			ECC_Pawn,
			ECC_Vehicle,
			ECC_WorldDynamic
		};
		for (const ECollisionChannel Channel : RequiredBlockedChannels)
		{
			if (Component->GetCollisionResponseToChannel(Channel) != ECR_Block)
			{
				return false;
			}
		}

		return true;
	}

	static bool HasHardenedStaticMeshCollision(const UStaticMeshComponent* Mesh)
	{
		if (!HasRequiredBlockingCollision(Mesh) || !Mesh->GetStaticMesh())
		{
			return false;
		}
		const UBodySetup* BodySetup = Mesh->GetStaticMesh()->GetBodySetup();
		return BodySetup
			&& (BodySetup->AggGeom.GetElementCount() > 0
				|| BodySetup->CollisionTraceFlag == CTF_UseComplexAsSimple);
	}

	static bool ValidateStaticMeshCollisionQueries(UWorld* World, const AStaticMeshActor* MeshActor, FString& OutError)
	{
		if (!World || !MeshActor)
		{
			OutError = TEXT("No generated static-mesh actor was available for real collision-query validation.");
			return false;
		}

		const FBox Bounds = MeshActor->GetComponentsBoundingBox(true);
		if (!Bounds.IsValid)
		{
			OutError = TEXT("Generated static-mesh validation actor has invalid bounds.");
			return false;
		}

		const FVector Center = Bounds.GetCenter();
		const FVector Start(Center.X, Center.Y, Bounds.Max.Z + 1000.0);
		const FVector End(Center.X, Center.Y, Bounds.Min.Z - 1000.0);
		const ECollisionChannel QueryChannels[] = {
			ECC_Visibility,
			ECC_Camera,
			ECC_Pawn,
			ECC_Vehicle,
			ECC_WorldDynamic
		};
		for (const ECollisionChannel Channel : QueryChannels)
		{
			FHitResult Hit;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OWSGeneratedGeometryValidation), false);
			if (!World->LineTraceSingleByChannel(Hit, Start, End, Channel, QueryParams)
				|| Hit.GetActor() != MeshActor)
			{
				OutError = FString::Printf(
					TEXT("Generated geometry actor '%s' did not block real line trace channel %d."),
					*MeshActor->GetName(),
					static_cast<int32>(Channel));
				return false;
			}
		}
		return true;
	}

	static bool ValidateTerrainSamples(
		UWorld* World,
		ALandscape* Landscape,
		uint32 ExpectedSeed,
		FString& OutError)
	{
		ULandscapeInfo* LandscapeInfo = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
		if (!World || !LandscapeInfo)
		{
			OutError = TEXT("Landscape sample validation could not access the world/LandscapeInfo.");
			return false;
		}

		const TMap<FName, FLandscapeTargetLayerSettings>& TargetLayers = Landscape->GetTargetLayers();
		auto FindLayer = [&TargetLayers](FName LayerName) -> ULandscapeLayerInfoObject*
		{
			const FLandscapeTargetLayerSettings* Settings = TargetLayers.Find(LayerName);
			return Settings ? Settings->LayerInfoObj : nullptr;
		};
		ULandscapeLayerInfoObject* LayerInfos[] = {
			FindLayer(TEXT("Grass")),
			FindLayer(TEXT("Dirt")),
			FindLayer(TEXT("Gravel")),
			FindLayer(TEXT("Asphalt"))
		};
		for (ULandscapeLayerInfoObject* LayerInfo : LayerInfos)
		{
			if (!LayerInfo)
			{
				OutError = TEXT("Landscape sample validation could not resolve all four target layers.");
				return false;
			}
		}

		struct FTerrainSample
		{
			const TCHAR* Label;
			FVector2D XY;
			float SemanticHeightMeters;
			ESurfaceClass SemanticSurface;
			bool bRequireLandscapeTrace;
		};
		constexpr float NoSemanticHeight = MAX_flt;
		const FTerrainSample Samples[] = {
			// Three adjacent samples straddle the X=0 regional-write boundary and
			// prove that the oval's height and paint remain continuous across it.
			{ TEXT("Campus foundation underlay"), FVector2D(15.0f, 0.0f), -0.5f, ESurfaceClass::Grass, false },
			{ TEXT("Oval west straight"), FVector2D(-2500.0f, -1450.0f), -3.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Oval continuity west of region seam"), FVector2D(-2.0f, -1450.0f), -3.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Oval continuity on region seam"), FVector2D(0.0f, -1450.0f), -3.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Oval continuity east of region seam"), FVector2D(2.0f, -1450.0f), -3.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Oval east straight"), FVector2D(2500.0f, -1450.0f), -3.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Urban arterial"), FVector2D(1450.0f, 1900.0f), 8.0f, ESurfaceClass::Asphalt, false },
			{ TEXT("Mountain pass low"), FVector2D(-1900.0f, 1300.0f), 52.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Mountain pass high"), FVector2D(-2450.0f, 2850.0f), 150.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Jump 8-degree lip"), FVector2D(-970.0f, -2950.0f), -5.0f + FMath::Tan(FMath::DegreesToRadians(8.0f)) * 80.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Jump 15-degree landing"), FVector2D(-400.0f, -3200.0f), 11.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Jump 25-degree landing"), FVector2D(0.0f, -3450.0f), 24.0f, ESurfaceClass::Asphalt, true },
			{ TEXT("Perimeter catch berm"), FVector2D(-4060.0f, 0.0f), NoSemanticHeight, ESurfaceClass::Gravel, true }
		};

		for (const FTerrainSample& Sample : Samples)
		{
			float ExpectedHeightMeters = 0.0f;
			ESurfaceClass ExpectedSurface = ESurfaceClass::Grass;
			SampleGeneratedTerrain(ExpectedSeed, Sample.XY, ExpectedHeightMeters, ExpectedSurface);
			if (Sample.SemanticHeightMeters != NoSemanticHeight
				&& !FMath::IsNearlyEqual(ExpectedHeightMeters, Sample.SemanticHeightMeters, 0.02f))
			{
				OutError = FString::Printf(
					TEXT("Generator semantic check failed at %s: computed %.3f m, required %.3f m."),
					Sample.Label, ExpectedHeightMeters, Sample.SemanticHeightMeters);
				return false;
			}
			if (ExpectedSurface != Sample.SemanticSurface)
			{
				OutError = FString::Printf(TEXT("Generator surface semantic check failed at %s."), Sample.Label);
				return false;
			}

			const int32 LandscapeX = FMath::Clamp(FMath::RoundToInt(Sample.XY.X + HalfExtentMeters), 0, QuadsPerSide);
			const int32 LandscapeY = FMath::Clamp(FMath::RoundToInt(Sample.XY.Y + HalfExtentMeters), 0, QuadsPerSide);
			uint16 StoredHeight = 0;
			{
				FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo, false);
				LandscapeEdit.GetHeightDataFast(
					LandscapeX, LandscapeY, LandscapeX, LandscapeY, &StoredHeight, 1);
			}
			const uint16 ExpectedStoredHeight = LandscapeDataAccess::GetTexHeight(ExpectedHeightMeters);
			if (FMath::Abs(static_cast<int32>(StoredHeight) - static_cast<int32>(ExpectedStoredHeight)) > 1)
			{
				OutError = FString::Printf(
					TEXT("Saved height mismatch at %s: stored=%u expected=%u."),
					Sample.Label, StoredHeight, ExpectedStoredHeight);
				return false;
			}

			uint8 StoredWeights[UE_ARRAY_COUNT(LayerInfos)] = { 0, 0, 0, 0 };
			for (int32 LayerIndex = 0; LayerIndex < UE_ARRAY_COUNT(LayerInfos); ++LayerIndex)
			{
				FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo, false);
				LandscapeEdit.GetWeightDataFast(
					LayerInfos[LayerIndex],
					LandscapeX, LandscapeY, LandscapeX, LandscapeY,
					&StoredWeights[LayerIndex], 1);
			}
			const int32 ExpectedLayerIndex = static_cast<int32>(ExpectedSurface);
			for (int32 LayerIndex = 0; LayerIndex < UE_ARRAY_COUNT(LayerInfos); ++LayerIndex)
			{
				const uint8 ExpectedWeight = LayerIndex == ExpectedLayerIndex ? 255 : 0;
				if (StoredWeights[LayerIndex] != ExpectedWeight)
				{
					OutError = FString::Printf(
						TEXT("Saved surface weight mismatch at %s (layer=%d stored=%u expected=%u)."),
						Sample.Label, LayerIndex, StoredWeights[LayerIndex], ExpectedWeight);
					return false;
				}
			}

			if (Sample.bRequireLandscapeTrace)
			{
				const FVector TraceStart(Sample.XY.X * 100.0f, Sample.XY.Y * 100.0f, ExpectedHeightMeters * 100.0f + 50000.0f);
				const FVector TraceEnd(Sample.XY.X * 100.0f, Sample.XY.Y * 100.0f, ExpectedHeightMeters * 100.0f - 50000.0f);
				const ECollisionChannel QueryChannels[] = {
					ECC_Visibility,
					ECC_Camera,
					ECC_Pawn,
					ECC_Vehicle,
					ECC_WorldDynamic
				};
				for (const ECollisionChannel Channel : QueryChannels)
				{
					FHitResult Hit;
					FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OWSLandscapeValidation), false);
					if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, Channel, QueryParams)
						|| !Hit.GetActor()
						|| !Hit.GetActor()->IsA<ALandscapeProxy>()
						|| FMath::Abs(Hit.ImpactPoint.Z - ExpectedHeightMeters * 100.0f) > 5.0f)
					{
						OutError = FString::Printf(
							TEXT("Landscape collision trace failed at %s on channel %d."),
							Sample.Label, static_cast<int32>(Channel));
						return false;
					}
				}
			}
		}

		AOWSTestLabEnvironment* CampusManager = nullptr;
		for (TActorIterator<AOWSTestLabEnvironment> It(World); It; ++It)
		{
			if (It->ActorHasTag(CampusManagerTag))
			{
				CampusManager = *It;
				break;
			}
		}
		if (!CampusManager)
		{
			OutError = TEXT("Campus-offset validation could not find the retained campus manager.");
			return false;
		}

		const FVector CampusTraceStart(1500.0f, 0.0f, 50000.0f);
		const FVector CampusTraceEnd(1500.0f, 0.0f, -50000.0f);
		FHitResult CampusFoundationHit;
		FCollisionQueryParams FoundationQuery(SCENE_QUERY_STAT(OWSCampusFoundationValidation), false);
		if (!World->LineTraceSingleByChannel(
			CampusFoundationHit, CampusTraceStart, CampusTraceEnd, ECC_Visibility, FoundationQuery)
			|| CampusFoundationHit.GetActor() != CampusManager
			|| FMath::Abs(CampusFoundationHit.ImpactPoint.Z) > 1.0f)
		{
			OutError = TEXT("Campus foundation did not provide the expected Z=0 blocking surface.");
			return false;
		}

		FHitResult CampusLandscapeHit;
		FCollisionQueryParams UnderlayQuery(SCENE_QUERY_STAT(OWSCampusUnderlayValidation), false);
		UnderlayQuery.AddIgnoredActor(CampusManager);
		if (!World->LineTraceSingleByChannel(
			CampusLandscapeHit, CampusTraceStart, CampusTraceEnd, ECC_Visibility, UnderlayQuery)
			|| !CampusLandscapeHit.GetActor()
			|| !CampusLandscapeHit.GetActor()->IsA<ALandscapeProxy>()
			|| FMath::Abs(CampusLandscapeHit.ImpactPoint.Z + 50.0f) > 5.0f
			|| FMath::Abs(
				CampusFoundationHit.ImpactPoint.Z - CampusLandscapeHit.ImpactPoint.Z - 50.0f) > 5.0f)
		{
			OutError = TEXT("Campus Landscape underlay is not collision-valid and exactly 50 cm below the foundation.");
			return false;
		}
		return true;
	}

	static bool ValidateGeneratedWorld(UWorld* World, uint32 ExpectedSeed, FString& OutError)
	{
		if (!ValidateRuntimeGrid(World, OutError))
		{
			return false;
		}

		const AWorldSettings* WorldSettings = World->GetWorldSettings();
		if (!WorldSettings || !FMath::IsNearlyEqual(WorldSettings->KillZ, WorldKillZCm, 0.1f))
		{
			OutError = TEXT("World KillZ is not -150000 cm.");
			return false;
		}

		for (const TCHAR* PackageName : GeneratedPackageNames)
		{
			if (!FPackageName::DoesPackageExist(PackageName))
			{
				OutError = FString::Printf(TEXT("Generated asset package is missing: %s"), PackageName);
				return false;
			}
		}

		UWorldPartition* WorldPartition = World->GetWorldPartition();
		int32 StreamingProxyCount = 0;
		FWorldPartitionHelpers::ForEachActorDescInstance<ALandscapeStreamingProxy>(
			WorldPartition,
			[&StreamingProxyCount](const FWorldPartitionActorDescInstance*)
			{
				++StreamingProxyCount;
				return true;
			});
		if (StreamingProxyCount != ExpectedStreamingProxyCount)
		{
			OutError = FString::Printf(
				TEXT("Landscape streaming proxy count is %d; expected %d."),
				StreamingProxyCount,
				ExpectedStreamingProxyCount);
			return false;
		}

		int32 LandscapeCount = 0;
		bool bLandscapePropertiesValid = true;
		const FName ExpectedSeedTag(*FString::Printf(TEXT("OWS.OpenWorld.Seed.%u"), ExpectedSeed));
		FWorldPartitionHelpers::FForEachActorWithLoadingParams LandscapeParams;
		LandscapeParams.ActorClasses = { ALandscape::StaticClass() };
		FWorldPartitionHelpers::ForEachActorWithLoading(
			WorldPartition,
			[&LandscapeCount, &bLandscapePropertiesValid, &ExpectedSeedTag](const FWorldPartitionActorDescInstance* ActorDescInstance)
			{
				const ALandscape* Landscape = Cast<ALandscape>(ActorDescInstance->GetActor());
				if (!Landscape || !Landscape->ActorHasTag(LandscapeTag))
				{
					return true;
				}

				++LandscapeCount;
				const FVector ExpectedLocation(-HalfExtentMeters * 100.0f, -HalfExtentMeters * 100.0f, 0.0f);
				bLandscapePropertiesValid &= Landscape->GetActorLocation().Equals(ExpectedLocation, 0.1f);
				bLandscapePropertiesValid &= Landscape->GetActorScale3D().Equals(FVector(LandscapeScaleCm), 0.01f);
				bLandscapePropertiesValid &= Landscape->ComponentSizeQuads == QuadsPerComponent;
				bLandscapePropertiesValid &= Landscape->SubsectionSizeQuads == QuadsPerSection;
				bLandscapePropertiesValid &= Landscape->NumSubsections == SectionsPerComponent;
				bLandscapePropertiesValid &= Landscape->GetGridSize() == QuadsPerComponent * LandscapeGridSizeInComponents;
				bLandscapePropertiesValid &= !Landscape->GetIsSpatiallyLoaded();
				bLandscapePropertiesValid &= Landscape->ActorHasTag(ExpectedSeedTag);
				return true;
			},
			LandscapeParams);
		if (LandscapeCount != 1 || !bLandscapePropertiesValid)
		{
			OutError = TEXT("Generated Landscape master count/configuration is invalid.");
			return false;
		}

		int32 CityBlockCount = 0;
		int32 RouteMarkerCount = 0;
		int32 DistrictSignCount = 0;
		int32 BridgePieceCount = 0;
		int32 RecoveryAnchorCount = 0;
		int32 CampusManagerCount = 0;
		int32 TotalPlayerStartCount = 0;
		int32 GeneratedPlayerStartCount = 0;
		int32 LoadedStreamingProxyCount = 0;
		int32 LoadedLandscapeComponentCount = 0;
		bool bCampusManagerAlwaysLoaded = true;
		bool bLoadedLandscapeCollisionValid = true;
		bool bGeneratedGeometryCollisionValid = true;
		const AStaticMeshActor* CollisionQueryMeshActor = nullptr;
		TArray<FVector> GeneratedStartLocations;

		FWorldPartitionHelpers::FForEachActorWithLoadingParams ActorParams;
		ActorParams.bKeepReferences = true;
		// Load only the actor classes that participate in validation. Loading
		// every actor also constructs the parked vehicle skeletal meshes, but a
		// commandlet world has no rendering scene and cannot create their GPU
		// skin objects. None of those vehicles contribute to the checks below.
		ActorParams.ActorClasses = {
			ALandscapeStreamingProxy::StaticClass(),
			AStaticMeshActor::StaticClass(),
			ATextRenderActor::StaticClass(),
			ATargetPoint::StaticClass(),
			APlayerStart::StaticClass(),
			AOWSTestLabEnvironment::StaticClass()
		};
		FWorldPartitionHelpers::FForEachActorWithLoadingResult ActorResult;
		FWorldPartitionHelpers::ForEachActorWithLoading(
			WorldPartition,
			[&](const FWorldPartitionActorDescInstance* ActorDescInstance)
			{
				const AActor* Actor = ActorDescInstance->GetActor();
				if (!Actor)
				{
					return true;
				}
				CityBlockCount += Actor->ActorHasTag(CityBlockTag) ? 1 : 0;
				RouteMarkerCount += Actor->ActorHasTag(RouteMarkerTag) ? 1 : 0;
				DistrictSignCount += Actor->ActorHasTag(DistrictSignTag) ? 1 : 0;
				BridgePieceCount += Actor->ActorHasTag(BridgeTag) ? 1 : 0;
				RecoveryAnchorCount += Actor->ActorHasTag(RecoveryAnchorTag) ? 1 : 0;
				if (const ALandscapeStreamingProxy* Proxy = Cast<ALandscapeStreamingProxy>(Actor))
				{
					++LoadedStreamingProxyCount;
					LoadedLandscapeComponentCount += Proxy->LandscapeComponents.Num();
					bLoadedLandscapeCollisionValid &= Proxy->LandscapeComponents.Num() == LandscapeGridSizeInComponents * LandscapeGridSizeInComponents;
					bLoadedLandscapeCollisionValid &= Proxy->CollisionComponents.Num() == Proxy->LandscapeComponents.Num();
					bLoadedLandscapeCollisionValid &= Proxy->CollisionMipLevel == 0;
					bLoadedLandscapeCollisionValid &= Proxy->SimpleCollisionMipLevel == 1;
					bLoadedLandscapeCollisionValid &= Proxy->GetIsSpatiallyLoaded();
					for (const ULandscapeHeightfieldCollisionComponent* CollisionComponent : Proxy->CollisionComponents)
					{
						bLoadedLandscapeCollisionValid &= HasRequiredBlockingCollision(CollisionComponent);
					}
				}
				if (const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
					MeshActor && Actor->ActorHasTag(GeneratedTag))
				{
					const UStaticMeshComponent* Mesh = MeshActor->GetStaticMeshComponent();
					bGeneratedGeometryCollisionValid &= HasHardenedStaticMeshCollision(Mesh);
					if (!CollisionQueryMeshActor && Actor->ActorHasTag(CityBlockTag))
					{
						CollisionQueryMeshActor = MeshActor;
					}
				}
				if (Actor->ActorHasTag(CampusManagerTag))
				{
					++CampusManagerCount;
					bCampusManagerAlwaysLoaded &= !Actor->GetIsSpatiallyLoaded();
				}
				if (Actor->IsA<APlayerStart>())
				{
					++TotalPlayerStartCount;
					if (Actor->ActorHasTag(MultiplayerStartTag))
					{
						++GeneratedPlayerStartCount;
						GeneratedStartLocations.Add(Actor->GetActorLocation());
					}
				}
				return true;
			},
			ActorParams,
			ActorResult);

		if (LoadedStreamingProxyCount != ExpectedStreamingProxyCount
			|| LoadedLandscapeComponentCount != ComponentCountPerSide * ComponentCountPerSide
			|| !bLoadedLandscapeCollisionValid
			|| !bGeneratedGeometryCollisionValid)
		{
			OutError = FString::Printf(
				TEXT("Fresh-load collision/geometry validation failed (proxies=%d, components=%d, landscape-collision=%s, generated-collision=%s)."),
				LoadedStreamingProxyCount,
				LoadedLandscapeComponentCount,
				bLoadedLandscapeCollisionValid ? TEXT("valid") : TEXT("invalid"),
				bGeneratedGeometryCollisionValid ? TEXT("valid") : TEXT("invalid"));
			return false;
		}

		ALandscape* LoadedLandscape = nullptr;
		for (TActorIterator<ALandscape> It(World); It; ++It)
		{
			if (It->ActorHasTag(LandscapeTag))
			{
				LoadedLandscape = *It;
				break;
			}
		}
		if (!LoadedLandscape || !LoadedLandscape->LandscapeMaterial
			|| LoadedLandscape->LandscapeMaterial->GetPackage()->GetName() != TerrainMaterialPackage)
		{
			OutError = TEXT("Fresh-load Landscape material binding is invalid.");
			return false;
		}

		ULandscapeInfo* LoadedLandscapeInfo = LoadedLandscape->GetLandscapeInfo();
		int32 ExtentMinX = 0;
		int32 ExtentMinY = 0;
		int32 ExtentMaxX = 0;
		int32 ExtentMaxY = 0;
		int32 FreshComponentCount = 0;
		if (!LoadedLandscapeInfo
			|| !LoadedLandscapeInfo->GetLandscapeExtent(ExtentMinX, ExtentMinY, ExtentMaxX, ExtentMaxY))
		{
			OutError = TEXT("Fresh-load LandscapeInfo/extent is unavailable.");
			return false;
		}
		LoadedLandscapeInfo->ForAllLandscapeComponents([&FreshComponentCount](ULandscapeComponent*)
		{
			++FreshComponentCount;
		});
		if (ExtentMinX != 0 || ExtentMinY != 0 || ExtentMaxX != QuadsPerSide || ExtentMaxY != QuadsPerSide
			|| FreshComponentCount != ComponentCountPerSide * ComponentCountPerSide)
		{
			OutError = TEXT("Fresh-load Landscape extent/component count is invalid.");
			return false;
		}

		const TMap<FName, FLandscapeTargetLayerSettings>& TargetLayers = LoadedLandscape->GetTargetLayers();
		auto HasExpectedLayer = [&TargetLayers](FName LayerName, const TCHAR* ExpectedPackage)
		{
			const FLandscapeTargetLayerSettings* Settings = TargetLayers.Find(LayerName);
			return Settings
				&& Settings->LayerInfoObj
				&& Settings->LayerInfoObj->GetPackage()->GetName() == ExpectedPackage;
		};
		if (!HasExpectedLayer(TEXT("Grass"), GrassLayerPackage)
			|| !HasExpectedLayer(TEXT("Asphalt"), AsphaltLayerPackage)
			|| !HasExpectedLayer(TEXT("Gravel"), GravelLayerPackage)
			|| !HasExpectedLayer(TEXT("Dirt"), DirtLayerPackage))
		{
			OutError = TEXT("Fresh-load Landscape target-layer bindings are invalid.");
			return false;
		}
		if (!ValidateTerrainSamples(World, LoadedLandscape, ExpectedSeed, OutError)
			|| !ValidateStaticMeshCollisionQueries(World, CollisionQueryMeshActor, OutError))
		{
			return false;
		}

		if (CityBlockCount < 100
			|| RouteMarkerCount < 90
			|| DistrictSignCount < 14
			|| BridgePieceCount < 7
			|| RecoveryAnchorCount < 7)
		{
			OutError = FString::Printf(
				TEXT("Generated actor inventory is incomplete (blocks=%d markers=%d signs=%d bridge=%d anchors=%d)."),
				CityBlockCount,
				RouteMarkerCount,
				DistrictSignCount,
				BridgePieceCount,
				RecoveryAnchorCount);
			return false;
		}

		if (CampusManagerCount != 1 || !bCampusManagerAlwaysLoaded)
		{
			OutError = TEXT("Central campus manager is missing or spatially loaded.");
			return false;
		}

		if (TotalPlayerStartCount < 5 || GeneratedPlayerStartCount != 4)
		{
			OutError = FString::Printf(
				TEXT("PlayerStart validation failed (total=%d, generated=%d; expected at least 5 and exactly 4)."),
				TotalPlayerStartCount,
				GeneratedPlayerStartCount);
			return false;
		}
		for (int32 A = 0; A < GeneratedStartLocations.Num(); ++A)
		{
			for (int32 B = A + 1; B < GeneratedStartLocations.Num(); ++B)
			{
				if (FVector::Dist2D(GeneratedStartLocations[A], GeneratedStartLocations[B]) < 5000.0f)
				{
					OutError = TEXT("Generated multiplayer PlayerStarts are not separated by at least 50 m.");
					return false;
				}
			}
		}

		return true;
	}

	static bool RepairGeneratedLandscapeMaterials(UWorld* World, FString& OutError)
	{
		if (!World || !World->GetWorldPartition())
		{
			OutError = TEXT("Material repair requires a loaded World Partition world.");
			return false;
		}

		ALandscape* Landscape = nullptr;
		TArray<ALandscapeStreamingProxy*> StreamingProxies;
		FWorldPartitionHelpers::FForEachActorWithLoadingParams Params;
		Params.bKeepReferences = true;
		Params.ActorClasses = { ALandscape::StaticClass(), ALandscapeStreamingProxy::StaticClass() };
		FWorldPartitionHelpers::FForEachActorWithLoadingResult Result;
		FWorldPartitionHelpers::ForEachActorWithLoading(
			World->GetWorldPartition(),
			[&](const FWorldPartitionActorDescInstance* ActorDescInstance)
			{
				if (ALandscape* Candidate = Cast<ALandscape>(ActorDescInstance->GetActor());
					Candidate && Candidate->ActorHasTag(LandscapeTag))
				{
					Landscape = Candidate;
				}
				else if (ALandscapeStreamingProxy* Proxy =
					Cast<ALandscapeStreamingProxy>(ActorDescInstance->GetActor()))
				{
					StreamingProxies.AddUnique(Proxy);
				}
				return true;
			},
			Params,
			Result);

		if (!Landscape || StreamingProxies.Num() != ExpectedStreamingProxyCount)
		{
			OutError = FString::Printf(
				TEXT("Material repair loaded landscape=%s and %d/%d streaming proxies."),
				Landscape ? TEXT("true") : TEXT("false"),
				StreamingProxies.Num(),
				ExpectedStreamingProxyCount);
			return false;
		}

		const FString MaterialObjectPath = FString::Printf(
			TEXT("%s.%s"),
			TerrainMaterialPackage,
			*FPackageName::GetShortName(TerrainMaterialPackage));
		UMaterialInterface* TerrainMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialObjectPath);
		if (!TerrainMaterial)
		{
			OutError = FString::Printf(TEXT("Could not load generated terrain material %s."), *MaterialObjectPath);
			return false;
		}

		Landscape->Modify();
		Landscape->LandscapeMaterial = TerrainMaterial;
		for (ALandscapeStreamingProxy* Proxy : StreamingProxies)
		{
			Proxy->Modify();
			for (ULandscapeComponent* Component : Proxy->LandscapeComponents)
			{
				if (Component)
				{
					Component->Modify();
					Component->UpdateMaterialInstances();
				}
			}
			Proxy->MarkPackageDirty();
		}
		Landscape->MarkPackageDirty();
		UMaterialInterface::SubmitRemainingJobsForWorld(World);
		FAssetCompilingManager::Get().FinishAllCompilation();
		FAssetCompilingManager::Get().ProcessAsyncTasks();
		FlushRenderingCommands();

		TArray<UPackage*> PackagesToSave;
		if (UPackage* LandscapePackage = Landscape->GetExternalPackage())
		{
			PackagesToSave.AddUnique(LandscapePackage);
		}
		for (ALandscapeStreamingProxy* Proxy : StreamingProxies)
		{
			if (UPackage* Package = Proxy->GetExternalPackage())
			{
				PackagesToSave.AddUnique(Package);
			}
		}
		PackagesToSave.Sort([](const UPackage& A, const UPackage& B)
		{
			return A.GetName() < B.GetName();
		});

		constexpr int32 SaveBatchSize = 16;
		UWorldPartition::FDisableNonDirtyActorTrackingScope TrackingScope(World->GetWorldPartition(), true);
		for (int32 BatchStart = 0; BatchStart < PackagesToSave.Num(); BatchStart += SaveBatchSize)
		{
			TArray<UPackage*> Batch;
			const int32 BatchEnd = FMath::Min(BatchStart + SaveBatchSize, PackagesToSave.Num());
			for (int32 Index = BatchStart; Index < BatchEnd; ++Index)
			{
				Batch.Add(PackagesToSave[Index]);
			}
			if (!UEditorLoadingAndSavingUtils::SavePackages(Batch, true))
			{
				OutError = FString::Printf(
					TEXT("Failed to save repaired Landscape material package batch beginning at %d."),
					BatchStart);
				return false;
			}
		}

		return true;
	}

	static bool SaveGeneratedWorld(UWorld* World, const FString& MapPackageName, const FGeneratedAssets& Assets, FString& OutError)
	{
		if (!UEditorLoadingAndSavingUtils::SavePackages(Assets.Packages, false))
		{
			OutError = TEXT("Failed to save one or more generated material/layer packages.");
			return false;
		}

		TSet<UPackage*> LandscapePackageSet;
		for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
		{
			if (UPackage* LandscapePackage = It->GetExternalPackage())
			{
				LandscapePackageSet.Add(LandscapePackage);
			}
		}

		TSet<UPackage*> ExternalPackageSet;
		for (UPackage* Package : World->PersistentLevel->GetLoadedExternalObjectPackages())
		{
			if (Package && !LandscapePackageSet.Contains(Package))
			{
				ExternalPackageSet.Add(Package);
			}
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UPackage* ExternalPackage = It->GetExternalPackage();
				ExternalPackage && !LandscapePackageSet.Contains(ExternalPackage))
			{
				ExternalPackageSet.Add(ExternalPackage);
			}
		}

		TArray<UPackage*> ExternalPackages = ExternalPackageSet.Array();
		ExternalPackages.Sort([](const UPackage& A, const UPackage& B)
		{
			return A.GetName() < B.GetName();
		});
		{
			UWorldPartition::FDisableNonDirtyActorTrackingScope TrackingScope(World->GetWorldPartition(), true);
			if (!UEditorLoadingAndSavingUtils::SavePackages(ExternalPackages, false))
			{
				OutError = TEXT("Failed to save one or more generated external actor packages.");
				return false;
			}
		}

		World->GetPackage()->MarkPackageDirty();
		if (!UEditorLoadingAndSavingUtils::SaveMap(World, MapPackageName))
		{
			OutError = FString::Printf(TEXT("Failed to save generated map %s."), *MapPackageName);
			return false;
		}
		return true;
	}

	static bool GenerateWorld(UWorld* World, const FString& MapPackageName, uint32 Seed, FString& OutError)
	{
		if (!PreflightGeneration(World, OutError))
		{
			return false;
		}

		if (!ConfigureWorldPartition(World, OutError) || !MakeCampusManagerAlwaysLoaded(World, OutError))
		{
			return false;
		}

		AWorldSettings* WorldSettings = World->GetWorldSettings();
		if (!WorldSettings)
		{
			OutError = TEXT("Target map has no WorldSettings.");
			return false;
		}
		WorldSettings->Modify();
		WorldSettings->KillZ = WorldKillZCm;
		WorldSettings->MarkPackageDirty();

		FGeneratedAssets Assets;
		if (!CreateGeneratedAssets(Assets))
		{
			OutError = TEXT("Failed to create the project-owned generated material/layer assets.");
			return false;
		}

		// Material PostEditChange compiles the freshly generated shader maps
		// asynchronously. Landscape import immediately creates render resources from
		// those materials, so finish the shader jobs and drain their render commands
		// before spawning/importing the Landscape. This mirrors UE's own blocking
		// shader-compilation + render-flush pattern and avoids racing the Landscape
		// render-resource setup in unattended commandlets.
		if (GShaderCompilingManager)
		{
			GShaderCompilingManager->FinishAllCompilation();
		}
		FlushRenderingCommands();

		UE_LOG(LogOWSOpenWorldGenerator, Display,
			TEXT("Generating deterministic 8129 x 8129 Landscape in bounded 16 x 16-component regions (seed %u)..."), Seed);
		ALandscape* Landscape = ImportLandscape(World, Assets, Seed, OutError);
		if (!Landscape)
		{
			return false;
		}

		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!CubeMesh)
		{
			OutError = TEXT("Engine BasicShapes Cube mesh is unavailable.");
			return false;
		}

		if (!SpawnCityBlocks(World, CubeMesh, Assets.CityMaterial, Seed)
			|| !SpawnBridgeAndUnderpass(World, CubeMesh, Assets.BridgeMaterial, Assets.ConcreteMaterial)
			|| !SpawnRouteMarkers(World, Seed, CubeMesh, Assets.MarkerMaterial)
			|| !SpawnSignsAndRecoveryAnchors(World, Seed, CubeMesh, Assets.MarkerMaterial)
			|| !SpawnMultiplayerStarts(World))
		{
			OutError = TEXT("Failed while spawning project-owned open-world test geometry.");
			return false;
		}

		if (!SaveGeneratedWorld(World, MapPackageName, Assets, OutError))
		{
			return false;
		}

		return true;
	}
}

UOWSGenerateOpenWorldLabCommandlet::UOWSGenerateOpenWorldLabCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

bool UOWSGenerateOpenWorldLabCommandlet::GenerateEditorWorld(UWorld* World, uint32 Seed, FString& OutError)
{
	if (!World || !World->GetOutermost()->GetName().Equals(
		OWSOpenWorld::ExpectedTargetMapPackage,
		ESearchCase::CaseSensitive))
	{
		OutError = FString::Printf(
			TEXT("Editor generation requires the exact target map '%s'."),
			OWSOpenWorld::ExpectedTargetMapPackage);
		return false;
	}

	return OWSOpenWorld::GenerateWorld(
		World,
		OWSOpenWorld::ExpectedTargetMapPackage,
		Seed,
		OutError);
}

int32 UOWSGenerateOpenWorldLabCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParameterValues;
	ParseCommandLine(*Params, Tokens, Switches, ParameterValues);

	auto HasSwitch = [&Switches](const TCHAR* Name)
	{
		return Switches.ContainsByPredicate(
			[Name](const FString& Switch)
			{
				return Switch.Equals(Name, ESearchCase::IgnoreCase);
			});
	};

	const bool bGenerate = HasSwitch(TEXT("Generate"));
	const bool bValidateOnly = HasSwitch(TEXT("ValidateOnly"));
	const bool bRepairMaterials = HasSwitch(TEXT("RepairMaterials"));
	const int32 SelectedModeCount =
		(bGenerate ? 1 : 0) + (bValidateOnly ? 1 : 0) + (bRepairMaterials ? 1 : 0);
	if (SelectedModeCount != 1)
	{
		UE_LOG(LogOWSOpenWorldGenerator, Error,
			TEXT("Specify exactly one of -Generate, -ValidateOnly, or -RepairMaterials."));
		return 1;
	}

	if (Tokens.Num() != 1)
	{
		UE_LOG(LogOWSOpenWorldGenerator, Error, TEXT("Specify exactly one target map package token."));
		return 1;
	}

	FString MapPackageName = Tokens[0];
	if (!FPackageName::IsValidLongPackageName(MapPackageName)
		|| !MapPackageName.EndsWith(OWSOpenWorld::RequiredMapSuffix, ESearchCase::CaseSensitive)
		|| !MapPackageName.Equals(OWSOpenWorld::ExpectedTargetMapPackage, ESearchCase::CaseSensitive))
	{
		UE_LOG(LogOWSOpenWorldGenerator, Error,
			TEXT("Target must be exactly '%s' (the protected source map is never accepted); received '%s'."),
			OWSOpenWorld::ExpectedTargetMapPackage,
			*MapPackageName);
		return 1;
	}

	uint32 Seed = OWSOpenWorld::DefaultSeed;
	if (const FString* SeedString = ParameterValues.Find(TEXT("Seed")))
	{
		if (!LexTryParseString(Seed, **SeedString))
		{
			UE_LOG(LogOWSOpenWorldGenerator, Error, TEXT("Invalid unsigned -Seed value '%s'."), **SeedString);
			return 1;
		}
	}

	UWorld::InitializationValues InitializationValues;
	InitializationValues.RequiresHitProxies(false);
	InitializationValues.ShouldSimulatePhysics(false);
	InitializationValues.EnableTraceCollision(true);
	InitializationValues.CreateNavigation(false);
	InitializationValues.CreateAISystem(false);
	InitializationValues.AllowAudioPlayback(false);
	InitializationValues.CreatePhysicsScene(true);
	FScopedEditorWorld EditorWorld(MapPackageName, InitializationValues, EWorldType::Editor);
	UWorld* World = EditorWorld.GetWorld();
	if (!World)
	{
		UE_LOG(LogOWSOpenWorldGenerator, Error, TEXT("Could not load and initialize map %s."), *MapPackageName);
		return 1;
	}

	FString Error;
	bool bSucceeded = false;
	if (bGenerate)
	{
		bSucceeded = OWSOpenWorld::GenerateWorld(World, MapPackageName, Seed, Error);
		if (bSucceeded)
		{
			UE_LOG(LogOWSOpenWorldGenerator, Display, TEXT("OWS_GENERATION_SUCCESS"));
		}
	}
	else if (bValidateOnly)
	{
		bSucceeded = OWSOpenWorld::ValidateGeneratedWorld(World, Seed, Error);
		if (bSucceeded)
		{
			UE_LOG(LogOWSOpenWorldGenerator, Display, TEXT("OWS_VALIDATION_SUCCESS"));
		}
	}
	else
	{
		bSucceeded = OWSOpenWorld::RepairGeneratedLandscapeMaterials(World, Error);
		if (bSucceeded)
		{
			UE_LOG(LogOWSOpenWorldGenerator, Display, TEXT("OWS_MATERIAL_REPAIR_SUCCESS"));
		}
	}

	if (!bSucceeded)
	{
		UE_LOG(LogOWSOpenWorldGenerator, Error, TEXT("%s"), *Error);
		return 1;
	}
	return 0;
}
