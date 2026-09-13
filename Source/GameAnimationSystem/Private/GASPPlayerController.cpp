// Copyright: Jichao Luo


#include "GASPPlayerController.h"
#include "Mover/GASPMoverCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Net/UnrealNetwork.h"

#define CHECK_MOVER \
if (!MoverCharacter) \
{ \
	MoverCharacter = Cast<AGASPMoverCharacter>(GetPawn()); \
	if (!MoverCharacter) return; \
};

AGASPPlayerController::AGASPPlayerController()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;

	{
		static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_LocomotionAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Locomotion/IMC_GASP_Locomotion.IMC_GASP_Locomotion")
		);
		if (IMC_LocomotionAsset.Succeeded()) { IMC_Locomotion = IMC_LocomotionAsset.Object; }

		static ConstructorHelpers::FObjectFinder<UInputAction> IA_MoveAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Locomotion/IA_GASP_Move.IA_GASP_Move")
		);
		if (IA_MoveAsset.Succeeded()) { IA_Move = IA_MoveAsset.Object; }

		static ConstructorHelpers::FObjectFinder<UInputAction> IA_LookAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Locomotion/IA_GASP_Look.IA_GASP_Look")
		);
		if (IA_LookAsset.Succeeded()) { IA_Look = IA_LookAsset.Object; }

		static ConstructorHelpers::FObjectFinder<UInputAction> IA_JumpAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Locomotion/IA_GASP_Jump.IA_GASP_Jump")
		);
		if (IA_JumpAsset.Succeeded()) { IA_Jump = IA_JumpAsset.Object; }
	}

	{
		static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_CombatAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Combat/IMC_GASP_Combat.IMC_GASP_Combat")
		);
		if (IMC_CombatAsset.Succeeded()) { IMC_Combat = IMC_CombatAsset.Object; }

		static ConstructorHelpers::FObjectFinder<UInputAction> IA_AimingAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Combat/IA_GASP_Aiming.IA_GASP_Aiming")
		);
		if (IA_AimingAsset.Succeeded()) { IA_Aiming = IA_AimingAsset.Object; }
	}

	{
		static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_DebugAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Debug/IMC_GASP_Debug.IMC_GASP_Debug")
		);
		if (IMC_DebugAsset.Succeeded()) { IMC_Debug = IMC_DebugAsset.Object; }

		static ConstructorHelpers::FObjectFinder<UInputAction> IA_ViewModeAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Debug/IA_GASP_ToggleViewMode.IA_GASP_ToggleViewMode")
		);
		if (IA_ViewModeAsset.Succeeded()) { IA_ToggleViewMode = IA_ViewModeAsset.Object; }

		static ConstructorHelpers::FObjectFinder<UInputAction> IA_RotationModeAsset(
			TEXT("/GameAnimationSystem/Player/Inputs/Debug/IA_GASP_ToggleRotationMode.IA_GASP_ToggleRotationMode")
		);
		if (IA_RotationModeAsset.Succeeded()) { IA_ToggleRotationMode = IA_RotationModeAsset.Object; }
	}
	
}

void AGASPPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AGASPPlayerController, MoverCharacter, COND_OwnerOnly);
}

void AGASPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (auto EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (auto EIS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (bEnableLocomotionInputs)
			{
				EIS->AddMappingContext(IMC_Locomotion, LocomotionInputsPriority);
				EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ThisClass::OnMoveTriggered);
				EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ThisClass::OnMoveCompleted);
				EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ThisClass::OnLookTriggered);
				EIC->BindAction(IA_Jump, ETriggerEvent::Started,   this, &ThisClass::OnJumpStarted);
				EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ThisClass::OnJumpReleased);
			}
			if (bEnableCombatInputs)
			{
				EIS->AddMappingContext(IMC_Combat, CombatInputsPriority);
				EIC->BindAction(IA_Aiming, ETriggerEvent::Started, this, &ThisClass::OnAimingStarted);
				EIC->BindAction(IA_Aiming, ETriggerEvent::Completed, this, &ThisClass::OnAimingReleased);
			}
			if (bEnableDebugInputs)
			{
				EIS->AddMappingContext(IMC_Debug, DebugInputsPriority);
				EIC->BindAction(IA_ToggleViewMode, ETriggerEvent::Completed, this, &ThisClass::OnToggleViewMode);
				EIC->BindAction(IA_ToggleRotationMode, ETriggerEvent::Completed, this, &ThisClass::OnToggleRotationMode);
			}
		}
	}
}

void AGASPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = ViewPitchMax;
	}
}

void AGASPPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AGASPPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	MoverCharacter = Cast<AGASPMoverCharacter>(InPawn);
	if (MoverCharacter)
	{
		// 仅在 MoverCharacter 存在时，设置其相关组件
	}
}

void AGASPPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	MoverCharacter = nullptr;
}

void AGASPPlayerController::OnRep_MoverPawn()
{
	// 通过网络同步设置 MoverPawn 相关组件
}

void AGASPPlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	CHECK_MOVER
	MoverCharacter->ReceiveMoveInput(Value.Get<FVector2D>());
}

void AGASPPlayerController::OnMoveCompleted(const FInputActionValue& Value)
{
	// 持续量：只在「松开」这一刻清零。
	// 千万不要改到 OnProduceInput 里去清 —— 固定 60Hz 模拟跑在 30fps 上时一帧有 2 个模拟步，
	// 清早了会漏掉一半输入（症状：低帧率下走路一顿一顿）
	CHECK_MOVER
	MoverCharacter->ReceiveMoveInput(FVector2D::ZeroVector);
}

void AGASPPlayerController::OnLookTriggered(const FInputActionValue& Value)
{
	FVector2D InputValue = Value.Get<FVector2D>();
	AddYawInput(InputValue.X);
	AddPitchInput(InputValue.Y);
}

void AGASPPlayerController::OnJumpStarted(const FInputActionValue& Value)
{
	CHECK_MOVER
	MoverCharacter->ReceiveJumpStarted();
}

void AGASPPlayerController::OnJumpReleased(const FInputActionValue& Value)
{
	CHECK_MOVER
	MoverCharacter->ReceiveJumpReleased();
}

void AGASPPlayerController::OnAimingStarted(const FInputActionValue& Value)
{
}

void AGASPPlayerController::OnAimingReleased(const FInputActionValue& Value)
{
}

void AGASPPlayerController::OnToggleViewMode(const FInputActionValue& Value)
{

}

void AGASPPlayerController::OnToggleRotationMode(const FInputActionValue& Value)
{

}
