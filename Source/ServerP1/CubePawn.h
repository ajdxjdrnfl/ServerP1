// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/BoxComponent.h"
#include "ServerP1.h"
#include "CubePawn.generated.h"

UCLASS()
class SERVERP1_API ACubePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACubePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	void MoveVertical(float Value);
	void MoveHorizontal(float Value);

	void Jump();
	bool CheckGrounded();

public:
	void DoSyncState();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UBoxComponent* BoxComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UFloatingPawnMovement* MovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* MeshComponent;

private:
	bool bCanJump = false;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bReplicated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float JumpImpulse = 1000.f;
public:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
public:
	void InputVertical(float Value);
	void InputHorizontal(float Value);
	void InputJump();

	FMyInput GetCurInput();
	void ClearInput();
	FMyInput curInput;
};
