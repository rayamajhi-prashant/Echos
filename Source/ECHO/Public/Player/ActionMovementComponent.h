//ActionMovementComponent.h
//アクションムーブメントコンポーネントヘッダー
//
//移動・物理の拡張コンポーネント

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ActionMovementComponent.generated.h"

UCLASS()
class ECHO_API UActionMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UActionMovementComponent();

	//移動パラメータを動的に変更すための関数例
	void SetMovementWeight(float NewAcceleration, float NewFriction);

	//速度を切り替える
	void SetSprinting(bool bIsSprinting);

	//スプリント状態かどうか
	bool IsSprinting() { return m_bIsSprinting; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunnction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
	float SprintSpeed;

	bool m_bIsSprinting;

public:
	//空中で入力方向に加える固定の力
	UPROPERTY(EditAnywhere, Category = "Jump")
	float AirPushForce = 1500.f;

	//空中の横方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float AirMaxHorizontalSpeed = 800.f;
	
	//回避中フラグ
	bool bIsDodging = false;

	//一段目ジャンプの空中横移動係数
	UPROPERTY(EditAnywhere, Category = "Jump")
	float AirControlFirstJump;

	//二段目ジャンプの空中横移動係数
	UPROPERTY(EditAnywhere, Category = "Jump")
	float AirControlSecondJump;

	//二段目ジャンプの追加Z初速
	UPROPERTY(EditAnywhere, Category = "Jump")
	float SecondJumpZVelocity;


	//着地硬直を開始する関数
	void StartLandingRecovery(bool bWasSprinting);

	//着地後の速度回復時間
	UPROPERTY(EditAnywhere, Category = "Landing")
	float LandingRecoveryTime;

	//着地硬直中の速度倍率
	UPROPERTY(EditAnywhere, Category = "Landing")
	float LandingSpeedMultiplier;

private:
	float MaxFallSpeed;		//落下速度
	bool bIsLandingRecovery;
	float LandingRecoveryElapsed;
	float CachedMaxWalkSpeed;
	float CachedMaxSprintSpeed;
};
