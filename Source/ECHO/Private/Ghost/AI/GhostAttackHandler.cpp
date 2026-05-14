//GhostAttackHandler.cpp
//ゴーストアタックハンドラーソース

#include "Ghost/AI/GhostAttackHandler.h"
#include "GameFramework/Character.h"

void UGhostAttackHandler::Execute(const FGhostActionData& Data, ACharacter* Owner)
{
    if (!Owner) return;

    Owner->SetActorRotation(Data.Rotation);

    if (AttackMontage)
    {
        Owner->PlayAnimMontage(AttackMontage);
    }
}

