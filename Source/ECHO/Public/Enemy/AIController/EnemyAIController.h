#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

//前方宣言
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviourTree;


UCLASS()
class ECHO_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

protected:
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* _inPawn) override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* _actor, FAIStimulus _stimulus);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "AI")
	UAIPerceptionComponent* m_pAIPerceptionComp;

	UAISenseConfig_Sight* m_pSightConfig;

	UAISenseConfig_Hearing* m_pHearingConfig;
};
