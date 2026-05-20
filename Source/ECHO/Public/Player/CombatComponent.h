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

//コンボ一段分のデータ
USTRUCT(BlueprintType)
struct FComboStepData
{
	GENERATED_BODY()

	//再生するモンタージュ
	UPROPERTY(EditAnywhere)
	UAnimMontage* Montage = nullptr;

	//ヒット判定の範囲
	UPROPERTY(EditAnywhere)
	float HitRadius = 60.f;

	//ヒット範囲の距離
	UPROPERTY(EditAnywhere)
	float HitRange = 100.f;

	//ダメージ量
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	//吹き飛ばし力（0なら吹き飛ばしなし）
	UPROPERTY(EditAnywhere)
	float LaunchForce = 0.f;

	//次のコンボ入力受開始タイミング
	UPROPERTY(EditAnywhere)
	float ComboWindowStart = 0.5f;

	//コンボリセットまでの時間
	UPROPERTY(EditAnywhere)
	float ComboResetTime = 1.f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	//攻撃を実行する関数
	void ExecuteAttack();

	//回避を実行する関数
	void ExecuteDodge();

	//着地したときに呼び出すリセット関数
	void ResetAirDodge();

	//AnimNotifyから呼ぶ
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHit();

	//コンボ窓口を開く
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OpenComboWindow();

	//コンボ窓口を閉じる
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CloseComboWindow();

	FOnGhostAction OnGhostActionRecorded;
	FOnHitEnemyDelegate OnHitEnemy;

protected:
	//コンボデータ配列（BPで4つ設定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TArray<FComboStepData> ComboSteps;

	//回避の強さ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float DodgeForce;

	//回避の持続時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float DodgeDuration;

	//回避のリキャスト時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float DodgeCooldown;

private:
	//コンボの状態
	int32 CurrentComboIndex;
	bool bIsAttacking;
	bool bComboWindowOpen;
	bool bComboInputBuffered; //窓口が開く前に入力があったか

	//タイマーハンドル
	FTimerHandle ComboResetTimerHandle;
	FTimerHandle DodgeTimerHandle;
	FTimerHandle DodgeCoolDownTimerHandle;


	//回避可能かどうかのフラグ
	bool bCanDodge;
	//空中で回避したかどうかを記録するフラグ
	bool bHasAirDodged;

	//元の値を記憶しておくための変数
	float CachedGravityScale;		//重力
	float CachedGroundFriction;		//摩擦

	//1回の攻撃で複数回ヒットしないようにするフラグ
	TArray<AActor*> HitActorsThisAttack;

	//コンボのステップを実行する関数
	void ExecuteComboStep(int32 StepIndex);

	//コンボをリセットする関数
	void OnComboResetTimeout();

	//重力を元に戻す関数
	void EndDodge();

	//回避を再び可能にする関数
	void ResetDodgeCooldown();



	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	//UAnimMontage* AttackMontage;

	////ヒット判定の範囲
	//UPROPERTY(EditAnywhere, Category = "Combat")
	//float AttackRadius;

	////ヒット判定の距離
	//UPROPERTY(EditAnywhere, Category = "Combat")
	//float AttackRange;

	////1ヒットあたりのダメージ
	//UPROPERTY(EditAnywhere, Category = "Combat")
	//float AttackDamage;

	//1ヒットあたりのエネルギー増加量
	UPROPERTY(EditAnywhere, Category = "Combat")
	float EnergyGainPerHit;
};
