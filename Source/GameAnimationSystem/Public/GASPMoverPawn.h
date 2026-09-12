// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"
#include "Types/GASPLocomotionTypes.h"
#include "GASPCameraInterface.h"
#include "GASPMoverPawn.generated.h"

/**
 * AGASPMoverPawn —— Mover 的「通用可运动对象」基类，只负责「管道」：
 *   Enhanced Input 绑定 / 输入缓存 / Request* API / OnProduceInput 钩子 / 视角转发。
 */
UCLASS()
class GAMEANIMATIONSYSTEM_API AGASPMoverPawn : 
	public APawn, 
	public IGASPCameraInterface,
	public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	AGASPMoverPawn(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UMoverComponent* MoverComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UGASPCamera* Camera;


	/**
	* IGASPCameraInterface
	*/
public:
	virtual FVector GetFirstPersonCameraLocation_Implementation() const override { return GetActorLocation(); }
	virtual FVector GetThirdPersonPivotLocation_Implementation() const override { return GetActorLocation(); };
	virtual FQuat GetOwnerMeshQuat_Implementation() const override { return  GetActorQuat(); }
	virtual FVector GetOwnerMeshScale_Implementation() const override { return GetActorScale(); }
	virtual FVector GetOwnerMeshSocketLocation_Implementation(FName SocketName) const override { return GetActorLocation(); }
	virtual void SetOwnerMeshNoSee_Implementation(bool bNewOwnerNoSee) override {}


	/**
	* IMoverInputProducerInterface
	*/
protected:
	// ProduceInput 的入口，不要直接重写，去扩展派生类 OnProduceInput 和 OnProduceInputInBlueprint
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	virtual void OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Produce Input", meta = (ScriptName = "OnProduceInput"))
	FMoverInputCmdContext OnProduceInputInBlueprint(float DeltaMs, FMoverInputCmdContext InputCmd);

private:
	bool bHasProduceInputInBpFunc = false;
	

	/**
	* Mover
	*/
public:
	virtual void ReceiveMoveInput(const FVector2D& MoveInput);

protected:
	/**
	 * 给 AI / 导航 / 过场用的「速度输入」（世界空间 cm/s）。
	 * 非零时优先于方向意图；用完自己清：RequestMoveByVelocity(FVector::ZeroVector)
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Locomotion")
	virtual void RequestMoveByVelocity(const FVector& DesiredVelocity);

protected:
	// ------------------------- 输入缓存 -------------------------
	// 「渲染帧 ↔ 模拟帧」之间的适配器。三类量的清零规则不同：
	//   持续量 → 只在 Completed（松开）清零
	//   速率量 → Tick 里用掉即清零
	//   边沿量 → ProduceInput_Implementation 末尾统一消费
	FVector CachedMoveInputIntent = FVector::ZeroVector;
	FVector CachedMoveInputVelocity = FVector::ZeroVector;

};
