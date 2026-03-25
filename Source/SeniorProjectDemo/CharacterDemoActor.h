#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "CharacterDemoActor.generated.h"

UCLASS()
class SENIORPROJECTDEMO_API ACharacterDemoActor : public AActor
{
    GENERATED_BODY()

public:
    ACharacterDemoActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
    AActor* TargetActor;

    UPROPERTY()
    USkeletalMeshComponent* TargetMesh;  // ← renamed from SkeletalMeshComp

    UFUNCTION(BlueprintCallable)
    void SetMorphTargetValue(FString MorphTargetName, float Value);

    UFUNCTION(BlueprintCallable)
    void ApplyAppearanceJSON(FString JSONString);

protected:
    virtual void BeginPlay() override;
};
