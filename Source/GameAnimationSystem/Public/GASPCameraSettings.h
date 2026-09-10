// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GASPCameraSettings.generated.h"

UENUM(BlueprintType)
enum class EGASPViewMode : uint8
{
	FirstPerson,
	ThirdPerson,
	TopDown,		// TODO
};

UENUM(BlueprintType)
enum class EGASPShoulderMode : uint8
{
	Middle,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct GAMEANIMATIONSYSTEM_API FGASPFirstPersonSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 5, ClampMax = 175, ForceUnits = "deg"))
	float FieldOfView = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CameraSocketName = FName("FirstPersonCamera");
};

USTRUCT(BlueprintType)
struct GAMEANIMATIONSYSTEM_API FGASPThirdPersonSettings
{
	GENERATED_BODY()
	
	float GetShoulderOffsetLength() const
	{
		return ShoulderMode == EGASPShoulderMode::Middle ? 0.f
			: (ShoulderMode == EGASPShoulderMode::Left ? -ShoulderOffset : ShoulderOffset);
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 5, ClampMax = 175, ForceUnits = "deg"))
	float FieldOfView = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGASPShoulderMode ShoulderMode = EGASPShoulderMode::Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 90, ForceUnits = "cm"))
	float ShoulderOffset = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ForceUnits = "cm"))
	float TraceRadius = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TraceOverrideOffset = FVector(0.f, 0.f, 40.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle))
	bool bEnableTraceDistanceSmoothing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bEnableTraceDistanceSmoothing"))
	float TraceDistanceSmoothingHalfLife = 0.2f;

	EGASPShoulderMode LastShoulderMode = EGASPShoulderMode::Right;
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class GAMEANIMATIONSYSTEM_API UGASPCameraSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverrideFieldOfView = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 5, ClampMax = 175, EditCondition = "bOverrideFieldOfView", ForceUnits = "deg"))
	float FieldOfViewOverride = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowLag = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIgnoreTimeDilation = true;  // 相机更新是否忽略时间膨胀

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TeleportDistanceThreshold = 200.f;  // 角色瞬移距离阈值

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ViewMode")
	FGASPFirstPersonSettings FP_Settings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ViewMode")
	FGASPThirdPersonSettings TP_Settings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PostProcess")
	float PostProcessBlendWeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PostProcess")
	FPostProcessSettings PostProcess;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDebugDrawLag = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDebugDrawCollision = false;
};
