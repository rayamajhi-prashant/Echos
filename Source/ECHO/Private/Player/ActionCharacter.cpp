//ActionCharacter.cpp
//アクションキャラクターソース

#include "Player/ActionCharacter.h"
#include "Player/ActionMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player//CombatComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "DrawDebugHelpers.h"//画面デバッグ用
//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("A"));

//FObjectInitializerを使用して、標準のMovementComponentを自作のものに差し替える
AActionCharacter::AActionCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UActionMovementComponent>(ACharacter::CharacterMovementComponentName)),
	TimeToSprint(1.5f),
	CurrentRunTime(0.f),
	GhostSummonCost(30.f),
	MaxEnergy(100.f),
	EnergyGainPerHit(15.f),
	CurrentEnergy(0.f),
	ActiveGhostCount(0),
	MaxGhostCount(5),
	JumpPressedTime(0.f),
	JumpHoldThreshold(0.2f),
	JumpZVelocityShort(1800.f)
{
	PrimaryActorTick.bCanEverTick = true;

	//二段ジャンプの設定
	JumpMaxCount = 2;

	//コントローラーの回転でキャラ自体が回らないようにする
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	//移動方向にキャラクターが自動でスッと向くようにする
	GetActionMovementComponent()->bOrientRotationToMovement = true;

	//キャラクターが振り向く速度
	GetActionMovementComponent()->RotationRate = FRotator(0.0f, 1000.0f, 0.0f);

	//戦闘コンポーネントを作成してキャラクターに追加する
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));


	// ----------------------------------------------------
	//カメラの設定（簡易的）
	// ----------------------------------------------------
	//スプリングアームの作成と取り付け
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	//視点操作の動きに合わせてアームを回転させるか
	CameraBoom->bUsePawnControlRotation = true;

	//基本の長さ
	CameraBoom->TargetArmLength = 400.0f;
	// 壁の衝突テスト
	CameraBoom->bDoCollisionTest = true;
	//カメラが反応するチャンネルををWorldStaticのみにする
	CameraBoom->ProbeChannel = ECC_GameTraceChannel1;
	//判定用の球の大きさを少し小さくしてがたつきを抑える
	CameraBoom->ProbeSize = 10.f;
	// 右肩越しオフセット
	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 50.0f);
	// カメラスムージング
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 15.0f;


	//カメラ本体の作成と取り付け
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	// スプリングアームの先端に取り付ける！
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// カメラ自体はアームの動きに追従するだけで、自分では回転しない
	FollowCamera->bUsePawnControlRotation = false;


	//残像コンポーネントを作成してキャラクターに追加する
	GhostRecorder = CreateDefaultSubobject<UGhostRecorderComponent>(TEXT("GhostRecorder"));
}

UActionMovementComponent* AActionCharacter::GetActionMovementComponent() const
{
	return Cast<UActionMovementComponent>(GetCharacterMovement());
}

void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();

	//エネルギー0スタート
	CurrentEnergy = 0.f;

	//CombatComponentのヒットデリゲートを取得
	if (CombatComponent)
	{
		CombatComponent->OnHitEnemy.AddUObject(
			this,
			&AActionCharacter::OnHitEnemy
		);
	}


	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
	//	FString::Printf(TEXT("BeginPlay GhostRecorderアドレス: %p"), GhostRecorder));

	//プレイヤーコントローラーを取得しIMCを登録する
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AActionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//キャラクターの現在の平行移動速度を取得
	float CurrentSpeed = GetVelocity().Size2D();

	//速度が一定以上かチェック
	if (CurrentSpeed > 10.f)
	{
		//走っている時間を加算
		CurrentRunTime += DeltaTime;

		//設定した時間を超えたらダッシュ開始
		if (CurrentRunTime >= TimeToSprint)
		{
			StartSprint();
		}
	}
	else
	{
		//立ち止まったら
		CurrentRunTime = 0.f;
		StopSprint();
	}
}

void AActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//攻撃のバインド
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AActionCharacter::Attack);

		//移動
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AActionCharacter::Move);

		//視点移動
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AActionCharacter::Look);

		//ジャンプ
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AActionCharacter::OnJumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AActionCharacter::OnJumpReleased);

		//回避のバインド
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AActionCharacter::Dodge);

		//召喚バインド
		EnhancedInputComponent->BindAction(SummonAction, ETriggerEvent::Started, this, &AActionCharacter::SummonGhost);

	}
}

void AActionCharacter::Attack()
{
	if (!CombatComponent)
	{
		return;
	}

	CombatComponent->ExecuteAttack();

}


void AActionCharacter::OnHitEnemy(float EnergyGain)
{
	CurrentEnergy = FMath::Clamp(CurrentEnergy + EnergyGain, 0.f, MaxEnergy);

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
		FString::Printf(TEXT("Energy: %.1f / %.1f"), CurrentEnergy, MaxEnergy));
}


void AActionCharacter::Move(const FInputActionValue& Value)
{
	//入力値を2Dベクトルとして取得
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		//コントローラーが向いている回転を取得
		const FRotator Rotation = Controller->GetControlRotation();
		//Z軸だけを抽出し、ピッチやロール傾きは無視する
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		//カメラが向いている方向の「前」ベクトルの取得
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		//カメラが向いている方向の「右」ベクトルの取得
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		//キャラクターに移動命令を出す
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

//走る入力が開始（Started）された時
void AActionCharacter::StartSprint()
{
	if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
	{
		MoveComp->SetSprinting(true);
	}
}

//走る入力が終了（Completed）した時
void AActionCharacter::StopSprint()
{
	if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
	{
		MoveComp->SetSprinting(false);
	}
}

void AActionCharacter::Look(const FInputActionValue& Value)
{
	//マウスの移動量（XとY）を取得
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		//左右のカメラ回転（Yaw）
		AddControllerYawInput(LookAxisVector.X);
		//上下のカメラ回転（Pitch）
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AActionCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	UActionMovementComponent* MoveComp = GetActionMovementComponent();
	if (!MoveComp) return;

	//現在のジャンプ回数を確認
	//JumpCurrentCount はACharacterに標準で用意されている「現在何回目のジャンプか」を持つ変数
	if (JumpCurrentCount == 1)
	{
		//【1段目のジャンプ時の処理】
		
		//空中横移動を抑える
		MoveComp->AirControl = MoveComp->AirControlFirstJump;

		//地面を蹴る土煙のエフェクト(Niagara)を足元に出す
		//「ハッ！」という通常ジャンプのボイスやSEを再生する
	}
	else if (JumpCurrentCount == 2)
	{
		//【2段目のジャンプ（エアハイク）時の処理】
		
		//横移動量をさらに抑えてZ初速を上書き
		MoveComp->AirControl = MoveComp->AirControlSecondJump;

		//現在のZ速度をSecondJumpZVelocityで上書き
		FVector CurrentVelocity = MoveComp->Velocity;
		CurrentVelocity.Z = MoveComp->SecondJumpZVelocity;
		MoveComp->Velocity = CurrentVelocity;

		FVector InputVector = MoveComp->GetLastInputVector();
		if (!InputVector.IsNearlyZero())
		{
			FVector InputDir = InputVector.GetSafeNormal2D();
			SetActorRotation(InputDir.Rotation());

			//入力方向に速度XYを向ける
			float HorizontalSpeed = CurrentVelocity.Size2D();
			MoveComp->Velocity.X = InputDir.X * HorizontalSpeed;
			MoveComp->Velocity.Y = InputDir.Y * HorizontalSpeed;
		}

		//空中に魔法陣や衝撃波のエフェクトを出す
		//キャラクターが空中でクルッと回転するような専用のモンタージュを再生する
		//Z軸（上方向）に少し追加の初速を与えて、滞空時間を伸ばす
	}
}

void AActionCharacter::OnJumpPressed()
{
	//押した時刻を記録
	JumpPressedTime = GetWorld()->GetTimeSeconds();

	//とりあえずジャンプ自体は即座に開始
	Jump();
}

void AActionCharacter::OnJumpReleased()
{
	StopJumping();

	if (JumpCurrentCount >= 2) return;

	float HoldDuration = GetWorld()->GetTimeSeconds() - JumpPressedTime;

	UActionMovementComponent* MoveComp = GetActionMovementComponent();
	if (!MoveComp) return;

	//押し込み時間でZ速度を上書き
	if (MoveComp->IsFalling())
	{
		FVector Vel = MoveComp->Velocity;

		if (HoldDuration < JumpHoldThreshold)
		{
			//短押し：速度を抑える
			Vel.Z = FMath::Min(Vel.Z, JumpZVelocityShort);
		}
		//長押しはそのままJumpZVelocity（CharacterMovementのデフォルト）を使う

		MoveComp->Velocity = Vel;
	}
}

void AActionCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	UActionMovementComponent* MoveComp = GetActionMovementComponent();

	if (MoveComp)
	{
		//着地時にAirControlを元に戻す
		MoveComp->AirControl = 0.f;

		//走り状態だったかチェックして着地硬直開始
		bool bWasSprinting = MoveComp->IsSprinting();
		MoveComp->StartLandingRecovery(bWasSprinting);
	}

	if (CombatComponent)
	{
		CombatComponent->ResetAirDodge();
	}

	//【着地時の処理】
	//着地した瞬間に「ドスッ」という重いSEと、足元に砂埃エフェクトを出す
	//高い場所から落ちた場合（落下速度 Z が一定以上だった場合）、数フレームだけ移動入力を無視して「着地硬直」のアニメーションを入れる
	//空中コンボ中だった場合、コンボのステート（状態）をリセットする
}

//回避
void AActionCharacter::Dodge()
{
	if (CombatComponent)
	{
		CombatComponent->ExecuteDodge();
	}
}

//残像召喚
void AActionCharacter::SummonGhost()
{
	//エネルギーチェック
	if (CurrentEnergy < GhostSummonCost) return;

	//レコーダーからデータ取得
	if (!GhostRecorder) return;
	TArray<FGhostActionData> Actions = GhostRecorder->GetRecordedActions();

	//データが空なら召喚しない
	if (Actions.IsEmpty()) return;

	//プレイヤーの後ろにSpawn
	FVector SpawnLocation = GetActorLocation() + GetActorRightVector() * 100.f;
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters Params;
	Params.Owner = this;

	AGhostCharacter* Ghost = GetWorld()->SpawnActor<AGhostCharacter>(
		GhostCharacterClass,
		SpawnLocation,
		SpawnRotation,
		Params
	);

	if (Ghost)
	{
		Ghost->InitGhost(Actions);
		CurrentEnergy -= GhostSummonCost;
		ActiveGhostCount++;
	}
}
