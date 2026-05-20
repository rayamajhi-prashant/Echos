//CombatComponent.cpp
//コンバットコンポーネントソース

#include "Player/CombatComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Enemy/EnemyChara.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "DrawDebugHelpers.h"//画面デバッグ用
//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("A"));

UCombatComponent::UCombatComponent() :
	DodgeForce(5000.f),
	DodgeDuration(0.1f),
	DodgeCooldown(0.5f),
	bIsAttacking(false),
	bComboWindowOpen(false),
	bComboInputBuffered(false),
	bCanDodge(true),
	bHasAirDodged(false),
	CachedGravityScale(1.f),
	CachedGroundFriction(8.f),
	EnergyGainPerHit(15.f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::ExecuteAttack()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White,
		FString::Printf(TEXT("ExecuteAttack: bIsAttacking=%s, bComboWindowOpen=%s, ComboIndex=%d"),
			bIsAttacking ? TEXT("true") : TEXT("false"),
			bComboWindowOpen ? TEXT("true") : TEXT("false"),
			CurrentComboIndex));

	//既に攻撃中なら何もしない
	if (bIsAttacking)
	{
		if (bComboWindowOpen)
		{
			//窓口が開いてるので即座にコンボへ
			int32 NextIndex = CurrentComboIndex + 1;
			if (NextIndex < ComboSteps.Num())
			{
				ExecuteComboStep(NextIndex);
			}
		}
		else
		{
			//窓口が開いていなければバッファに積んで待つ
			bComboInputBuffered = true;

			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("Tunda"));
		}
		return;
	}

	//最初の一段目
	ExecuteComboStep(0);
	
	//ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	//if (!OwnerCharacter) return;
	//
	//HitActorsThisAttack.Empty();
	//
	//FGhostActionData Data;
	//Data.Type	   = EGhostActionType::Attack;
	//Data.Timestamp = GetWorld()->GetTimeSeconds();
	//Data.Location  = OwnerCharacter->GetActorLocation();
	//Data.Rotation  = OwnerCharacter->GetActorRotation();
	//Data.AnimationTag = FName("Attack");
	//OnGhostActionRecorded.Broadcast(Data);
	//
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ExAtack"));
	//
	//
	////モンタージュがセットされていれば再生
	//if (AttackMontage)
	//{
	//	//bIsAttaking = true;
	//	OwnerCharacter->PlayAnimMontage(AttackMontage);
	//}
}

void UCombatComponent::ExecuteComboStep(int32 StepIndex)
{
	if (!ComboSteps.IsValidIndex(StepIndex)) return;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
		FString::Printf(TEXT("コンボ %d段目 実行"), StepIndex + 1));

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	const FComboStepData& Step = ComboSteps[StepIndex];

	//状態更新
	CurrentComboIndex = StepIndex;
	bIsAttacking = true;
	bComboWindowOpen = false;
	bComboInputBuffered = false;
	HitActorsThisAttack.Empty();

	//コンボリセットタイマーをリセット
	GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ComboResetTimerHandle,
		this,
		&UCombatComponent::OnComboResetTimeout,
		Step.ComboResetTime,
		false
	);

	//GhostActionに記録
	FGhostActionData Data;
	Data.Type = EGhostActionType::Attack;
	Data.Timestamp = GetWorld()->GetTimeSeconds();
	Data.Location = OwnerCharacter->GetActorLocation();
	Data.Rotation = OwnerCharacter->GetActorRotation();
	Data.AnimationTag = FName(*FString::Printf(TEXT("Attack%d"), StepIndex + 1));
	OnGhostActionRecorded.Broadcast(Data);

	//モンタージュ再生
	if (Step.Montage)
	{
		OwnerCharacter->PlayAnimMontage(Step.Montage);
	}
}


void UCombatComponent::OpenComboWindow()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
		TEXT("コンボ窓口オープン"));

	bComboWindowOpen = true;

	//バッファに入力が積まれていれば即次のコンボへ
	if (bComboInputBuffered)
	{
		bComboInputBuffered = false;
		int32 NextIndex = CurrentComboIndex + 1;
		if (NextIndex < ComboSteps.Num())
		{
			ExecuteComboStep(NextIndex);
		}
	}
}


void UCombatComponent::CloseComboWindow()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Purple,
		FString::Printf(TEXT("CloseComboWindow: Index=%d"), CurrentComboIndex));

	bComboWindowOpen = false;

	// 4段目が終わったらリセット
	//if (CurrentComboIndex >= ComboSteps.Num() - 1)
	//{
	//	OnComboResetTimeout();
	//}
}

void UCombatComponent::OnComboResetTimeout()
{
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
		TEXT("コンボリセット"));

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

	//キャラクターの前方にSphereTrace
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start + OwnerCharacter->GetActorForwardVector() * Step.HitRange;;

	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Step.HitRadius);

	DrawDebugSphere(GetWorld(), Start, Step.HitRadius, 12, FColor::Yellow, false, 1.f);
	DrawDebugSphere(GetWorld(), End, Step.HitRadius, 12, FColor::Red, false, 1.f);
	DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);

	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		Sphere
	);

	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("CheckHit呼ばれた: ヒット数=%d"), HitResults.Num()));

	if (!bHit) return;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();

		if (!HitActor || HitActor == OwnerCharacter) continue;

		//同じ攻撃で2回当たらないようにする
		if (HitActorsThisAttack.Contains(HitActor)) continue;
		HitActorsThisAttack.Add(HitActor);

		//ダメージを与える
		UGameplayStatics::ApplyDamage(
			HitActor,
			Step.Damage,
			OwnerCharacter->GetController(),
			OwnerCharacter,
			UDamageType::StaticClass()
		);

		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, FString::Printf(TEXT("EnemyキャストできたかL %s"), Cast<AEnemyChara>(HitActor) ? TEXT("OK") : TEXT("NO")));

		//4段目の吹き飛ばし
		if (Step.LaunchForce > 0.f)
		{
			if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
			{
				FVector LaunchDir = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
				LaunchDir.Z = 0.5f;
				HitCharacter->LaunchCharacter(LaunchDir * Step.LaunchForce, true, true);
			}
		}

		//エネルギー加算を通知
		if (Cast<AEnemyChara>(HitActor))
		{
			OnHitEnemy.Broadcast(Step.Damage * 0.5f);
		}
	}
}


void UCombatComponent::ExecuteDodge()
{
	//リキャスト中なら何もしない
	if (!bCanDodge) return;

	if (bIsAttacking) return;

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

	UActionMovementComponent* ActionMoveComp = Cast<UActionMovementComponent>(MoveComp);
	if (ActionMoveComp)
	{
		ActionMoveComp->bIsDodging = true;
	}

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

		UActionMovementComponent* ActionMoveComp = Cast<UActionMovementComponent>(MoveComp);
		if (ActionMoveComp)
		{
			ActionMoveComp->bIsDodging = false;
		}

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
