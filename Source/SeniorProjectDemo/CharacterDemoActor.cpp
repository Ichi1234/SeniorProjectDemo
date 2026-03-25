#include "CharacterDemoActor.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Components/SkeletalMeshComponent.h"

ACharacterDemoActor::ACharacterDemoActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TargetActor = nullptr;
    TargetMesh = nullptr;
}

void ACharacterDemoActor::BeginPlay()
{
    Super::BeginPlay();

    if (!TargetActor)
    {
        UE_LOG(LogTemp, Error, TEXT("TargetActor not assigned!"));
        return;
    }

    TargetMesh = TargetActor->FindComponentByClass
        <USkeletalMeshComponent>();

    if (!TargetMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("No skeletal mesh found!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Mesh found! Applying JSON..."));

    FString TestJSON = TEXT(R"({
        "l-cheek-bones-incr": 1,
        "l-ear-scale-vert-incr": 1
    })");

    ApplyAppearanceJSON(TestJSON);
}

void ACharacterDemoActor::SetMorphTargetValue(
    FString MorphTargetName, float Value)
{
    if (!TargetMesh) return;

    float SafeValue = FMath::Clamp(Value, 0.0f, 1.0f);
    TargetMesh->SetMorphTarget(FName(*MorphTargetName), SafeValue);

    UE_LOG(LogTemp, Log, TEXT("Set %s = %f"),
        *MorphTargetName, SafeValue);
}

void ACharacterDemoActor::ApplyAppearanceJSON(FString JSONString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
        return;
    }

    for (auto& Pair : JsonObject->Values)
    {
        if (Pair.Value->Type == EJson::Number)
        {
            double Value = Pair.Value->AsNumber();
            SetMorphTargetValue(Pair.Key, (float)Value);
        }
    }
}
