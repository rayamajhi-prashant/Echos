//ActionCharacter.h
//アクションキャラクターヘッダー
// 
//プレイヤーのコアクラス
//入力とカメラ制御、各コンポーネントへの処理の委譲

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Ghost/GhostCharacter.h"
#include "Ghost/GhostRecorderComponent.h"
#include "ActionCharacter.generated.h"

class UCombatComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class ECHO_API AActionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AActionCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	//入力をバインドする関数（ACharacterの標準機能をオーバーライド）
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//ジャンプが成功したときに呼ばれる関数
	virtual void OnJumped_Implementation() override;

	//地面などに着地した時に呼ばれる関数
	virtual void Landed(const FHitResult& Hit) override;

	//入力マッピングコンテキスト（キー割り当てのまとめ）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	//戦闘統括コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCombatComponent* CombatComponent;

	//攻撃アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AttackAction;

	//攻撃入力時に呼ばれる関数
	void Attack();

	//移動アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	//移動入力がトリガーされた時に呼ばれる関数
	void Move(const FInputActionValue& Value);

	//オートダッシュへの移行時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TimeToSprint;

	//走り続けている時間を計測する変数
	float CurrentRunTime;

	//走り開始と終了
	void StartSprint();
	void StopSprint();


	//ジャンプアクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;


	//視点移動アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;

	//視点移動がトリガーされた時に呼ばれる関数
	void Look(const FInputActionValue& Value);


	//回避アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* DodgeAction;

	//回避入力時に呼ばれる関数
	void Dodge();


	//召喚アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SummonAction;


	// カメラを繋ぐ棒（スプリングアーム）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	// カメラ本体
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

public:
	//自身のMovementComponentをキャストして取得しやすくする
	UFUNCTION(BlueprintCallable, Category = "Movement")
	class UActionMovementComponent* GetActionMovementComponent() const;

	// ---召喚---
	UFUNCTION(BlueprintCallable)
	void SummonGhost();

private:
	void OnHitEnemy(float EnergyGain);

	UPROPERTY(VisibleAnywhere)
	UGhostRecorderComponent* GhostRecorder;

	//召喚コスト
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	float GhostSummonCost;

	//得られるエネルギー
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float EnergyGainPerHit;

	//スポーンするGhostCharacterのクラス
	UPROPERTY(EditAnywhere, Category = "Ghost")
	TSubclassOf<AGhostCharacter> GhostCharacterClass;

public:
	//現在のエネルギー
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float CurrentEnergy;

	//マックスエネルギー
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float MaxEnergy;

	//現在召喚中の残像数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 ActiveGhostCount;

	//残像の最大数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 MaxGhostCount;


};
