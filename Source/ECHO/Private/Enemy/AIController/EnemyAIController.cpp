#include "Enemy/AIController/EnemyAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyAIController::AEnemyAIController() : m_pAIPerceptionComp(nullptr), m_pSightConfig(nullptr), m_pHearingConfig(nullptr)
{
	m_pAIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	//視覚
	m_pSightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	m_pSightConfig->SightRadius = 2000.f;
	m_pSightConfig->LoseSightRadius = 2500.f;
	m_pSightConfig->PeripheralVisionAngleDegrees = 90.f;
	m_pSightConfig->SetMaxAge(5.f);
	
	//聴覚
	m_pHearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	m_pHearingConfig->HearingRange = 2000.f;
	m_pHearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	m_pHearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	m_pHearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	//
	m_pAIPerceptionComp->ConfigureSense(*m_pSightConfig);
	m_pAIPerceptionComp->SetDominantSense(m_pSightConfig->GetSenseImplementation());
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyAIController::OnPossess(APawn* _inPawn)
{
	Super::OnPossess(_inPawn);
}


void AEnemyAIController::OnTargetPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus)
{
	if (_actor->ActorHasTag("Player"))
	{
		if (_stimulus.WasSuccessfullySensed())
		{
			GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), _actor);
		}
	}
	else
	{
		GetBlackboardComponent()->ClearValue(TEXT("TargetActor"));
	}
}

