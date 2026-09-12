// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "GASPMoverPawn.h"
#include "GASPMoverCharacter.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API AGASPMoverCharacter : public AGASPMoverPawn
{
	GENERATED_BODY()

public:
	AGASPMoverCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* Mesh;


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
	* IMoverInputProducerInterface
	*/
protected:
	virtual void OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult);


	/**
	* Mover
	*/
public:
	virtual void ReceiveJumpStarted();
	virtual void ReceiveJumpReleased();

private:
	// 跳跃缓存：边沿量 + 电平量，清零点不同
	bool bIsJumpJustPressed = false;
	bool bIsJumpPressed = false;

	/** 上一次「有效」的朝向意图 */
	FVector LastAffirmativeOrientationIntent = FVector::ZeroVector;

	/** 没有移动输入时保持上一次朝向（快速拨杆后角色会把转身动作做完） */
	UPROPERTY(EditAnywhere, Category = "GASP|Locomotion")
	bool bMaintainLastInputOrientation = false;

};
