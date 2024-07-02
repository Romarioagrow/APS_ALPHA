#pragma once

UENUM(BlueprintType)
enum class EOrbitDistributionType : uint8
{
    Uniform     UMETA(DisplayName = "Uniform"),
    Gaussian    UMETA(DisplayName = "Gaussian"),
    Chaotic     UMETA(DisplayName = "Chaotic"),
    InnerOuter  UMETA(DisplayName = "InnerOuter"),
    Dense       UMETA(DisplayName = "Dense"),
   // Count       UMETA(DisplayName = "Count") 
}; 
