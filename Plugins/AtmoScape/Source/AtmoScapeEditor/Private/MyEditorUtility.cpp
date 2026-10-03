#include "MyEditorUtility.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"

void UMyEditorUtilityWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsAutomaticRotationEnabled)
    {
        const float YawDelta = SunYaw + 360.0f * InDeltaTime * RotationSpeed / 24.0f;
        const float PitchDelta = SunPitch + 360.0f * InDeltaTime * RotationSpeed / 24.0f;

        RotateDirectionalLight(YawDelta, PitchDelta);
    }
}

void UMyEditorUtilityWidget::ToggleAutomaticRotation()
{
    // Logique pour activer ou d�sactiver la rotation
    bIsAutomaticRotationEnabled = !bIsAutomaticRotationEnabled;
}

void UMyEditorUtilityWidget::RotateDirectionalLight(float YawDelta, float PitchDelta)
{
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        return;
    }

    for (TActorIterator<ADirectionalLight> It(EditorWorld); It; ++It)
    {
        ADirectionalLight* Light = *It;
        if (Light)
        {
            // Obtenir la rotation actuelle en tant que quaternion
            FQuat CurrentQuat = Light->GetActorQuat();

            // Cr�er des quaternions de rotation pour Yaw et Pitch
            FQuat QuatYaw = FQuat(FRotator(0.0f, YawDelta, 0.0f));
            FQuat QuatPitch = FQuat(FRotator(PitchDelta, 0.0f, 0.0f));

            // Appliquer la rotation en utilisant des quaternions pour �viter le blocage de cardan
            FQuat NewQuat = CurrentQuat * QuatYaw * QuatPitch;

            // Convertir le quaternion en FRotator et l'appliquer � la lumi�re
            Light->SetActorRotation(NewQuat.Rotator());
        }
    }
}

