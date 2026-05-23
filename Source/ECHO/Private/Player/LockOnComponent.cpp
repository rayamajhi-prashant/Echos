//LockOnComponent.cpp
//ロックオンコンポーネントソース

#include "Player/LockOnComponent.h"
#include "Enemy/EnemyChara.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentTarget.IsValid()) return;

	//ターゲットが無効になったら自動解除
	if (!IsTargetValid(CurrentTarget.Get()))
	{
		ClearLockOn();
	}
}

void ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
	}
	else
	{
		AActor* Best = FindBestTarget();
		if (Best)
		{
			CurrentTarget = Best;
			OnLockOnChanged.Broadcast(Best);
		}
	}
}

void ULockOnComponent::ClearLockOn()
{
	CurrentTarget = nullptr;
	OnLockOnChanged.Broadcast(nullptr);
}

bool ULockOnComponent::IsTargetValid(AActor* Target) const
{
	if (!Target) return false;
	
	//距離チェック
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return false;

	float Distance = FVector::Dist(
		OwnerCharacter->GetActorLocation(),
		Target->GetActorLocation()
	);

	if (Distance > LockOnBreakRange) return false;

	//生存チェック
	//今は距離だけで判定、死亡判定は敵クラスに合わせて後で追加
	return true;
}

AActor* ULockOnComponent::FindBestTarget() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return nullptr;

	//カメラを取得
	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return nullptr;

	FVector CameraForward = Camera->GetForwardVector();
	FVector CameraLocation = Camera->GetComponentLocation();

	//全エネミーを取得
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyChara::StaticClass(), Enemies);

	AActor* BestTarget = nullptr;
	float BestDot = -1.f;

	for (AActor* Enemy : Enemies)
	{
		if (!Enemy) continue;

		//距離チェック
		float Distance = FVector::Dist(
			OwnerCharacter->GetActorLocation(),
			Enemy->GetActorLocation()
		);
		if (Distance > LockOnRange) continue;

		//カメラ前方との内積で画面中央に近いものを選ぶ
		FVector ToEnemy = (Enemy->GetActorLocation() - CameraLocation).GetSafeNormal();
		float Dot = FVector::DotProduct(CameraForward, ToEnemy);

		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

