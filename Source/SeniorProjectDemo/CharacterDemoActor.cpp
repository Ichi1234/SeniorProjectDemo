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

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ACharacterDemoActor::ACharacterDemoActor()
{
    PrimaryActorTick.bCanEverTick = false;
    TargetActor = nullptr;
    TargetMesh = nullptr;
}

void ACharacterDemoActor::BeginPlay()
{
    Super::BeginPlay();

    // Load API key from .env
    GeminiAPIKey = LoadAPIKeyFromEnv();

    if (GeminiAPIKey.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Gemini API Key is missing!"));
        return;
    }

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

    //GenerateAppearanceFromText(CharacterDescription);
}

FString ACharacterDemoActor::LoadAPIKeyFromEnv()
{
    FString FilePath = FPaths::ProjectDir() + TEXT(".env");
    FString FileContent;

    if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read .env file"));
        return "";
    }

    TArray<FString> Lines;
    FileContent.ParseIntoArrayLines(Lines);

    for (const FString& Line : Lines)
    {
        FString Key, Value;
        if (Line.Split(TEXT("="), &Key, &Value))
        {
            if (Key == "GEMINI_API_KEY")
            {
                return Value.TrimStartAndEnd();
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("GEMINI_API_KEY not found in .env"));
    return "";
}

FString ACharacterDemoActor::BuildPrompt(FString Description)
{
    TArray<FString> MorphTargets = {
      
        "head-diamond", "head-invertedtriangular","head-oval","head-rectangular","head-round",
        "head-square",  "head-triangular", "head-fat-decr", "head-fat-incr"
    };

    FString MorphList;

    ResetAllMorphTargets();

    UE_LOG(LogTemp, Log, TEXT("Reset all morph targets to 0"));

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
        "5. NEVER combine two head shapes together - pick ONLY ONE\n"
        "6. ALWAYS use the exact max value shown below for the chosen shape\n"
        "7. If no head shape is mentioned don't need to include the head shape parameters\n"
        "HEAD SHAPE MAX VALUES (always use these exact values):\n"
        "- head-round: 0.132057\n"
        "- head-square: 0.1\n"
        "- head-oval: 0.13\n"
        "- head-rectangular: 0.090274\n"
        "- head-triangular: 0.074856\n"
        "- head-invertedtriangular: 0.136986\n"
        "- head-diamond: 0.101898\n"
        "HEAD FAT RULES:\n"
        "- head-fat-incr: use 0.2 for chubby/fat face, anything above looks weird\n"
        "- head-fat-decr: use 0.1 for thin/slim face, anything above looks starving\n"
        "- NEVER use head-fat-incr and head-fat-decr together\n"
        "- If no fat/slim is mentioned, do NOT include either\n"
        "EXAMPLE OUTPUT:\n"
        "{\n"
        "  \"head-oval\": 0.13\n"
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
        TEXT("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent?key=%s"),
        *GeminiAPIKey
    );

    FString Prompt = BuildPrompt(Description);

    TSharedPtr<FJsonObject> RequestBody = MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> Content = MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> Part = MakeShareable(new FJsonObject);
    TSharedPtr<FJsonObject> GenerationConfig = MakeShareable(new FJsonObject);

    Part->SetStringField("text", Prompt);

    TArray<TSharedPtr<FJsonValue>> Parts;
    Parts.Add(MakeShareable(new FJsonValueObject(Part)));
    Content->SetArrayField("parts", Parts);

    TArray<TSharedPtr<FJsonValue>> Contents;
    Contents.Add(MakeShareable(new FJsonValueObject(Content)));
    RequestBody->SetArrayField("contents", Contents);

    GenerationConfig->SetStringField("responseMimeType", "application/json");
    RequestBody->SetObjectField("generationConfig", GenerationConfig);

    FString BodyString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
    FJsonSerializer::Serialize(RequestBody.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
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
    UE_LOG(LogTemp, Log, TEXT("Gemini raw response: %s"), *ResponseString);

    TSharedPtr<FJsonObject> ResponseJson;
    TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(ResponseString);

    if (!FJsonSerializer::Deserialize(Reader, ResponseJson))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse response"));
        return;
    }

    FString JsonText;

    const TArray<TSharedPtr<FJsonValue>>* Candidates;
    if (ResponseJson->TryGetArrayField(TEXT("candidates"), Candidates)
        && Candidates->Num() > 0)
    {
        TSharedPtr<FJsonObject> FirstCandidate =
            (*Candidates)[0]->AsObject();

        TSharedPtr<FJsonObject> ContentObj =
            FirstCandidate->GetObjectField("content");

        const TArray<TSharedPtr<FJsonValue>>* PartsArray;
        if (ContentObj->TryGetArrayField(TEXT("parts"), PartsArray)
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

void ACharacterDemoActor::ResetAllMorphTargets()
{
    TArray<FString> AllMorphTargets = {
        "head-diamond", "head-invertedtriangular", "head-oval", "head-rectangular",
        "head-round", "head-square", "head-triangular", "head-fat-decr", "head-fat-incr"
    };

    for (const FString& MorphName : AllMorphTargets)
    {
        SetMorphTargetValue(MorphName, 0.0f);
    }
}