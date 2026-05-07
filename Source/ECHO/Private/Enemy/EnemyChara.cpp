#include "Enemy/EnemyChara.h"
#include "Enemy/AIController/EnemyAIController.h"

// Sets default values
AEnemyChara::AEnemyChara() : m_maxHP(100.f), m_currentHP(0.0f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEnemyChara::BeginPlay()
{
	Super::BeginPlay();
	

	m_currentHP = m_maxHP;
}

// Called every frame
void AEnemyChara::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



