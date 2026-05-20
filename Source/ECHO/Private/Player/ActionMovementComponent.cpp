//ActionMovementComponent.cpp
//アクションムーブメントコンポーネントソース

#include "Player/ActionMovementComponent.h"

UActionMovementComponent::UActionMovementComponent() :
	MaxFallSpeed(4000.f),	//落下速度
	BaseWalkSpeed(600.f),	//通常スピード
	SprintSpeed(1200.f),	//スプリント速度
	m_bIsSprinting(false),		//スプリント状態かどうか
	AirControlFirstJump(0.3f),	  //一段目の空中制御
	AirControlSecondJump(0.15f),  //二段目の空中制御
	SecondJumpZVelocity(400.f),   //二段目のZ方向への追加初速
	LandingRecoveryTime(0.5f),    //着地硬直が治るまでの時間
	LandingSpeedMultiplier(0.3f), //着地硬直後速度を戻す倍率
	bIsLandingRecovery(false),
	LandingRecoveryElapsed(0.f),
	CachedMaxWalkSpeed(0.f),
	CachedMaxSprintSpeed(0.f)
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxAcceleration = 2048.f;				// 加速度
	GroundFriction = 8.f;					// 摩擦
	BrakingDecelerationWalking = 2048.f;	// ブレーキ力
	GravityScale = 3.f;						// 重力

	BrakingDecelerationFalling = 2048.f;	//空中ブレーキ力

	//初期の移動速度をBaseWalkSpeedに設定
	MaxWalkSpeed = BaseWalkSpeed;
}

void UActionMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//空中では体の向きを変えない
	if (IsFalling())
	{
		bOrientRotationToMovement = false;

		//回避中はスキップ
		if (!bIsDodging)
		{
			//入力方向に固定の力を加える
			FVector InputVector = GetLastInputVector();
			if (!InputVector.IsNearlyZero())
			{
				//入力方向のXY成分だけ取り出して正規化
				FVector InputDir = InputVector.GetSafeNormal2D();

				//固定の押し出し力を加える
				FVector AirPushVelocity = InputDir * AirPushForce;

				//現在の速度にブレンドして加える（急激に変わらないように）
				Velocity += AirPushVelocity * GetWorld()->GetDeltaSeconds();

				//XY方向の速度が最大を超えないようにクランプ
				FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
				if (HorizontalVelocity.Size() > AirMaxHorizontalSpeed)
				{
					HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * AirMaxHorizontalSpeed;
					Velocity.X = HorizontalVelocity.X;
					Velocity.Y = HorizontalVelocity.Y;
				}
			}
		}
	}
	else
	{
		bOrientRotationToMovement = true;
	}

	//着地硬直の回復処理
	if (bIsLandingRecovery)
	{

		LandingRecoveryElapsed += DeltaTime;
		float Alpha = FMath::Clamp(
			LandingRecoveryElapsed / LandingRecoveryTime, 0.f, 1.f);

		float EasedAlpha = FMath::InterpEaseIn(0.f, 1.f, Alpha, 2.f);

		MaxWalkSpeed = FMath::Lerp(
			CachedMaxWalkSpeed * LandingSpeedMultiplier,
			CachedMaxWalkSpeed,
			EasedAlpha);

		if (Alpha >= 1.f)
		{
			bIsLandingRecovery = false;
			MaxWalkSpeed = CachedMaxWalkSpeed;
		}
	}
}

void UActionMovementComponent::SetMovementWeight(float NewAcceleration, float NewFriction)
{
	MaxAcceleration = NewAcceleration;
	GroundFriction = NewFriction;
}

void UActionMovementComponent::SetSprinting(bool bIsSprinting)
{
	//引数がtrueならスプリント速度、falseなら通常速度をMaxWalkSpeedに代入
	MaxWalkSpeed = bIsSprinting ? SprintSpeed : BaseWalkSpeed;
	m_bIsSprinting = bIsSprinting;
}

//着地硬直を開始する関数
void UActionMovementComponent::StartLandingRecovery(bool bWasSprinting)
{
	//走り状態だったらスプリント速度を基準にする
	CachedMaxWalkSpeed = bWasSprinting ? SprintSpeed : BaseWalkSpeed;
	LandingRecoveryElapsed = 0.f;
	bIsLandingRecovery = true;
	MaxWalkSpeed = CachedMaxWalkSpeed * LandingSpeedMultiplier;
}
