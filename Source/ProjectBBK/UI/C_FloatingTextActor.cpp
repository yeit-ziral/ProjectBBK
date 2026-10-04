// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/C_FloatingTextActor.h"
#include "C_FloatingTextWidget.h"
#include "Components/WidgetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AC_FloatingTextActor::AC_FloatingTextActor()
{
 	PrimaryActorTick.bCanEverTick = true;

	widgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("TextWidget"));
	SetRootComponent(widgetComp);

	// 월드 공간에 그려야 거리에 따라 크기가 변하고 3D 위치를 가질 수 있다.
	widgetComp->SetWidgetSpace(EWidgetSpace::World);
	widgetComp->SetDrawSize(FVector2D(300.f, 80.f));
	widgetComp->SetTwoSided(true);

	// 연출용이라 아무것과도 부딪힐 필요가 없다.
	widgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AC_FloatingTextActor::Initialize(const FText& InText, const FLinearColor& InColor)
{
	if(UC_FloatingTextWidget* W = Cast<UC_FloatingTextWidget>(widgetComp->GetUserWidgetObject()))
		W->SetTextAndColor(InText, InColor);
}

// Called when the game starts or when spawned
void AC_FloatingTextActor::BeginPlay()
{
	Super::BeginPlay();
	
	startLocation = GetActorLocation();
	
	// 시간이 지나면 스스로 사라진다
	SetLifeSpan(lifeTime); 

	// 크리가 연달아 터질 때 같은 자리에 겹쳐 읽기 어려워지는 걸 막는다
	horizontalVelocity = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal() * horizontalJitterSpeed;
}

// Called every frame
void AC_FloatingTextActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	elapsed += DeltaTime;

	// 속도를 매 틱 누적하지 않고 시간 t를 직접 대입한다.
	//   y(t) = v0*t - 0.5*g*t^2

	const float height = initialSpeed * elapsed - 0.5f * gravity * elapsed * elapsed;
	SetActorLocation(startLocation + horizontalVelocity * elapsed + FVector(0.f, 0.f, height));

	// 알파: fadeStartRatio 지점까지는 1, 이후 0까지 선형 감소
	const float lifeRatio = (lifeTime > 0.f) ? FMath::Clamp(elapsed / lifeTime, 0.f, 1.f) : 1.f;
	const float alpha = (lifeRatio < fadeStartRatio)
		? 1.f
		: 1.f - (lifeRatio - fadeStartRatio) / FMath::Max(1.f - fadeStartRatio, KINDA_SMALL_NUMBER);

	if (UC_FloatingTextWidget* W = Cast<UC_FloatingTextWidget>(widgetComp->GetUserWidgetObject()))
			W->SetAlpha(alpha);

	// 항상 카메라를 향하게. 180도 더하지 않으면 글자가 좌우 반전돼 보인다.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector CamRot = Cam->GetCameraLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.f, CamRot.Y + 180.f, 0.f));
	}
}

