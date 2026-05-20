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

	FOnGhostAction OnGhostActionRecorded;

protected:
	//攻撃状態かどうか
	bool bIsAttaking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	UAnimMontage* AttackMontage;

	//回避の強さ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float DodgeForce;

	//空中で回避したかどうかを記録するフラグ
	bool bHasAirDodged;

	//回避の持続時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float DodgeDuration;

	//回避のリキャスト時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float DodgeCooldown;

	//回避可能かどうかのフラグ
	bool bCanDodge;

	//タイマーハンドル
	FTimerHandle DodgeTimerHandle;
	FTimerHandle DodgeCoolDownTimerHandle;
	
	//回避を再び可能にする関数
	void ResetDodgeCooldown();

	//重力を元に戻す関数
	void EndDodge();

	//元の値を記憶しておくための変数
	float CachedGravityScale;		//重力
	float CachedGroundFriction;		//摩擦



	//------仮攻撃判定等------
public:
	//AnimNotifyから呼ぶヒット判定関数
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHit();

	//エネルギー加算のデリゲート
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHitEnemy, float);
	FOnHitEnemy OnHitEnemy;

private:
	//1回の攻撃で複数回ヒットしないようにするフラグ
	TArray<AActor*> HitActorsThisAttack;

	//ヒット判定の範囲
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRadius = 80.f;

	//ヒット判定の距離
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 120.f;

	//1ヒットあたりのダメージ
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 20.f;

	//1ヒットあたりのエネルギー増加量
	UPROPERTY(EditAnywhere, Category = "Combat")
	float EnergyGainPerHit = 15.f;


};
