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
        // Head shape
        "head-diamond", "head-invertedtriangular", "head-oval", "head-rectangular",
        "head-round", "head-square", "head-triangular",
        "head-fat-decr", "head-fat-incr",
        "head-age-decr", "head-age-incr",
        "head-scale-horiz-decr", "head-scale-horiz-incr",
        "head-scale-vert-decr", "head-scale-vert-incr",

        // Forehead
        "forehead-nubian-decr", "forehead-nubian-incr",
        "forehead-scale-vert-decr", "forehead-scale-vert-incr",
        "forehead-temple-decr", "forehead-temple-incr",

        // Eyes
        "l-eye-scale-decr", "l-eye-scale-incr",
        "r-eye-scale-decr", "r-eye-scale-incr",
        "l-eye-height1-decr", "l-eye-height1-incr",
        "r-eye-height1-decr", "r-eye-height1-incr",
        "l-eye-corner1-down", "l-eye-corner1-up",
        "r-eye-corner1-down", "r-eye-corner1-up",
        "l-eye-corner2-down", "l-eye-corner2-up",
        "r-eye-corner2-down", "r-eye-corner2-up",
        "l-eye-push1-in", "l-eye-push1-out",
        "r-eye-push1-in", "r-eye-push1-out",

        // Eyebrows
        "eyebrows-angle-down", "eyebrows-angle-up",
        "eyebrows-trans-down", "eyebrows-trans-up",

        // Nose
        "nose-scale-horiz-decr", "nose-scale-horiz-incr",
        "nose-scale-vert-decr", "nose-scale-vert-incr",
        "nose-scale-depth-decr", "nose-scale-depth-incr",
        "nose-width1-decr", "nose-width1-incr",
        "nose-width2-decr", "nose-width2-incr",
        "nose-curve-concave", "nose-curve-convex",
        "nose-hump-decr", "nose-hump-incr",
        "nose-point-down", "nose-point-up",
        "nose-flaring-decr", "nose-flaring-incr",

        // Mouth
        "mouth-scale-horiz-decr", "mouth-scale-horiz-incr",
        "mouth-scale-vert-decr", "mouth-scale-vert-incr",
        "mouth-upperlip-volume-decr", "mouth-upperlip-volume-incr",
        "mouth-lowerlip-volume-decr", "mouth-lowerlip-volume-incr",
        "mouth-angles-down", "mouth-angles-up",
        "mouth-trans-down", "mouth-trans-up",

        // Chin / Jaw
        "chin-width-decr", "chin-width-incr",
        "chin-prominent-decr", "chin-prominent-incr",
        "chin-height-decr", "chin-height-incr",
        "chin-bones-decr", "chin-bones-incr",
        "chin-cleft-decr", "chin-cleft-incr",
        "chin-jaw-drop-decr", "chin-jaw-drop-incr",
        "chin-prognathism-decr", "chin-prognathism-incr",

        // Cheeks
        "l-cheek-volume-decr", "l-cheek-volume-incr",
        "r-cheek-volume-decr", "r-cheek-volume-incr",
        "l-cheek-bones-decr", "l-cheek-bones-incr",
        "r-cheek-bones-decr", "r-cheek-bones-incr",

        // Ears
        "l-ear-scale-decr", "l-ear-scale-incr",
        "r-ear-scale-decr", "r-ear-scale-incr",
        "l-ear-shape-pointed", "r-ear-shape-pointed",

        // Neck
        "neck-scale-vert-decr", "neck-scale-vert-incr",
        "neck-scale-horiz-decr", "neck-scale-horiz-incr",
        "measure-neck-circ-decr", "measure-neck-circ-incr",
        "neck-double-decr", "neck-double-incr",

        // Height (torso + legs)
        "torso-scale-vert-decr", "torso-scale-vert-incr",
        "upperlegs-height-decr", "upperlegs-height-incr",
        "lowerlegs-height-decr", "lowerlegs-height-incr",

        // Torso / Build
        "torso-scale-horiz-decr", "torso-scale-horiz-incr",
        "torso-scale-depth-decr", "torso-scale-depth-incr",
        "torso-vshape-decr", "torso-vshape-incr",
        "measure-shoulder-dist-decr", "measure-shoulder-dist-incr",
        "measure-waist-circ-decr", "measure-waist-circ-incr",
        "measure-hips-circ-decr", "measure-hips-circ-incr",
        "measure-bust-circ-decr", "measure-bust-circ-incr",

        // Muscle
        "torso-muscle-pectoral-decr", "torso-muscle-pectoral-incr",
        "torso-muscle-dorsi-decr", "torso-muscle-dorsi-incr",
        "l-upperarm-muscle-decr", "l-upperarm-muscle-incr",
        "r-upperarm-muscle-decr", "r-upperarm-muscle-incr",
        "l-upperarm-shoulder-muscle-decr", "l-upperarm-shoulder-muscle-incr",
        "r-upperarm-shoulder-muscle-decr", "r-upperarm-shoulder-muscle-incr",
        "l-lowerarm-muscle-decr", "l-lowerarm-muscle-incr",
        "r-lowerarm-muscle-decr", "r-lowerarm-muscle-incr",
        "l-upperleg-muscle-decr", "l-upperleg-muscle-incr",
        "r-upperleg-muscle-decr", "r-upperleg-muscle-incr",
        "l-lowerleg-muscle-decr", "l-lowerleg-muscle-incr",
        "r-lowerleg-muscle-decr", "r-lowerleg-muscle-incr",

        // Fat
        "l-upperarm-fat-decr", "l-upperarm-fat-incr",
        "r-upperarm-fat-decr", "r-upperarm-fat-incr",
        "l-upperleg-fat-decr", "l-upperleg-fat-incr",
        "r-upperleg-fat-decr", "r-upperleg-fat-incr",
        "l-lowerleg-fat-decr", "l-lowerleg-fat-incr",
        "r-lowerleg-fat-decr", "r-lowerleg-fat-incr",

        // Stomach
        "stomach-tone-decr", "stomach-tone-incr",
        "stomach-pregnant-decr", "stomach-pregnant-incr",

        // Buttocks
        "buttocks-volume-decr", "buttocks-volume-incr",

        // Hips
        "hip-scale-horiz-decr", "hip-scale-horiz-incr",
        "hip-scale-depth-decr", "hip-scale-depth-incr",

        // Breast
        "breast-volume-vert-down", "breast-volume-vert-up",
        "breast-dist-decr", "breast-dist-incr",

        // Pelvis
        "pelvis-tone-decr", "pelvis-tone-incr"
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
        "2. Do NOT include explanations or markdown\n"
        "3. Use ONLY morph names from the list above\n"
        "4. Values between 0.0 and 1.0\n"
        "5. Only include parameters relevant to the description\n"
        "6. For symmetric features (arms, legs, eyes, ears, cheeks), ALWAYS set both l- and r- to the same value\n"
        "7. NEVER combine two head shapes together - pick ONLY ONE\n"
        "8. If no head shape is mentioned, do NOT include head shape parameters\n"
        "9. NEVER use both -incr and -decr of the same parameter together\n\n"
        "HEAD SHAPE MAX VALUES (use these exact values):\n"
        "- head-round: 0.132057, head-square: 0.1, head-oval: 0.13\n"
        "- head-rectangular: 0.090274, head-triangular: 0.074856\n"
        "- head-invertedtriangular: 0.136986, head-diamond: 0.101898\n\n"
        "HEAD FAT RULES:\n"
        "- head-fat-incr: max 0.2 for chubby/fat face\n"
        "- head-fat-decr: max 0.1 for thin/slim face\n"
        "- NEVER use both together\n\n"
        "HEIGHT RULES:\n"
        "- Tall: use torso-scale-vert-incr, upperlegs-height-incr, lowerlegs-height-incr (0.1-0.3)\n"
        "- Short: use torso-scale-vert-decr, upperlegs-height-decr, lowerlegs-height-decr (0.1-0.3)\n\n"
        "BODY BUILD RULES:\n"
        "- Muscular: use muscle-incr params (0.2-0.5), measure-shoulder-dist-incr, torso-vshape-incr\n"
        "- Fat/heavy: use fat-incr params, stomach-tone-decr, measure-waist-circ-incr, head-fat-incr (0.2-0.5)\n"
        "- Thin/slim: use fat-decr, stomach-tone-incr (0.1-0.3)\n"
        "- Broad shoulders: measure-shoulder-dist-incr (0.2-0.4)\n"
        "- Wide hips: measure-hips-circ-incr, hip-scale-horiz-incr (0.2-0.4)\n"
        "- Keep values moderate to avoid deformation\n\n"
        "AGE RULES:\n"
        "- Old/elderly: head-age-incr (0.3-0.5)\n"
        "- Young/baby-faced: head-age-decr (0.2-0.4)\n\n"
        "EXAMPLE for 'tall muscular man with square jaw':\n"
        "{\n"
        "  \"torso-scale-vert-incr\": 0.2,\n"
        "  \"upperlegs-height-incr\": 0.2,\n"
        "  \"lowerlegs-height-incr\": 0.2,\n"
        "  \"torso-muscle-pectoral-incr\": 0.3,\n"
        "  \"torso-muscle-dorsi-incr\": 0.25,\n"
        "  \"l-upperarm-muscle-incr\": 0.3,\n"
        "  \"r-upperarm-muscle-incr\": 0.3,\n"
        "  \"l-upperleg-muscle-incr\": 0.25,\n"
        "  \"r-upperleg-muscle-incr\": 0.25,\n"
        "  \"measure-shoulder-dist-incr\": 0.3,\n"
        "  \"torso-vshape-incr\": 0.2,\n"
        "  \"chin-width-incr\": 0.15,\n"
        "  \"chin-bones-incr\": 0.1,\n"
        "  \"head-square\": 0.1\n"
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

    TMap<FString, float> AppliedValues;

    for (auto& Pair : JsonObject->Values)
    {
        if (Pair.Value->Type == EJson::Number)
        {
            double Value = Pair.Value->AsNumber();
            SetMorphTargetValue(Pair.Key, (float)Value);
            AppliedValues.Add(Pair.Key, (float)Value);
        }
    }

    OnAppearanceApplied.Broadcast();
}

void ACharacterDemoActor::ResetAllMorphTargets()
{
    TArray<FString> AllMorphTargets = {
        // Head shape
        "head-diamond", "head-invertedtriangular", "head-oval", "head-rectangular",
        "head-round", "head-square", "head-triangular",
        "head-fat-decr", "head-fat-incr",
        "head-age-decr", "head-age-incr",
        "head-scale-horiz-decr", "head-scale-horiz-incr",
        "head-scale-vert-decr", "head-scale-vert-incr",

        // Forehead
        "forehead-nubian-decr", "forehead-nubian-incr",
        "forehead-scale-vert-decr", "forehead-scale-vert-incr",
        "forehead-temple-decr", "forehead-temple-incr",

        // Eyes
        "l-eye-scale-decr", "l-eye-scale-incr",
        "r-eye-scale-decr", "r-eye-scale-incr",
        "l-eye-height1-decr", "l-eye-height1-incr",
        "r-eye-height1-decr", "r-eye-height1-incr",
        "l-eye-corner1-down", "l-eye-corner1-up",
        "r-eye-corner1-down", "r-eye-corner1-up",
        "l-eye-corner2-down", "l-eye-corner2-up",
        "r-eye-corner2-down", "r-eye-corner2-up",
        "l-eye-push1-in", "l-eye-push1-out",
        "r-eye-push1-in", "r-eye-push1-out",

        // Eyebrows
        "eyebrows-angle-down", "eyebrows-angle-up",
        "eyebrows-trans-down", "eyebrows-trans-up",

        // Nose
        "nose-scale-horiz-decr", "nose-scale-horiz-incr",
        "nose-scale-vert-decr", "nose-scale-vert-incr",
        "nose-scale-depth-decr", "nose-scale-depth-incr",
        "nose-width1-decr", "nose-width1-incr",
        "nose-width2-decr", "nose-width2-incr",
        "nose-curve-concave", "nose-curve-convex",
        "nose-hump-decr", "nose-hump-incr",
        "nose-point-down", "nose-point-up",
        "nose-flaring-decr", "nose-flaring-incr",

        // Mouth
        "mouth-scale-horiz-decr", "mouth-scale-horiz-incr",
        "mouth-scale-vert-decr", "mouth-scale-vert-incr",
        "mouth-upperlip-volume-decr", "mouth-upperlip-volume-incr",
        "mouth-lowerlip-volume-decr", "mouth-lowerlip-volume-incr",
        "mouth-angles-down", "mouth-angles-up",
        "mouth-trans-down", "mouth-trans-up",

        // Chin / Jaw
        "chin-width-decr", "chin-width-incr",
        "chin-prominent-decr", "chin-prominent-incr",
        "chin-height-decr", "chin-height-incr",
        "chin-bones-decr", "chin-bones-incr",
        "chin-cleft-decr", "chin-cleft-incr",
        "chin-jaw-drop-decr", "chin-jaw-drop-incr",
        "chin-prognathism-decr", "chin-prognathism-incr",

        // Cheeks
        "l-cheek-volume-decr", "l-cheek-volume-incr",
        "r-cheek-volume-decr", "r-cheek-volume-incr",
        "l-cheek-bones-decr", "l-cheek-bones-incr",
        "r-cheek-bones-decr", "r-cheek-bones-incr",

        // Ears
        "l-ear-scale-decr", "l-ear-scale-incr",
        "r-ear-scale-decr", "r-ear-scale-incr",
        "l-ear-shape-pointed", "r-ear-shape-pointed",

        // Neck
        "neck-scale-vert-decr", "neck-scale-vert-incr",
        "neck-scale-horiz-decr", "neck-scale-horiz-incr",
        "measure-neck-circ-decr", "measure-neck-circ-incr",
        "neck-double-decr", "neck-double-incr",

        // Height (torso + legs)
        "torso-scale-vert-decr", "torso-scale-vert-incr",
        "upperlegs-height-decr", "upperlegs-height-incr",
        "lowerlegs-height-decr", "lowerlegs-height-incr",

        // Torso / Build
        "torso-scale-horiz-decr", "torso-scale-horiz-incr",
        "torso-scale-depth-decr", "torso-scale-depth-incr",
        "torso-vshape-decr", "torso-vshape-incr",
        "measure-shoulder-dist-decr", "measure-shoulder-dist-incr",
        "measure-waist-circ-decr", "measure-waist-circ-incr",
        "measure-hips-circ-decr", "measure-hips-circ-incr",
        "measure-bust-circ-decr", "measure-bust-circ-incr",

        // Muscle
        "torso-muscle-pectoral-decr", "torso-muscle-pectoral-incr",
        "torso-muscle-dorsi-decr", "torso-muscle-dorsi-incr",
        "l-upperarm-muscle-decr", "l-upperarm-muscle-incr",
        "r-upperarm-muscle-decr", "r-upperarm-muscle-incr",
        "l-upperarm-shoulder-muscle-decr", "l-upperarm-shoulder-muscle-incr",
        "r-upperarm-shoulder-muscle-decr", "r-upperarm-shoulder-muscle-incr",
        "l-lowerarm-muscle-decr", "l-lowerarm-muscle-incr",
        "r-lowerarm-muscle-decr", "r-lowerarm-muscle-incr",
        "l-upperleg-muscle-decr", "l-upperleg-muscle-incr",
        "r-upperleg-muscle-decr", "r-upperleg-muscle-incr",
        "l-lowerleg-muscle-decr", "l-lowerleg-muscle-incr",
        "r-lowerleg-muscle-decr", "r-lowerleg-muscle-incr",

        // Fat
        "l-upperarm-fat-decr", "l-upperarm-fat-incr",
        "r-upperarm-fat-decr", "r-upperarm-fat-incr",
        "l-upperleg-fat-decr", "l-upperleg-fat-incr",
        "r-upperleg-fat-decr", "r-upperleg-fat-incr",
        "l-lowerleg-fat-decr", "l-lowerleg-fat-incr",
        "r-lowerleg-fat-decr", "r-lowerleg-fat-incr",

        // Stomach
        "stomach-tone-decr", "stomach-tone-incr",
        "stomach-pregnant-decr", "stomach-pregnant-incr",

        // Buttocks
        "buttocks-volume-decr", "buttocks-volume-incr",

        // Hips
        "hip-scale-horiz-decr", "hip-scale-horiz-incr",
        "hip-scale-depth-decr", "hip-scale-depth-incr",

        // Breast
        "breast-volume-vert-down", "breast-volume-vert-up",
        "breast-dist-decr", "breast-dist-incr",

        // Pelvis
        "pelvis-tone-decr", "pelvis-tone-incr"
    };

    for (const FString& MorphName : AllMorphTargets)
    {
        SetMorphTargetValue(MorphName, 0.0f);
    }
}

float ACharacterDemoActor::GetMorphTargetValue(FString MorphTargetName)
{
    if (!TargetMesh) return 0.0f;
    return TargetMesh->GetMorphTarget(FName(*MorphTargetName));
}
