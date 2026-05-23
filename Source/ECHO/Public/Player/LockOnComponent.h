//LockOnComponent.h
//ロックオンコンポーネントヘッダー
//
//ターゲット管理・トグル処理・自動解除

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnChanged, AActor*);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULockOnComponent();

	//トグル
	void ToggleLockOn();

	//現在のターゲットを取得
	AActor* GetTarget() const { return CurrentTarget.Get(); }

	//ロックオン中かどうか
	bool IsLockedOn() const { return CurrentTarget.IsValid(); }

	//ターゲット変更通知デリゲート
	FOnLockOnChanged OnLockOnChanged;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:	
	//現在のターゲット
	TWeakObjectPtr<AActor> CurrentTarget;

	//ロックオン可能な最大距離
	float LockOnRange = 1500.f;

	//ロックオン自動解除距離
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float LockOnBreakRange = 2000.f;

	//画面中央に最も近い敵を探す
	AActor* FindBestTarget() const;

	//ロックオンを解除する
	void ClearLockOn();

	//ターゲットが有効かチェック
	bool IsTargetValid(AActor* Target) const;
};
