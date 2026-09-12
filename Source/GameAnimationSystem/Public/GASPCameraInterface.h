// Copyright: Jichao Luo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GASPCameraInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGASPCameraInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GAMEANIMATIONSYSTEM_API IGASPCameraInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Camera")
	FVector GetFirstPersonCameraLocation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Camera")
	FVector GetThirdPersonPivotLocation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Camera")
	FQuat GetOwnerMeshQuat() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Camera")
	FVector GetOwnerMeshScale() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Camera")
	FVector GetOwnerMeshSocketLocation(FName SocketName) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GASP|Camera")
	void SetOwnerMeshNoSee(bool bNewOwnerNoSee);
};
