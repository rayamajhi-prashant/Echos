//CombatComponent.cpp
//コンバットコンポーネントソース

#include "Player/CombatComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Enemy/EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::ExecuteAttack()
{
    if (bIsAttacking)
    {
        if (bComboWindowOpen)
        {
            int32 NextIndex = CurrentComboIndex + 1;
            if (NextIndex < ComboSteps.Num())
                ExecuteComboStep(NextIndex);
        }
        else
        {
            bComboInputBuffered = true;
        }
        return;
    }

    ExecuteComboStep(0);
}

void UCombatComponent::ExecuteComboStep(int32 StepIndex)
{
    if (!ComboSteps.IsValidIndex(StepIndex)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    const FComboStepData& Step = ComboSteps[StepIndex];

    CurrentComboIndex = StepIndex;
    bIsAttacking = true;
    bComboWindowOpen = false;
    bComboInputBuffered = false;
    HitActorsThisAttack.Empty();

    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        ComboResetTimerHandle,
        this,
        &UCombatComponent::OnComboResetTimeout,
        Step.ComboResetTime,
        false
    );

    FGhostActionData Data;
    Data.Type = EGhostActionType::Attack;
    Data.Timestamp = GetWorld()->GetTimeSeconds();
    Data.Location = OwnerCharacter->GetActorLocation();
    Data.Rotation = OwnerCharacter->GetActorRotation();
    Data.AnimationTag = FName(*FString::Printf(TEXT("Attack%d"), StepIndex + 1));
    OnGhostActionRecorded.Broadcast(Data);

    if (Step.Montage)
        OwnerCharacter->PlayAnimMontage(Step.Montage);
}

void UCombatComponent::OpenComboWindow()
{
    bComboWindowOpen = true;

    if (bComboInputBuffered)
    {
        bComboInputBuffered = false;
        int32 NextIndex = CurrentComboIndex + 1;
        if (NextIndex < ComboSteps.Num())
            ExecuteComboStep(NextIndex);
    }
}

void UCombatComponent::CloseComboWindow()
{
    bComboWindowOpen = false;
}

void UCombatComponent::OnComboResetTimeout()
{
    CurrentComboIndex = 0;
    bIsAttacking = false;
    bComboWindowOpen = false;
    bComboInputBuffered = false;
    HitActorsThisAttack.Empty();
    GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
}

void UCombatComponent::CheckHit()
{
    if (!ComboSteps.IsValidIndex(CurrentComboIndex)) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    const FComboStepData& Step = ComboSteps[CurrentComboIndex];

    FVector Start = OwnerCharacter->GetActorLocation();
    FVector End = Start + OwnerCharacter->GetActorForwardVector() * Step.HitRange;

    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(Step.HitRadius);

    DrawDebugSphere(GetWorld(), Start, Step.HitRadius, 12, FColor::Yellow, false, 1.f);
    DrawDebugSphere(GetWorld(), End, Step.HitRadius, 12, FColor::Red, false, 1.f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);

    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults, Start, End,
        FQuat::Identity, ECC_Pawn, Sphere);

    if (!bHit) return;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || HitActor == OwnerCharacter) continue;
        if (HitActorsThisAttack.Contains(HitActor)) continue;
        HitActorsThisAttack.Add(HitActor);

        UGameplayStatics::ApplyDamage(
            HitActor, Step.Damage,
            OwnerCharacter->GetController(),
            OwnerCharacter,
            UDamageType::StaticClass());

        if (Step.LaunchForce > 0.f)
        {
            if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
            {
                FVector LaunchDir = (HitActor->GetActorLocation()
                    - OwnerCharacter->GetActorLocation()).GetSafeNormal();
                LaunchDir.Z = 0.5f;
                HitCharacter->LaunchCharacter(
                    LaunchDir * Step.LaunchForce, true, true);
            }
        }

        if (Cast<AEnemyChara>(HitActor))
            OnHitEnemy.Broadcast(Step.Damage * 0.5f);
    }
}

void UCombatComponent::ExecuteDodge()
{
    if (!bCanDodge) return;
    if (bIsAttacking) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

    FVector DodgeDirection = MoveComp->GetLastInputVector();
    if (DodgeDirection.IsNearlyZero())
        DodgeDirection = OwnerCharacter->GetActorForwardVector();

    FGhostActionData Data;
    Data.Type = EGhostActionType::Dodge;
    Data.Timestamp = GetWorld()->GetTimeSeconds();
    Data.Location = OwnerCharacter->GetActorLocation();
    Data.Rotation = OwnerCharacter->GetActorRotation();
    Data.Direction = DodgeDirection.GetSafeNormal();
    OnGhostActionRecorded.Broadcast(Data);

    if (MoveComp->IsFalling())
    {
        if (bHasAirDodged) return;
        bHasAirDodged = true;
    }

    bCanDodge = false;
    OwnerCharacter->GetWorldTimerManager().SetTimer(
        DodgeCoolDownTimerHandle, this,
        &UCombatComponent::ResetDodgeCooldown,
        DodgeCooldown, false);

    DodgeDirection.Z = 0.f;
    CachedGravityScale = MoveComp->GravityScale;
    CachedGroundFriction = MoveComp->GroundFriction;
    MoveComp->GravityScale = 0.f;
    MoveComp->GroundFriction = 0.f;

    OwnerCharacter->GetWorldTimerManager().SetTimer(
        DodgeTimerHandle, this,
        &UCombatComponent::EndDodge,
        DodgeDuration, false);

    UActionMovementComponent* ActionMoveComp =
        Cast<UActionMovementComponent>(MoveComp);
    if (ActionMoveComp) ActionMoveComp->bIsDodging = true;

    OwnerCharacter->LaunchCharacter(
        DodgeDirection.GetSafeNormal() * DodgeForce, true, true);
}

void UCombatComponent::EndDodge()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    if (!MoveComp) return;

    UActionMovementComponent* ActionMoveComp =
        Cast<UActionMovementComponent>(MoveComp);
    if (ActionMoveComp) ActionMoveComp->bIsDodging = false;

    MoveComp->GravityScale = CachedGravityScale;
    MoveComp->GroundFriction = CachedGroundFriction;

    FVector Vel = MoveComp->Velocity;
    Vel.X = 0.f;
    Vel.Y = 0.f;
    MoveComp->Velocity = Vel;
}

void UCombatComponent::ResetDodgeCooldown()
{
    bCanDodge = true;
}

void UCombatComponent::ResetAirDodge()
{
    bHasAirDodged = false;
}