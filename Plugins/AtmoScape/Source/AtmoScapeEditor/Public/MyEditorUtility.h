#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "MyEditorUtility.generated.h"

UCLASS()
class UMyEditorUtilityWidget : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Light Rotation")
        void ToggleAutomaticRotation();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Rotation")
        float RotationSpeed = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Rotation")
        float SunYaw = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sun Rotation")
        float SunPitch = 0.0f;

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void RotateDirectionalLight(float YawDelta, float PitchDelta);
    bool bIsAutomaticRotationEnabled = false;
};
