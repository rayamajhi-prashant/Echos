#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyChara.generated.h"

//敵のステート
UENUM(BlueprintType)
enum class EEnemyState : uint8 
{
	None UMETA(DisplayName = "None"),
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	Chase UMETA(DisplayName = "Chase"),
	Attack UMETA(DisplayName = "Attack")
};

UCLASS()
class ECHO_API AEnemyChara : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyChara();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//現在の体力を取得
	float GetCurrentHP() { return m_currentHP; }

	//体力をセットする
	void SetHP(float _hp) { m_currentHP = _hp; }

private:
	//現在の体力
	float m_currentHP;

	//最大の体力
	float m_maxHP;

	//現在の状態
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "AI")
	EEnemyState m_enemyState = EEnemyState::None;
};
