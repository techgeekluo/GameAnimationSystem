// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GASPCameraSettings.h"
#include "GASPCamera.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModeChanged, EGASPViewMode);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShoulderModeChanged, EGASPShoulderMode);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveCameraAiming, bool);

/**
 * 
 */
UCLASS(HideCategories = ("ComponentTick", "Clothing", "Physics", "MasterPoseComponent", "Collision", "AnimationRig",
	"Lighting", "Deformer", "Rendering", "PathTracing", "HLOD", "Navigation", "VirtualTexture", "SkeletalMesh",
	"LeaderPoseComponent", "Optimization", "LOD", "MaterialParameters", "TextureStreaming", "Mobile", "RayTracing"))
class GAMEANIMATIONSYSTEM_API UGASPCamera : public USkeletalMeshComponent
{
	GENERATED_BODY()
	
public:
	UGASPCamera();

	FOnViewModeChanged		OnViewModeChangedDelegate;
	FOnShoulderModeChanged  OnShoulderModeChangedDelegate;
	FOnActiveCameraAiming	OnActiveCameraAimingDelegate;

	UFUNCTION(BlueprintPure)
	void GetViewInfo(FMinimalViewInfo& ViewInfo) const;

	UFUNCTION(BlueprintCallable)
	void SetViewMode(EGASPViewMode NewViewMode, bool bForce = false);

	UFUNCTION(BlueprintCallable)
	void SetShoulderMode(EGASPShoulderMode NewShoulderMode, bool bForce = false);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SetViewMode(EGASPViewMode NewViewMode);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostLoad() override;
	virtual void OnRegister() override;
	virtual void Activate(bool bReset) override;
	virtual void RegisterComponentTickFunctions(bool bRegister) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void CompleteParallelAnimationEvaluation(bool bDoPostAnimationEvaluation) override;

protected:
	void TickCamera(float DeltaTime, bool bAllowLag = true);

	FRotator CalcCameraRotation(const FRotator& CameraTargetRotation, float DeltaTime, bool bAllowLag) const;
	FVector CalcPivotLagLocation(const FQuat& CameraYawRotation, float DeltaTime, bool bAllowLag) const;
	FVector CalcCollisionFixLocation(const FVector& CameraTargetLocation, const FVector& PivotOffset,
									 float DeltaTime, bool bAllowLag, float& NewTraceDistanceRatio) const;
	bool TryAdjustTraceStartLocation(FVector& Location, float MeshScale) const;

private:
	UPROPERTY()
	class UAnimInstance* CameraAnimInst;

	UPROPERTY()
	class APawn* OwnerPawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Settings", meta = (AllowPrivateAccess = "true"))
	UGASPCameraSettings* CameraSettings;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Camera|ViewMode", meta = (AllowPrivateAccess = "true"))
	EGASPViewMode ViewMode = EGASPViewMode::ThirdPerson;

	FVector CameraLocation;
	FRotator CameraRotation;
	float CameraFieldOfView;

	FVector PivotLocation;
	FVector PivotTargetLocation;
	FVector PivotLagLocation;

	float PreviousGlobalTimeDilation = 1.f;
	float TraceDistanceRatio = 1.f;
	bool bHideOwner = false;	// FP与TP切换时，隐藏角色模型

public:
	FORCEINLINE UGASPCameraSettings* GetCameraSettings() const { return CameraSettings; }
	FORCEINLINE EGASPViewMode GetViewMode() const { return ViewMode; }
};
