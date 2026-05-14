//GhostAttackHandler.h
//ゴーストアタックハンドラーヘッダー

#pragma once

#include "CoreMinimal.h"
#include "Ghost/AI/GhostActionHandlerBase.h"
#include "GhostAttackHandler.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UGhostAttackHandler : public UGhostActionHandlerBase
{
    GENERATED_BODY()

public:
    virtual EGhostActionType GetHandledType() const override
    {
        return EGhostActionType::Attack;
    }

    virtual void Execute(const FGhostActionData& Data, ACharacter* Owner) override;

    //BPで指定するモンタージュ
    UPROPERTY(EditAnywhere, Category = "Ghost")
    UAnimMontage* AttackMontage;
};