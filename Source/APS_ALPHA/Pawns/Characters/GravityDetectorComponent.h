#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Core/Enums/GravityTypeEnum.h"
#include "Components/ActorComponent.h"
#include "GravityDetectorComponent.generated.h"

enum class EGravityType : uint8;
class ACharacter;
class UCapsuleComponent;
class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGravityTypeChanged, EGravityType, NewType, AActor*, ZoneActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGravityZoneEnter,  EGravityType, ZoneType, AActor*, ZoneActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGravityZoneExit,   EGravityType, ZoneType, AActor*, ZoneActor);

/**
 * Компонент-детектор гравитационных зон + ZeroG-передвижение.
 * Добавляется в любой BP/Actor/Character. Сам подписывается на overlaps капсулы (или первого Primitive).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class APS_ALPHA_API UGravityDetectorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGravityDetectorComponent();

    /** === События === */
    UPROPERTY(BlueprintAssignable, Category="Gravity")
    FOnGravityTypeChanged OnGravityTypeChanged;

    UPROPERTY(BlueprintAssignable, Category="Gravity")
    FOnGravityZoneEnter OnGravityZoneEnter;

    UPROPERTY(BlueprintAssignable, Category="Gravity")
    FOnGravityZoneExit OnGravityZoneExit;

    /** Текущее состояние */
    UFUNCTION(BlueprintCallable, Category="Gravity")
    EGravityType GetCurrentGravityType() const { return CurrentType; }

    /** Текущая зона (может быть nullptr в ZeroG) */
    UFUNCTION(BlueprintCallable, Category="Gravity")
    AActor* GetCurrentZoneActor() const { return CurrentZoneActor.Get(); }

    /** Принудительный ZeroG (игнорировать зоны до отключения) */
    UFUNCTION(BlueprintCallable, Category="Gravity")
    void ForceZeroG(bool bEnable);

    /** === ZeroG-параметры (редактируемые) === */
    /** Базовая скорость ZeroG: масштаб входа для AddMovementInput (600 ~ стандартная скорость UE) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZeroG")
    float ZeroGMoveSpeed = 2000.f;

    /** Поворачивать ли меш к ориентации (Orientation) при движении */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZeroG")
    bool bRotateMeshToOrientation = true;

    /** Ориентация для осей X/Y/Z (например, Arrow или SpringArm). Если не задано — берётся ActorRotation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZeroG")
    USceneComponent* Orientation = nullptr;

    /** Применить безопасные настройки движения для ZeroG (MOVE_Flying, GravityScale=0 и т.п.) */
    UFUNCTION(BlueprintCallable, Category="ZeroG")
    void ApplyZeroGMovementSettings();

    /** Управление в ZeroG: оси движения */
    UFUNCTION(BlueprintCallable, Category="ZeroG")
    void ZeroG_MoveForward(float Value);

    UFUNCTION(BlueprintCallable, Category="ZeroG")
    void ZeroG_MoveRight(float Value);

    UFUNCTION(BlueprintCallable, Category="ZeroG")
    void ZeroG_MoveUp(float Value);

    /** Повернуть меш к ориентации (мгновенно). Для плавности вызывайте чаще/в тике по желанию */
    UFUNCTION(BlueprintCallable, Category="ZeroG")
    void ZeroG_RotateMeshTowardsOrientation();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    /** Подписаться на overlaps владельца (Capsule у Character, иначе первый Primitive) */
    void BindOwnerOverlap();

    /** Overlap-обработчики */
    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                            const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                          UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    /** Сменить состояние и сгенерировать событие при изменении */
    void SetGravityType(EGravityType NewType, AActor* ZoneActor);

    /** Когда вышли из зоны: пересчитать лучшую зону (или ZeroG) */
    void FallbackAfterExit();

    /** Пересчитать текущую зону по списку пересечений (выбор ближайшей) */
    void ResolveCurrentZone();

    /** Найти ближайшую зону среди OverlappingZones */
    AActor* FindNearestZone() const;

    /** Расстояние до актора (DistSquared для скорости) */
    float DistanceToActor(const AActor* Zone) const;

    /** Сопоставить класс актора-зоны типу гравитации */
    static EGravityType ClassToType(const AActor* Zone);

private:
    /** Владелец как Character (если нет — компонент работает, но без ZeroG-движения) */
    TWeakObjectPtr<ACharacter> OwnerCharacter;

    /** Капсула владельца (если Character) */
    TWeakObjectPtr<UCapsuleComponent> OwnerCapsule;

    /** На каком компоненте слушаем overlaps */
    TWeakObjectPtr<UPrimitiveComponent> BoundCollision;

    /** Текущее состояние */
    EGravityType CurrentType = EGravityType::ZeroG;
    TWeakObjectPtr<AActor> CurrentZoneActor;

    /** Активные пересечения с зонами */
    TArray<TWeakObjectPtr<AActor>> OverlappingZones;

    /** Принудительный ZeroG */
    bool bForcedZeroG = false;
};
