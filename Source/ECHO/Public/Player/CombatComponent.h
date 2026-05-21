//CombatComponent.h
//コンバットコンポーネントヘッダー
//
//戦闘統括コンポーネント
//コンボの状態管理、現在装備中の武器
//回避、アクショントリガー、無敵時間

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/GhostType.h"
#include "CombatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGhostAction, const FGhostActionData&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHitEnemyDelegate, float);

USTRUCT(BlueprintType)
struct FComboStepData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    UAnimMontage* Montage = nullptr;

    UPROPERTY(EditAnywhere)
    float HitRadius = 60.f;

    UPROPERTY(EditAnywhere)
    float HitRange = 100.f;

    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

    // 吹き飛ばし力（0なら吹き飛ばしなし）
    UPROPERTY(EditAnywhere)
    float LaunchForce = 0.f;

    // コンボ入力受付開始タイミング（0?1）
    UPROPERTY(EditAnywhere)
    float ComboWindowStart = 0.5f;

    // コンボリセットまでの時間
    UPROPERTY(EditAnywhere)
    float ComboResetTime = 1.0f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    void ExecuteAttack();
    void ExecuteDodge();
    void ResetAirDodge();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CheckHit();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OpenComboWindow();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CloseComboWindow();

    FOnGhostAction OnGhostActionRecorded;
    FOnHitEnemyDelegate OnHitEnemy;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
    TArray<FComboStepData> ComboSteps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeForce = 6000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeDuration = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
    float DodgeCooldown = 0.5f;

private:
    int32 CurrentComboIndex = 0;
    bool bIsAttacking = false;
    bool bComboWindowOpen = false;
    bool bComboInputBuffered = false;

    FTimerHandle ComboResetTimerHandle;
    FTimerHandle DodgeTimerHandle;
    FTimerHandle DodgeCoolDownTimerHandle;

    bool bCanDodge = true;
    bool bHasAirDodged = false;
    float CachedGravityScale = 1.f;
    float CachedGroundFriction = 8.f;

    TArray<AActor*> HitActorsThisAttack;

    void ExecuteComboStep(int32 StepIndex);
    void OnComboResetTimeout();
    void EndDodge();
    void ResetDodgeCooldown();
};