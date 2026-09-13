// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "GASPPlayerController.generated.h"

/**
 * AGASPPlayerController —— 「输入采集」层：
 *   绑定按键、接收输入，再 Cast 到当前 possessed 的 Pawn，把「意图」传给它。
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API AGASPPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AGASPPlayerController();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

protected:
	UFUNCTION()
	void OnRep_MoverPawn();

private:
	UPROPERTY(ReplicatedUsing = OnRep_MoverPawn)
	class AGASPMoverCharacter* MoverCharacter;

	/** 俯仰角限制 */
	UPROPERTY(EditAnywhere, Category = "GASP|CameraManager")
	float ViewPitchMin = -60.f;

	UPROPERTY(EditAnywhere, Category = "GASP|CameraManager")
	float ViewPitchMax = 60.f;


	// ---------------------------------------------------------------------
	// Locomotion Inputs
	// ---------------------------------------------------------------------
protected:
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	void OnLookTriggered(const FInputActionValue& Value);
	void OnJumpStarted(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);

private:
	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Locomotion")
	bool bEnableLocomotionInputs = true;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Locomotion")
	class UInputMappingContext* IMC_Locomotion;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Locomotion")
	int32 LocomotionInputsPriority = 1;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Locomotion")
	UInputAction* IA_Move;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Locomotion")
	UInputAction* IA_Look;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Locomotion")
	UInputAction* IA_Jump;

public:
	FORCEINLINE UInputAction* GetMoveInputAction() const { return IA_Move; }


	// ---------------------------------------------------------------------
	// Combat Inputs
	// ---------------------------------------------------------------------
protected:
	void OnAimingStarted(const FInputActionValue& Value);
	void OnAimingReleased(const FInputActionValue& Value);

private:
	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Combat")
	bool bEnableCombatInputs = true;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Combat")
	class UInputMappingContext* IMC_Combat;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Combat")
	int32 CombatInputsPriority = 1;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Combat")
	UInputAction* IA_Aiming;


	// ---------------------------------------------------------------------
	// Debug Inputs
	// ---------------------------------------------------------------------
protected:
	void OnToggleViewMode(const FInputActionValue& Value);
	void OnToggleRotationMode(const FInputActionValue& Value);
private:
	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Debug")
	bool bEnableDebugInputs = true;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Debug")
	class UInputMappingContext* IMC_Debug;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Debug")
	int32 DebugInputsPriority = 0;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Debug")
	UInputAction* IA_ToggleViewMode;

	UPROPERTY(EditAnywhere, Category = "GASP|Inputs|Debug")
	UInputAction* IA_ToggleRotationMode;

};
