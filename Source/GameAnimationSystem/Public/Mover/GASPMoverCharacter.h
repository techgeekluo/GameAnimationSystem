// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"
#include "MoverDataModelTypes.h"
#include "Camera/GASPCameraInterface.h"
#include "GASPMoverInputs.h"
#include "GASPMoverCharacterInterface.h"
#include "GASPMoverCharacter.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API AGASPMoverCharacter : 
	public APawn, 
	public IGASPCameraInterface,
	public IGASPMoverCharacterInterface,
	public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AGASPMoverCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* Capsule;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UGASPCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UGASPMoverComponent* Mover;

public:
	FORCEINLINE USkeletalMeshComponent* GetMesh() const { return Mesh; }
	FORCEINLINE UGASPCameraComponent* GetCameraComponent() const { return Camera; }
	FORCEINLINE UGASPMoverComponent* GetMoverComponent() const { return Mover; }

	/**
	* IGASPCameraInterface
	*/
public:
	virtual FVector GetFirstPersonCameraLocation_Implementation() const override;
	virtual FVector GetThirdPersonPivotLocation_Implementation() const override;
	virtual FQuat GetOwnerMeshQuat_Implementation() const override;
	virtual FVector GetOwnerMeshScale_Implementation() const override;
	virtual FVector GetOwnerMeshSocketLocation_Implementation(FName SocketName) const override;
	virtual void SetOwnerMeshNoSee_Implementation(bool bNewOwnerNoSee) override;


	/**
	* IGASPMoverInterface
	*/
public:
	virtual void GetAnimationProperties_Implementation(
		FGASPEssentialStates& EssentialStates,
		FGASPEssentialValues& EssentialValues
	) const override;


	/**
	* IMoverInputProducerInterface
	*/
protected:
	// ProduceInput 的入口，不要直接重写，去扩展派生类 OnProduceInput 和 OnProduceInputInBlueprint
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	virtual void OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Produce Input", meta = (ScriptName = "OnProduceInput"))
	FMoverInputCmdContext OnProduceInputInBlueprint(float DeltaMs, FMoverInputCmdContext InputCmd);

protected:
	FVector2D GetMoveInput2D() const;
	FVector GetMoveInputIntent() const;
	FRotator GetAimingRotation() const;

private:
	bool bHasProduceInputInBpFunc = false;


	/**
	* Mover
	*/
public:
	virtual void ReceiveMoveInput(const FVector2D& MoveInput);
	virtual void ReceiveJumpStarted();
	virtual void ReceiveJumpReleased();

private:
	// ------------------------- 输入缓存 -------------------------
	// 「渲染帧 ? 模拟帧」之间的适配器。三类量的清零规则不同：
	//   持续量 → 只在 Completed（松开）清零
	//   速率量 → Tick 里用掉即清零
	//   边沿量 → ProduceInput_Implementation 末尾统一消费
	FVector CachedMoveInputIntent = FVector::ZeroVector;
	FVector CachedMoveInputVelocity = FVector::ZeroVector;
	bool bIsJumpJustPressed = false;
	bool bIsJumpPressed = false;

	/** 上一次「有效」的朝向意图 */
	FVector LastAffirmativeOrientationIntent = FVector::ZeroVector;

	/** 没有移动输入时保持上一次朝向（快速拨杆后角色会把转身动作做完） */
	UPROPERTY(EditAnywhere, Category = "GASP|Locomotion")
	bool bMaintainLastInputOrientation = false;

protected:
	// 用于在「模拟帧」获取实时的输入值
	class UEnhancedPlayerInput* EnhancedInput;
	class UInputAction* IA_Move;

	FCharacterDefaultInputs MoverCharacterInputs_PostSim;
	FGASPMoverInputs MoverCustomInputs_PostSim;

	FVector FloorNormal;
	FVector FloorLocation;

	float ControlRotationRate;
	FRotator LastControlRotation;

	bool bTwinStickMode = false;
	FRotator TwinStickAimRotation;
	
	float SmoothedAnalogInputAmount = 1.f;

	EGASPMovementMode MovementMode = EGASPMovementMode::OnGround;




	/**
	* Combat
	*/
private:
	UPROPERTY()
	AActor* TargetedActor;

	TArray<AActor*> TargetableActors;
};
