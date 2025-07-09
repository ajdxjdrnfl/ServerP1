// Fill out your copyright notice in the Description page of Project Settings.


#include "CubePawn.h"

// Sets default values
ACubePawn::ACubePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	RootComponent = BoxComponent;

	BoxComponent->SetBoxExtent(FVector(40.f, 40.f, 90.f));
	BoxComponent->SetSimulatePhysics(true);             
	BoxComponent->SetEnableGravity(true);                 
	BoxComponent->SetNotifyRigidBodyCollision(true);      
	BoxComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
	BoxComponent->BodyInstance.bLockXRotation = true;
	BoxComponent->BodyInstance.bLockYRotation = true;
	BoxComponent->BodyInstance.bLockZRotation = true;


	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 600.f;
}

// Called when the game starts or when spawned
void ACubePawn::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent->OnComponentHit.AddDynamic(this, &ACubePawn::OnHit);
}

// Called every frame
void ACubePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

// Called to bind functionality to input
void ACubePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	/*
	PlayerInputComponent->BindAxis("MoveForward", this, &ACubePawn::MoveVertical);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACubePawn::MoveHorizontal);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACubePawn::Jump);
	*/
}

void ACubePawn::MoveVertical(float Value)
{
	AddMovementInput(GetActorForwardVector(), Value);
}

void ACubePawn::MoveHorizontal(float Value)
{
	AddMovementInput(GetActorRightVector(), Value);
}

void ACubePawn::Jump()
{
	if (bCanJump)
	{
		BoxComponent->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);
		bCanJump = false;
	}
}

bool ACubePawn::CheckGrounded()
{
	FVector BoxOrigin = BoxComponent->GetComponentLocation();
	FVector BoxExtent = BoxComponent->GetScaledBoxExtent();

	FVector Start = BoxOrigin;
	FVector End = Start - FVector(0, 0, BoxExtent.Z + 1.0f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	return bHit;
}

void ACubePawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (Hit.Normal.Z > 0.7f)
	{
		bCanJump = true;
	}
}
