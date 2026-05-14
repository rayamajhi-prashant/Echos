//CombatComponent.cpp
//コンバットコンポーネントソース

#include "Player/CombatComponent.h"
#include "Enemy/KariEnemy/KariEnemyChar.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "DrawDebugHelpers.h"//画面デバッグ用
//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("A"));

UCombatComponent::UCombatComponent() :
	bIsAttaking(false),
	DodgeForce(6000.f),
	bHasAirDodged(false),
	DodgeDuration(0.1f),
	DodgeCooldown(0.5f),
	bCanDodge(true)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::CheckHit()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	//キャラクターの前方にSphereTrace
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start + OwnerCharacter->GetActorForwardVector() * AttackRange;

	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius);

	DrawDebugSphere(GetWorld(), Start, AttackRadius, 12, FColor::Yellow, false, 1.f);
	DrawDebugSphere(GetWorld(), End, AttackRadius, 12, FColor::Red, false, 1.f);
	DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);

	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere
	);

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
		FString::Printf(TEXT("CheckHit呼ばれた: ヒット数=%d"), HitResults.Num()));

	if (!bHit) return;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
			FString::Printf(TEXT("ヒットしたActor: %s"), *GetNameSafe(HitActor)));

		if (!HitActor || HitActor == OwnerCharacter) continue;

		//同じ攻撃で2回当たらないようにする
		if (HitActorsThisAttack.Contains(HitActor)) continue;
		HitActorsThisAttack.Add(HitActor);

		//ダメージを与える
		UGameplayStatics::ApplyDamage(
			HitActor,
			AttackDamage,
			OwnerCharacter->GetController(),
			OwnerCharacter,
			UDamageType::StaticClass()
		);

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
			FString::Printf(TEXT("EnemyキャストできたかL %s"), Cast<AEnemyChar>(HitActor) ? TEXT("OK") : TEXT("NO")));

		//エネルギー加算を通知
		if (Cast<AEnemyChar>(HitActor))
		{
			OnHitEnemy.Broadcast(EnergyGainPerHit);
		}
	}
}

void UCombatComponent::ExecuteAttack()
{
	//既に攻撃中なら何もしない
	if (bIsAttaking) return;
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	HitActorsThisAttack.Empty();

	FGhostActionData Data;
	Data.Type	   = EGhostActionType::Attack;
	Data.Timestamp = GetWorld()->GetTimeSeconds();
	Data.Location  = OwnerCharacter->GetActorLocation();
	Data.Rotation  = OwnerCharacter->GetActorRotation();
	Data.AnimationTag = FName("Attack");
	OnGhostActionRecorded.Broadcast(Data);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ExAtack"));


	//モンタージュがセットされていれば再生
	if (AttackMontage)
	{
		//bIsAttaking = true;
		OwnerCharacter->PlayAnimMontage(AttackMontage);
	}
}

void UCombatComponent::ExecuteDodge()
{
	//リキャスト中なら何もしない
	if (!bCanDodge) return;

	if (bIsAttaking) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	if (!MoveComp) return;

	//直前に入力をしていた移動方向を取得
	FVector DodgeDirection = MoveComp->GetLastInputVector();

	//もし入力をしていなかったらキャラクターの前の方向にする
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = OwnerCharacter->GetActorForwardVector();
	}

	FGhostActionData Data;
	Data.Type      = EGhostActionType::Dodge;
	Data.Timestamp = GetWorld()->GetTimeSeconds();
	Data.Location  = OwnerCharacter->GetActorLocation();
	Data.Rotation = OwnerCharacter->GetActorRotation();
	Data.Direction = DodgeDirection.GetSafeNormal();
	OnGhostActionRecorded.Broadcast(Data);

	//回数制限のロジック
	if (MoveComp->IsFalling())
	{
		if (bHasAirDodged) return;
		bHasAirDodged = true;
	}

	//回避フラグを折ってタイマー開始
	bCanDodge = false;
	OwnerCharacter->GetWorldTimerManager().SetTimer(
		DodgeCoolDownTimerHandle,
		this,
		&UCombatComponent::ResetDodgeCooldown,
		DodgeCooldown,
		false
	);

	//Z軸を無視して完全に水平方向のベクトルにする
	DodgeDirection.Z = 0.f;

	//重力と摩擦を0にする直前に、現在の設定を記録しておく
	CachedGravityScale = MoveComp->GravityScale;
	CachedGroundFriction = MoveComp->GroundFriction;

	MoveComp->GravityScale = 0.0f;
	MoveComp->GroundFriction = 0.0f;

	OwnerCharacter->GetWorldTimerManager().SetTimer(
		DodgeTimerHandle,
		this,
		&UCombatComponent::EndDodge,
		DodgeDuration,
		false
	);

	//方向ベクトルを正規化して、回避の力を掛ける
	FVector LaunchVelocity = DodgeDirection.GetSafeNormal() * DodgeForce;

	//キャラクターを弾き飛ばす
	OwnerCharacter->LaunchCharacter(LaunchVelocity, true, true);
}

//重力を戻す関数
void UCombatComponent::EndDodge()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();

		//記録しておいた元の重力と摩擦に戻す
		MoveComp->GravityScale = CachedGravityScale;
		MoveComp->GroundFriction = CachedGroundFriction;

		//横方向の勢いを殺す（Z軸の落下速度はそのまま残す）
		FVector CurrentVelocity = MoveComp->Velocity;

		// XとYの速度を0にする（完全に真下に落ちる）
		// ※もし「ほんの少しだけ慣性を残したい」場合は = 0.0f ではなく *= 0.1f などにしてください
		CurrentVelocity.X = 0.0f;
		CurrentVelocity.Y = 0.0f;

		MoveComp->Velocity = CurrentVelocity;
	}
}

//回避フラグを可能に戻す関数
void UCombatComponent::ResetDodgeCooldown()
{
	bCanDodge = true;
}

//着地時に呼ばれるリセット処理
void UCombatComponent::ResetAirDodge()
{
	bHasAirDodged = false;
}
