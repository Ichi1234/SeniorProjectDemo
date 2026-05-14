#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "CharacterDemoActor.generated.h"

// Declare delegate with no params - widget will read values itself
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAppearanceAppliedSignature);

UCLASS()
class SENIORPROJECTDEMO_API ACharacterDemoActor : public AActor
{
    GENERATED_BODY()
public:
    ACharacterDemoActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
    AActor* TargetActor;

    UPROPERTY()
    USkeletalMeshComponent* TargetMesh;

    FString GeminiAPIKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FString CharacterDescription;

    // Event Dispatcher - no TMap to avoid compile issues
    UPROPERTY(BlueprintAssignable, Category = "Appearance")
    FOnAppearanceAppliedSignature OnAppearanceApplied;

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    void SetMorphTargetValue(FString MorphTargetName, float Value);

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    float GetMorphTargetValue(FString MorphTargetName);

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    void ApplyAppearanceJSON(FString JSONString);

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    void GenerateAppearanceFromText(FString Description);

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    void ResetAllMorphTargets();

    UFUNCTION(BlueprintCallable, Category = "Appearance")
    void SetSkinTone(FString SkinTone); // "light", "medium", "dark"

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Light")
    UMaterialInterface* Mat_Body_Light;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Light")
    UMaterialInterface* Mat_Ears_Light;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Light")
    UMaterialInterface* Mat_Lips_Light;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Light")
    UMaterialInterface* Mat_Nails_Light;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Medium")
    UMaterialInterface* Mat_Body_Medium;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Medium")
    UMaterialInterface* Mat_Ears_Medium;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Medium")
    UMaterialInterface* Mat_Lips_Medium;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Medium")
    UMaterialInterface* Mat_Nails_Medium;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Dark")
    UMaterialInterface* Mat_Body_Dark;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Dark")
    UMaterialInterface* Mat_Ears_Dark;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Dark")
    UMaterialInterface* Mat_Lips_Dark;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin - Dark")
    UMaterialInterface* Mat_Nails_Dark;

private:
    void OnGeminiResponse(FHttpRequestPtr Request,
        FHttpResponsePtr Response, bool bSuccess);
    FString BuildPrompt(FString Description);
    FString LoadAPIKeyFromEnv();

protected:
    virtual void BeginPlay() override;
};