// Copyright: Jichao Luo


#include "GASPCamera.h"
#include "GASPCameraInterface.h"
#include "GASPFunctionLibrary.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/OverlapResult.h"

UGASPCamera::UGASPCamera()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bTickInEditor = false;
	bHiddenInGame = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CameraMeshAsset(
		TEXT("/GameAnimationSystem/Blueprints/Camera/SKM_MatineeCamera.SKM_MatineeCamera")
	);
	if (CameraMeshAsset.Succeeded()) { SetSkeletalMesh(CameraMeshAsset.Object); }

	static ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Camera(
		TEXT("/GameAnimationSystem/Blueprints/Camera/ABP_GASPCamera.ABP_GASPCamera_C")
	);
	if (ABP_Camera.Succeeded()) { SetAnimInstanceClass(ABP_Camera.Class); }

	static ConstructorHelpers::FObjectFinder<UGASPCameraSettings> CameraSettingsAsset(
		TEXT("/GameAnimationSystem/Blueprints/Camera/DA_CameraSettings.DA_CameraSettings")
	);
	if (CameraSettingsAsset.Succeeded()) { CameraSettings = CameraSettingsAsset.Object; }
}

void UGASPCamera::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UGASPCamera, ViewMode, COND_OwnerOnly);
}

void UGASPCamera::PostLoad()
{
	Super::PostLoad();

	// 相机骨骼网格体不会渲染，但依然需要更新动画蓝图，在资产加载完毕后强制覆盖
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void UGASPCamera::OnRegister()
{
	Super::OnRegister();

	CameraAnimInst = GetAnimInstance();
	OwnerPawn = Cast<APawn>(GetOwner());
}

void UGASPCamera::Activate(bool bReset)
{
	if (bReset || ShouldActivate())
	{
		TickCamera(0.0f, false);  // 相机激活时强制更新一次
	}

	Super::Activate(bReset);
}

void UGASPCamera::RegisterComponentTickFunctions(bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);

	AddTickPrerequisiteActor(GetOwner());  // 确保在角色的Tick之后Tick
}

void UGASPCamera::BeginPlay()
{
	Super::BeginPlay();

	checkf(CameraSettings, TEXT("CameraSettings is nullptr, please fill it in UMMAlsCameraComponent"));
	SetViewMode(ViewMode, true);
	SetShoulderMode(CameraSettings->TP_Settings.ShoulderMode, true);
}

void UGASPCamera::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (OwnerPawn && CameraSettings && CameraSettings->bIgnoreTimeDilation)
	{
		const float TimeDilation = PreviousGlobalTimeDilation * OwnerPawn->CustomTimeDilation;
		DeltaTime = TimeDilation > UE_SMALL_NUMBER ? DeltaTime / TimeDilation : GetWorld()->DeltaRealTimeSeconds;
	}
	PreviousGlobalTimeDilation = GetWorld()->GetWorldSettings()->GetEffectiveTimeDilation();

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 在骨骼动画并行计算的时候不直接进入TickCamera
	if (!IsRunningParallelEvaluation())
	{
		TickCamera(DeltaTime, CameraSettings->bAllowLag);
	}
}

void UGASPCamera::CompleteParallelAnimationEvaluation(bool bDoPostAnimationEvaluation)
{
	Super::CompleteParallelAnimationEvaluation(bDoPostAnimationEvaluation);

	TickCamera(
		CameraAnimInst ? CameraAnimInst->GetDeltaSeconds() : 0.f, 
		CameraSettings ? CameraSettings->bAllowLag : true
	);
}

#pragma optimize("", off)
void UGASPCamera::TickCamera(float DeltaTime, bool bAllowLag)
{
	if (!OwnerPawn || !CameraAnimInst || !CameraSettings) return;
	if (!OwnerPawn->Implements<UGASPCameraInterface>()) return;

	// TODO: 是否考虑BaseMovement?

	const FRotator CameraTargetRot = OwnerPawn->GetViewRotation();

	const FVector PreviousPivotTargetLoc = PivotTargetLocation;
	PivotTargetLocation = IGASPCameraInterface::Execute_GetThirdPersonPivotLocation(OwnerPawn);

	const float FirstPersonOverride = FMath::Clamp(CameraAnimInst->GetCurveValue(FName("FirstPersonOverride")), 0.0f, 1.0f);
	if (FAnimWeight::IsFullWeight(FirstPersonOverride))
	{
		// 角色完全进入第一人称模式，跳过其他计算
		PivotLagLocation = PivotTargetLocation;
		PivotLocation = PivotTargetLocation;

		CameraLocation = IGASPCameraInterface::Execute_GetFirstPersonCameraLocation(OwnerPawn);
		CameraRotation = CameraTargetRot;
		CameraFieldOfView = CameraSettings->FP_Settings.FieldOfView;

		// HideOwner是为了在视角切换时避免穿模现象
		if (bHideOwner)
		{
			bHideOwner = false;
			IGASPCameraInterface::Execute_SetOwnerMeshNoSee(OwnerPawn, false);
		}
		return;
	}
	else if (FirstPersonOverride > 0.7f)
	{
		if (!bHideOwner)
		{
			bHideOwner = true;
			IGASPCameraInterface::Execute_SetOwnerMeshNoSee(OwnerPawn, true);
		}
	}
	else
	{
		if (bHideOwner)
		{
			bHideOwner = false;
			IGASPCameraInterface::Execute_SetOwnerMeshNoSee(OwnerPawn, false);
		}
	}

	// 如果角色瞬移，强制禁用相机延迟，TeleportDistanceThreshold小于等于0表示禁用瞬移检测
	// 采用平方距离避免开方计算，提升性能
	bAllowLag &= CameraSettings->TeleportDistanceThreshold <= 0.0f ||
		FVector::DistSquared(PreviousPivotTargetLoc, PivotTargetLocation) <= FMath::Square(CameraSettings->TeleportDistanceThreshold);

	// 计算相机旋转
	// TODO: 是否考虑“MovementBase 的相对旋转”的影响？
	CameraRotation = CalcCameraRotation(CameraTargetRot, DeltaTime, bAllowLag);
	const FQuat CameraYawRotation = FQuat(FVector::ZAxisVector, FMath::DegreesToRadians(CameraRotation.Yaw));

	// 计算Pivot延迟位置
	// TODO: 是否考虑“MovementBase 的相对旋转”的影响？
	PivotLagLocation = CalcPivotLagLocation(CameraYawRotation, DeltaTime, bAllowLag);

	// 计算Pivot位置
	const FVector PivotOffset = IGASPCameraInterface::Execute_GetOwnerMeshQuat(OwnerPawn).RotateVector(
		FVector{
			CameraAnimInst->GetCurveValue(FName("PivotOffsetX")),
			CameraAnimInst->GetCurveValue(FName("PivotOffsetY")),
			CameraAnimInst->GetCurveValue(FName("PivotOffsetZ"))
		} * IGASPCameraInterface::Execute_GetOwnerMeshScale(OwnerPawn).Z
	);
	PivotLocation = PivotLagLocation + PivotOffset;

	// 计算TargetCameraLocation
	const FVector CameraOffset = CameraRotation.RotateVector(
		FVector{
			CameraAnimInst->GetCurveValue(FName("CameraOffsetX")),
			CameraAnimInst->GetCurveValue(FName("CameraOffsetY")),
			CameraAnimInst->GetCurveValue(FName("CameraOffsetZ"))
		} * IGASPCameraInterface::Execute_GetOwnerMeshScale(OwnerPawn).Z
	);
	const FVector CameraTargetLoc = PivotLocation + CameraOffset;

	// 修正相机碰撞
	const FVector CameraFinalLoc = CalcCollisionFixLocation(CameraTargetLoc, PivotOffset, DeltaTime, bAllowLag, TraceDistanceRatio);
	if (FAnimWeight::IsRelevant(FirstPersonOverride))
	{
		UE_LOG(LogTemp, Log, TEXT("FirstPersonOverride: %f"), FirstPersonOverride);
		FVector FPCameraLoc = IGASPCameraInterface::Execute_GetFirstPersonCameraLocation(OwnerPawn);
		CameraLocation = FMath::Lerp(CameraFinalLoc, FPCameraLoc, FirstPersonOverride);
		CameraFieldOfView = FMath::Lerp(CameraSettings->TP_Settings.FieldOfView, CameraSettings->FP_Settings.FieldOfView, FirstPersonOverride);
	}
	else
	{
		CameraLocation = CameraFinalLoc;
		CameraFieldOfView = CameraSettings->TP_Settings.FieldOfView;
	}

	if (CameraSettings->bOverrideFieldOfView)
	{
		CameraFieldOfView = CameraSettings->FieldOfViewOverride;
	}

	const float FovOffset = CameraAnimInst->GetCurveValue(FName("FovOffset"));
	CameraFieldOfView = FMath::Clamp(CameraFieldOfView + FovOffset, 5.f, 175.f);

	if (CameraSettings->bDebugDrawLag)
	{
		DrawDebugSphere(GetWorld(), PivotTargetLocation, 12.f, 12, FColor::Orange, false, DeltaTime);
		DrawDebugSphere(GetWorld(), PivotLagLocation, 12.f, 12, FColor::Blue, false, DeltaTime);
		DrawDebugSphere(GetWorld(), PivotLocation, 12.f, 12, FColor::Purple, false, DeltaTime);
		DrawDebugSphere(GetWorld(), CameraLocation, 12.f, 12, FColor::Black, false, DeltaTime);
		DrawDebugLine(GetWorld(), PivotTargetLocation, PivotLagLocation, FColor::Orange, false, DeltaTime);
		DrawDebugLine(GetWorld(), PivotLagLocation, PivotLocation, FColor::Blue, false, DeltaTime);
		DrawDebugLine(GetWorld(), PivotLocation, CameraLocation, FColor::Purple, false, DeltaTime);
	}
}
#pragma optimize("", on)

FRotator UGASPCamera::CalcCameraRotation(const FRotator& CameraTargetRotation, float DeltaTime, bool bAllowLag) const
{
	if (!bAllowLag) return CameraTargetRotation;

	const float RotationLag = CameraAnimInst->GetCurveValue(FName("RotationLag"));
	return UGASPFunctionLibrary::DamperExactRotation(CameraRotation, CameraTargetRotation, DeltaTime, RotationLag);
}

FVector UGASPCamera::CalcPivotLagLocation(const FQuat& CameraYawRotation, float DeltaTime, bool bAllowLag) const
{
	if (!bAllowLag) return PivotTargetLocation;

	const FVector UnrotatedCurrentLoc = CameraYawRotation.UnrotateVector(PivotLagLocation);
	const FVector UnrotatedTargetLoc = CameraYawRotation.UnrotateVector(PivotTargetLocation);

	const float LocationLagX = CameraAnimInst->GetCurveValue(FName("LocationLagX"));
	const float LocationLagY = CameraAnimInst->GetCurveValue(FName("LocationLagY"));
	const float LocationLagZ = CameraAnimInst->GetCurveValue(FName("LocationLagZ"));

	return CameraYawRotation.RotateVector({
		UGASPFunctionLibrary::DamperExact(UnrotatedCurrentLoc.X, UnrotatedTargetLoc.X, DeltaTime, LocationLagX),
		UGASPFunctionLibrary::DamperExact(UnrotatedCurrentLoc.Y, UnrotatedTargetLoc.Y, DeltaTime, LocationLagY),
		UGASPFunctionLibrary::DamperExact(UnrotatedCurrentLoc.Z, UnrotatedTargetLoc.Z, DeltaTime, LocationLagZ)
	});
}

FVector UGASPCamera::CalcCollisionFixLocation(const FVector& CameraTargetLocation, const FVector& PivotOffset, float DeltaTime, bool bAllowLag, float& NewTraceDistanceRatio) const
{
	const float MeshScale = IGASPCameraInterface::Execute_GetOwnerMeshScale(OwnerPawn).Z;

	FVector TraceOffsetDir = UKismetMathLibrary::GetRightVector(CameraRotation);
	FVector TraceOffset = TraceOffsetDir * CameraSettings->TP_Settings.GetShoulderOffsetLength() * MeshScale;
	
	FVector TraceStart = FMath::Lerp(
		IGASPCameraInterface::Execute_GetOwnerMeshSocketLocation(OwnerPawn, FName("head")) + TraceOffset,
		PivotTargetLocation + PivotOffset + FVector{ CameraSettings->TP_Settings.TraceOverrideOffset },
		FMath::Clamp(CameraAnimInst->GetCurveValue(FName("TraceOverride")), 0.f, 1.f)
	);
	FVector TraceEnd = CameraTargetLocation;

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		TraceStart,
		TraceEnd,
		CameraSettings->TP_Settings.TraceRadius * MeshScale,
		UEngineTypes::ConvertToTraceType(CameraSettings->TP_Settings.TraceChannel),
		false,
		{ GetOwner() },
		CameraSettings->bDebugDrawCollision ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
		HitResult,
		true
	);

	FVector TraceResult = TraceEnd;
	if (bHit)
	{
		if (!HitResult.bStartPenetrating)
		{
			TraceResult = HitResult.Location;
		}
		else if (TryAdjustTraceStartLocation(TraceStart, MeshScale))
		{
			UKismetSystemLibrary::SphereTraceSingle(
				GetWorld(),
				TraceStart,
				TraceEnd,
				CameraSettings->TP_Settings.TraceRadius * MeshScale,
				UEngineTypes::ConvertToTraceType(CameraSettings->TP_Settings.TraceChannel),
				false,
				{ GetOwner() },
				CameraSettings->bDebugDrawCollision ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None,
				HitResult,
				true
			);
			if (HitResult.IsValidBlockingHit())
			{
				TraceResult = HitResult.Location;
			}
		}
		else
		{
			// TryAdjustTraceStartLocation()返回false之后TraceStart也可能被更改
			TraceResult = TraceStart;
		}
	}

	if (!bAllowLag || !CameraSettings->TP_Settings.bEnableTraceDistanceSmoothing)
	{
		NewTraceDistanceRatio = 1.f;
		return TraceResult;
	}

	const FVector TraceVector = TraceEnd - TraceStart;
	const float TraceDistance = TraceVector.Size();
	if (TraceDistance <= UE_KINDA_SMALL_NUMBER)
	{
		NewTraceDistanceRatio = 1.f;
		return TraceResult;
	}

	// 被阻挡时镜头直接拉近，避免遮挡视线，障碍消失时相机平滑拉远
	const float TargetTraceDistanceRatio = (TraceResult - TraceStart).Size() / TraceDistance;
	if (TargetTraceDistanceRatio <= TraceDistanceRatio)
	{
		NewTraceDistanceRatio = TargetTraceDistanceRatio;
	}
	else
	{
		NewTraceDistanceRatio = UGASPFunctionLibrary::DamperExact(
			TraceDistanceRatio,
			TargetTraceDistanceRatio,
			DeltaTime,
			CameraSettings->TP_Settings.TraceDistanceSmoothingHalfLife
		);
	}
	return TraceStart + TraceVector * TraceDistanceRatio;
}

bool UGASPCamera::TryAdjustTraceStartLocation(FVector& Location, float MeshScale) const
{
	const FCollisionShape CollisionShape = FCollisionShape::MakeSphere((CameraSettings->TP_Settings.TraceRadius + 1.f) * MeshScale);

	static TArray<FOverlapResult> A_Overlaps;
	check(A_Overlaps.IsEmpty());
	ON_SCOPE_EXIT{ A_Overlaps.Reset(); };  // 在函数退出时清空数组

	// Overlaps存储所有检测到的重叠结果，用于后续计算最小移动向量MTD
	bool bHit = GetWorld()->OverlapMultiByChannel(
		A_Overlaps,
		Location,
		FQuat::Identity,
		CameraSettings->TP_Settings.TraceChannel,
		CollisionShape,
		{ FName("Overlap Multi"), false, GetOwner() }
	);
	if (!bHit) return false;

	FVector Adjustment = FVector::ZeroVector;
	bool bAnyValidBlock = false;

	FMTDResult MTDResult;
	for (const auto& Overlap : A_Overlaps)
	{
		if (!Overlap.Component.IsValid() ||
			Overlap.Component->GetCollisionResponseToChannel(CameraSettings->TP_Settings.TraceChannel) != ECR_Block) continue;

		const FBodyInstance* OverlapBody = Overlap.Component->GetBodyInstance(NAME_None, true, Overlap.ItemIndex);
		if (!OverlapBody || !OverlapBody->OverlapTest(Location, FQuat::Identity, CollisionShape, &MTDResult)) return false;
		// 只要有一个Overlap不能正确给出MTD，整个逻辑就放弃

		if (!FMath::IsNearlyZero(MTDResult.Distance))
		{
			Adjustment += MTDResult.Direction * MTDResult.Distance;
			bAnyValidBlock = true;
		}
	}
	if (!bAnyValidBlock) return false;

	// Adjustment是积累出来的推开向量，多个重叠MTD的合力
	// 通过点积判断Adjustment方向，不允许将相机推离角色更远的地方
	FVector AdjustmentDir = Adjustment;
	if (!AdjustmentDir.Normalize() ||
		((GetOwner()->GetActorLocation() - Location).GetSafeNormal() | AdjustmentDir) < -UE_KINDA_SMALL_NUMBER) return false;

	Location += Adjustment;
	return !GetWorld()->OverlapBlockingTestByChannel(
		Location,
		FQuat::Identity,
		CameraSettings->TP_Settings.TraceChannel,
		FCollisionShape::MakeSphere(CameraSettings->TP_Settings.TraceRadius * MeshScale),
		{ FName("Free Space Overlap"), false, GetOwner() }
	);
}

/**
* Public APIs
*/
void UGASPCamera::GetViewInfo(FMinimalViewInfo& ViewInfo) const
{
	ViewInfo.Location = CameraLocation;
	ViewInfo.Rotation = CameraRotation;
	ViewInfo.FOV = CameraFieldOfView;

	ViewInfo.PostProcessBlendWeight = IsValid(CameraSettings) ? CameraSettings->PostProcessBlendWeight : 0.0f;
	if (ViewInfo.PostProcessBlendWeight > UE_SMALL_NUMBER)
	{
		ViewInfo.PostProcessSettings = CameraSettings->PostProcess;
	}
}

void UGASPCamera::SetViewMode(EGASPViewMode NewViewMode, bool bForce)
{
	if (bForce || ViewMode != NewViewMode)
	{
		ViewMode = NewViewMode;
		
		// TODO: 切到第一人称时，强制将角色移动组件的旋转模式切换为“朝向移动方向”

		if (OwnerPawn->GetRemoteRole() != ROLE_Authority)
		{
			Server_SetViewMode(NewViewMode);
		}

		OnViewModeChangedDelegate.Broadcast(NewViewMode);
	}
}

void UGASPCamera::Server_SetViewMode_Implementation(EGASPViewMode NewViewMode)
{
	ViewMode = NewViewMode;
}

void UGASPCamera::SetShoulderMode(EGASPShoulderMode NewShoulderMode, bool bForce)
{
	if (bForce || CameraSettings->TP_Settings.ShoulderMode != NewShoulderMode)
	{
		CameraSettings->TP_Settings.LastShoulderMode = CameraSettings->TP_Settings.ShoulderMode;
		CameraSettings->TP_Settings.ShoulderMode = NewShoulderMode;
		OnShoulderModeChangedDelegate.Broadcast(NewShoulderMode);
	}
}
