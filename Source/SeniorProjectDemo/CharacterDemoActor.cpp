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
        // Cheeks
        "l-cheek-bones-decr","l-cheek-bones-incr","l-cheek-inner-decr","l-cheek-inner-incr",
        "l-cheek-trans-down","l-cheek-trans-up","l-cheek-volume-decr","l-cheek-volume-incr",
        "r-cheek-bones-decr","r-cheek-bones-incr","r-cheek-inner-decr","r-cheek-inner-incr",
        "r-cheek-trans-down","r-cheek-trans-up","r-cheek-volume-decr","r-cheek-volume-incr",

        // Chin
        "chin-bones-decr","chin-bones-incr","chin-cleft-decr","chin-cleft-incr",
        "chin-height-decr","chin-height-incr","chin-jaw-drop-decr","chin-jaw-drop-incr",
        "chin-prognathism-decr","chin-prognathism-incr","chin-prominent-decr","chin-prominent-incr",
        "chin-triangle","chin-width-decr","chin-width-incr",

        // Left Ear
        "l-ear-flap-decr","l-ear-flap-incr","l-ear-lobe-decr","l-ear-lobe-incr",
        "l-ear-rot-backward","l-ear-rot-forward","l-ear-scale-decr",
        "l-ear-scale-depth-decr","l-ear-scale-depth-incr","l-ear-scale-incr",
        "l-ear-scale-vert-decr","l-ear-scale-vert-incr",
        "l-ear-shape-pointed","l-ear-shape-round","l-ear-shape-square","l-ear-shape-triangle",
        "l-ear-trans-backward","l-ear-trans-down","l-ear-trans-forward","l-ear-trans-up",
        "l-ear-wing-decr","l-ear-wing-incr",

        // Right Ear
        "r-ear-flap-decr","r-ear-flap-incr","r-ear-lobe-decr","r-ear-lobe-incr",
        "r-ear-rot-backward","r-ear-rot-forward","r-ear-scale-decr",
        "r-ear-scale-depth-decr","r-ear-scale-depth-incr","r-ear-scale-incr",
        "r-ear-scale-vert-decr","r-ear-scale-vert-incr",
        "r-ear-shape-pointed","r-ear-shape-round","r-ear-shape-square","r-ear-shape-triangle",
        "r-ear-trans-backward","r-ear-trans-down","r-ear-trans-forward","r-ear-trans-up",
        "r-ear-wing-decr","r-ear-wing-incr",

        // Eyebrows
        "eyebrows-angle-down","eyebrows-angle-up",
        "eyebrows-trans-backward","eyebrows-trans-down",
        "eyebrows-trans-forward","eyebrows-trans-up",

        // Left Eye
        "l-eye-bag-decr","l-eye-bag-height-decr","l-eye-bag-height-incr",
        "l-eye-bag-in","l-eye-bag-incr","l-eye-bag-out",
        "l-eye-corner1-down","l-eye-corner1-up","l-eye-corner2-down","l-eye-corner2-up",
        "l-eye-epicanthus-in","l-eye-epicanthus-out",
        "l-eye-eyefold-angle-down","l-eye-eyefold-angle-up",
        "l-eye-eyefold-concave","l-eye-eyefold-convex",
        "l-eye-eyefold-down","l-eye-eyefold-up",
        "l-eye-height1-decr","l-eye-height1-incr",
        "l-eye-height2-decr","l-eye-height2-incr",
        "l-eye-height3-decr","l-eye-height3-incr",
        "l-eye-push1-in","l-eye-push1-out","l-eye-push2-in","l-eye-push2-out",
        "l-eye-scale-decr","l-eye-scale-incr",
        "l-eye-trans-down","l-eye-trans-in","l-eye-trans-out","l-eye-trans-up",

        // Right Eye
        "r-eye-bag-decr","r-eye-bag-height-decr","r-eye-bag-height-incr",
        "r-eye-bag-in","r-eye-bag-incr","r-eye-bag-out",
        "r-eye-corner1-down","r-eye-corner1-up","r-eye-corner2-down","r-eye-corner2-up",
        "r-eye-epicanthus-in","r-eye-epicanthus-out",
        "r-eye-eyefold-angle-down","r-eye-eyefold-angle-up",
        "r-eye-eyefold-concave","r-eye-eyefold-convex",
        "r-eye-eyefold-down","r-eye-eyefold-up",
        "r-eye-height1-decr","r-eye-height1-incr",
        "r-eye-height2-decr","r-eye-height2-incr",
        "r-eye-height3-decr","r-eye-height3-incr",
        "r-eye-push1-in","r-eye-push1-out","r-eye-push2-in","r-eye-push2-out",
        "r-eye-scale-decr","r-eye-scale-incr",
        "r-eye-trans-down","r-eye-trans-in","r-eye-trans-out","r-eye-trans-up",

        // Forehead
        "forehead-nubian-decr","forehead-nubian-incr",
        "forehead-scale-vert-decr","forehead-scale-vert-incr",
        "forehead-temple-decr","forehead-temple-incr",
        "forehead-trans-backward","forehead-trans-forward",

        // Head
        "head-age-decr","head-age-incr",
        "head-angle-in","head-angle-out",
        "head-back-scale-depth-decr","head-back-scale-depth-incr",
        "head-diamond","head-fat-decr","head-fat-incr",
        "head-invertedtriangular","head-oval","head-rectangular","head-round",
        "head-scale-depth-decr","head-scale-depth-incr",
        "head-scale-horiz-decr","head-scale-horiz-incr",
        "head-scale-vert-decr","head-scale-vert-incr",
        "head-square",
        "head-trans-backward","head-trans-down","head-trans-forward",
        "head-trans-in","head-trans-out","head-trans-up",
        "head-triangular",

        // Mouth
        "mouth-angles-down","mouth-angles-up",
        "mouth-cupidsbow-decr","mouth-cupidsbow-incr",
        "mouth-cupidsbow-width-decr","mouth-cupidsbow-width-incr",
        "mouth-dimples-in","mouth-dimples-out",
        "mouth-laugh-lines-in","mouth-laugh-lines-out",
        "mouth-lowerlip-ext-down","mouth-lowerlip-ext-up",
        "mouth-lowerlip-height-decr","mouth-lowerlip-height-incr",
        "mouth-lowerlip-middle-down","mouth-lowerlip-middle-up",
        "mouth-lowerlip-volume-decr","mouth-lowerlip-volume-incr",
        "mouth-lowerlip-width-decr","mouth-lowerlip-width-incr",
        "mouth-philtrum-volume-decr","mouth-philtrum-volume-incr",
        "mouth-scale-depth-decr","mouth-scale-depth-incr",
        "mouth-scale-horiz-decr","mouth-scale-horiz-incr",
        "mouth-scale-vert-decr","mouth-scale-vert-incr",
        "mouth-trans-backward","mouth-trans-down","mouth-trans-forward",
        "mouth-trans-in","mouth-trans-out","mouth-trans-up",
        "mouth-upperlip-ext-down","mouth-upperlip-ext-up",
        "mouth-upperlip-height-decr","mouth-upperlip-height-incr",
        "mouth-upperlip-middle-down","mouth-upperlip-middle-up",
        "mouth-upperlip-volume-decr","mouth-upperlip-volume-incr",
        "mouth-upperlip-width-decr","mouth-upperlip-width-incr",

        // Nose
        "nose-base-down","nose-base-up",
        "nose-compression-compress","nose-compression-uncompress",
        "nose-curve-concave","nose-curve-convex",
        "nose-flaring-decr","nose-flaring-incr",
        "nose-greek-decr","nose-greek-incr",
        "nose-hump-decr","nose-hump-incr",
        "nose-nostrils-angle-down","nose-nostrils-angle-up",
        "nose-nostrils-width-decr","nose-nostrils-width-incr",
        "nose-point-down","nose-point-up",
        "nose-point-width-decr","nose-point-width-incr",
        "nose-scale-depth-decr","nose-scale-depth-incr",
        "nose-scale-horiz-decr","nose-scale-horiz-incr",
        "nose-scale-vert-decr","nose-scale-vert-incr",
        "nose-septumangle-decr","nose-septumangle-incr",
        "nose-trans-backward","nose-trans-down","nose-trans-forward",
        "nose-trans-in","nose-trans-out","nose-trans-up",
        "nose-volume-decr","nose-volume-incr",
        "nose-width1-decr","nose-width1-incr",
        "nose-width2-decr","nose-width2-incr",
        "nose-width3-decr","nose-width3-incr"
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
        "6. Include ONLY relevant morph targets\n"
        "7. LEFT and RIGHT features MUST be symmetrical unless explicitly specified\n"
        "8. If a feature is not mentioned, DO NOT include it\n"
        "10. Values above 0.5 are EXTREME and should be avoided\n"
        "11. Prefer values between 0.1 and 0.4 for natural human appearance\n"
        "12. Related features should be adjusted together for natural results\n"
        "   - Example: increasing cheekbones may also slightly increase cheek volume\n"
        "   - Example: increasing eye size may slightly raise eyebrows\n"
        "   - Keep secondary adjustments weaker than primary ones\n"

        "{\n"
        "  \"l-ear-scale-vert-incr\": 0.9,\n"
        "  \"r-ear-scale-vert-incr\": 0.9\n"
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