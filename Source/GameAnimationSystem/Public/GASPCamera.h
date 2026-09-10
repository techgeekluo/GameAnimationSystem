// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GASPCameraSettings.h"
#include "GASPCamera.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModeChanged, EGASPViewMode);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShoulderModeChanged, EGASPShoulderMode);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveAiming, bool);

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

	UFUNCTION(BlueprintPure)
	void GetViewInfo(FMinimalViewInfo& ViewInfo) const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostLoad() override;
	virtual void OnRegister() override;
	virtual void Activate(bool bReset) override;
	virtual void RegisterComponentTickFunctions(bool bRegister) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void CompleteParallelAnimationEvaluation(bool bDoPostAnimationEvaluation) override;

	FOnViewModeChanged		OnViewModeChangedDelegate;
	FOnShoulderModeChanged  OnShoulderModeChangedDelegate;
	FOnActiveAiming			OnActiveAimingDelegate;

private:
	UPROPERTY()
	class UAnimInstance* CameraAnimInstance;

	UPROPERTY()
	class APawn* OwnerPawn;


};
