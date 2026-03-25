#include "CharacterDemoActor.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Components/SkeletalMeshComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

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

    TargetMesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();

    if (!TargetMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("No skeletal mesh found!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Mesh found! Ready."));

    GenerateAppearanceFromText(CharacterDescription);
}

FString ACharacterDemoActor::BuildPrompt(FString Description)
{
    TArray<FString> MorphTargets = {
        "l-cheek-bones-incr", "l-cheek-bones-decr",
        "r-cheek-bones-incr", "r-cheek-bones-decr",

        "chin-width-incr", "chin-width-decr",
        "chin-height-incr", "chin-height-decr",

        "l-ear-scale-vert-incr", "l-ear-scale-vert-decr",
        "r-ear-scale-vert-incr", "r-ear-scale-vert-decr",

        "l-eye-scale-incr", "l-eye-scale-decr",
        "r-eye-scale-incr", "r-eye-scale-decr",

        "nose-scale-horiz-incr", "nose-scale-horiz-decr",
        "nose-scale-vert-incr", "nose-scale-vert-decr",

        "mouth-scale-horiz-incr", "mouth-scale-horiz-decr",
        "mouth-scale-vert-incr", "mouth-scale-vert-decr",

        "head-scale-horiz-incr", "head-scale-horiz-decr",
        "head-scale-vert-incr", "head-scale-vert-decr"
    };

    FString MorphList;

    for (int i = 0; i < MorphTargets.Num(); i++)
    {
        MorphList += MorphTargets[i];
        if (i < MorphTargets.Num() - 1)
        {
            MorphList += ", ";
        }
    }

    return FString::Printf(TEXT(
        "You are an AI that converts character descriptions into morph target values.\n\n"

        "Character Description:\n%s\n\n"

        "Available morph targets:\n[%s]\n\n"

        "STRICT RULES:\n"
        "1. Output ONLY valid JSON\n"
        "2. Do NOT include explanations\n"
        "3. Do NOT include markdown\n"
        "4. Use ONLY morph names from the list\n"
        "5. Each value MUST be a float between 0.0 and 1.0\n"
        "6. Do NOT use values lower than 0.0 or higher than 1.0\n"
        "7. Include ONLY relevant morph targets\n\n"

        "Example:\n"
        "{\n"
        "  \"l-ear-scale-vert-incr\": 0.9,\n"
        "  \"r-ear-scale-vert-incr\": 0.9,\n"
        "  \"l-cheek-bones-incr\": 0.8,\n"
        "  \"r-cheek-bones-incr\": 0.8\n"
        "}\n"
    ), *Description, *MorphList);
}

void ACharacterDemoActor::GenerateAppearanceFromText(FString Description)
{
    if (GeminiAPIKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("GeminiAPIKey is empty!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Calling Gemini API..."));

    FString URL = FString::Printf(
        TEXT("https://generativelanguage.googleapis.com/v1beta/"
            "models/gemini-2.0-flash:generateContent?key=%s"),
        *GeminiAPIKey
    );

    FString Prompt = BuildPrompt(Description);

    TSharedPtr<FJsonObject> RequestBody =
        MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> Content =
        MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> Part =
        MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> GenerationConfig =
        MakeShareable(new FJsonObject);

    Part->SetStringField("text", Prompt);

    TArray<TSharedPtr<FJsonValue>> Parts;
    Parts.Add(MakeShareable(new FJsonValueObject(Part)));
    Content->SetArrayField("parts", Parts);

    TArray<TSharedPtr<FJsonValue>> Contents;
    Contents.Add(MakeShareable(new FJsonValueObject(Content)));
    RequestBody->SetArrayField("contents", Contents);

    GenerationConfig->SetStringField(
        "responseMimeType", "application/json");
    RequestBody->SetObjectField(
        "generationConfig", GenerationConfig);

    FString BodyString;
    TSharedRef<TJsonWriter<>> Writer =
        TJsonWriterFactory<>::Create(&BodyString);
    FJsonSerializer::Serialize(RequestBody.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest> HttpRequest =
        FHttpModule::Get().CreateRequest();
    HttpRequest->SetURL(URL);
    HttpRequest->SetVerb("POST");
    HttpRequest->SetHeader("Content-Type", "application/json");
    HttpRequest->SetContentAsString(BodyString);
    HttpRequest->OnProcessRequestComplete().BindUObject(
        this, &ACharacterDemoActor::OnGeminiResponse);
    HttpRequest->ProcessRequest();
}

void ACharacterDemoActor::OnGeminiResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bSuccess)
{
    if (!bSuccess || !Response)
    {
        UE_LOG(LogTemp, Error, TEXT("HTTP request failed!"));
        return;
    }

    FString ResponseString = Response->GetContentAsString();
    UE_LOG(LogTemp, Log, TEXT("Gemini raw response: %s"),
        *ResponseString);

    TSharedPtr<FJsonObject> ResponseJson;
    TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(ResponseString);

    if (!FJsonSerializer::Deserialize(Reader, ResponseJson))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse response"));
        return;
    }

    // Extract text from Gemini response structure
    FString JsonText;
    const TArray<TSharedPtr<FJsonValue>>* Candidates;
    if (ResponseJson->TryGetArrayField(
        TEXT("candidates"), Candidates)
        && Candidates->Num() > 0)
    {
        TSharedPtr<FJsonObject> FirstCandidate =
            (*Candidates)[0]->AsObject();
        TSharedPtr<FJsonObject> ContentObj =
            FirstCandidate->GetObjectField("content");
        const TArray<TSharedPtr<FJsonValue>>* PartsArray;
        if (ContentObj->TryGetArrayField(
            TEXT("parts"), PartsArray)
            && PartsArray->Num() > 0)
        {
            JsonText = (*PartsArray)[0]
                ->AsObject()->GetStringField("text");
        }
    }

    if (JsonText.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Empty JSON from Gemini"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Applying JSON: %s"), *JsonText);
    ApplyAppearanceJSON(JsonText);
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
    TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JSONString);

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