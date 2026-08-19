// Copyright (c) 2026 Zhengyi Miao (github.com/myoozy)

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "VehicleInputStructs.generated.h"

USTRUCT(BlueprintType)
struct FVehicleInputAxisConfig
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVector2f InterpSpeed = FVector2f(5.f, 5.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UCurveFloat* ResponseCurve = nullptr;

    FVehicleInputAxisConfig(
        FVector2f newInterpSpeed = FVector2f(5.f, 5.f),
        UCurveFloat* newResponseCurve = nullptr)
    {
        InterpSpeed = newInterpSpeed;
        ResponseCurve = newResponseCurve;
    }

    static float InterpInputValueConstant(
        float Current,
        float Target,
        float DeltaTime,
        FVector2f Speed
    )
    {
        float s = (Target < SMALL_NUMBER) ? Speed.Y : Speed.X;
        return (s <= 0) ? Target : FMath::FInterpConstantTo(Current, Target, DeltaTime, s);
    }

    static float InterpInputValue(
        float Current,
        float Target,
        float DeltaTime,
        FVector2f Speed
    )
    {
        float s = (Target < SMALL_NUMBER) ? Speed.Y : Speed.X;
        return (s <= 0) ? Target : FMath::FInterpTo(Current, Target, DeltaTime, s);
    }
};

USTRUCT(BlueprintType)
struct FVehiclInputConfig
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputAxisConfig Throttle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputAxisConfig Brake;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputAxisConfig Clutch;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputAxisConfig Handbrake = FVehicleInputAxisConfig(FVector2f(15.f, 15.f));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputAxisConfig Steering = FVehicleInputAxisConfig(FVector2f(2.5f, 2.5f));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UCurveFloat* HighSpeedSteeringScale = nullptr;
};

USTRUCT(BlueprintType)
struct FVehicleInputAssistConfig
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ToolTip = "engage clutch when eg. changing gear or low rpm"))
    bool bAutomaticClutch = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ToolTip = "disable AutomaticClutch, and disable throttle when in N gear"))
    bool bEVClutchLogic = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ToolTip = "at which rpm the clutch should be (gradually) released"))
    FVector2f AutoClutchRange = FVector2f(1200, 2500);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bRevMatching = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RevMatchMaxThrottle = 0.6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bAutoHold = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ToolTip = "Releases the brake of the axle if the torque weight(normalized) is > 0.5"))
    bool bBurnOutAssist = true;
};

USTRUCT(BlueprintType)
struct FVehicleInputState
{
    GENERATED_USTRUCT_BODY()

    //input
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float Throttle = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float Brake = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float Clutch = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float Handbrake = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    float Steering = 0.f;
};

USTRUCT(BlueprintType)
struct FVehicleInputPipeline
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputState Raw;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputState Smoothened;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    FVehicleInputState Final;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    bool bSwitchThrottleAndBrake = false;
};
