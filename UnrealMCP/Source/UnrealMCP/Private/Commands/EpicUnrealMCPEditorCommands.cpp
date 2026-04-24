#include "Commands/EpicUnrealMCPEditorCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "UObject/TopLevelAssetPath.h"
#include "Commands/EpicUnrealMCPBlueprintCommands.h"
#include "Misc/OutputDevice.h"
#include "Misc/Base64.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FEpicUnrealMCPEditorCommands::FEpicUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor"))
    {
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Generalized asset search
    else if (CommandType == TEXT("search_assets"))
    {
        return HandleSearchAssets(Params);
    }
    // Escape-hatch / AI Assistant bridge commands
    else if (CommandType == TEXT("execute_console_command"))
    {
        return HandleExecuteConsoleCommand(Params);
    }
    else if (CommandType == TEXT("execute_editor_python"))
    {
        return HandleExecuteEditorPython(Params);
    }
    else if (CommandType == TEXT("ask_ai_assistant"))
    {
        return HandleAskAIAssistant(Params);
    }
    else if (CommandType == TEXT("list_editor_subsystems"))
    {
        return HandleListEditorSubsystems(Params);
    }
    
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        AStaticMeshActor* NewMeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
        if (NewMeshActor)
        {
            // Check for an optional static_mesh parameter to assign a mesh
            FString MeshPath;
            if (Params->TryGetStringField(TEXT("static_mesh"), MeshPath))
            {
                UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
                if (Mesh)
                {
                    NewMeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Could not find static mesh at path: %s"), *MeshPath);
                }
            }
        }
        NewActor = NewMeshActor;
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FEpicUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FEpicUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // This function will now correctly call the implementation in BlueprintCommands
    FEpicUnrealMCPBlueprintCommands BlueprintCommands;
    return BlueprintCommands.HandleCommand(TEXT("spawn_blueprint_actor"), Params);
}

// -----------------------------------------------------------------------------
// HandleSearchAssets
//
// Generalized asset search built on FARFilter / FTopLevelAssetPath — same
// machinery HandleGetAvailableMaterials already uses for materials, but usable
// for any asset class (animations, skeletons, meshes, textures, blueprints...).
//
// Params:
//   class_names (array of strings, required)
//       Asset class short names (e.g. "AnimSequence", "AnimMontage",
//       "BlendSpace", "AnimBlueprint", "Skeleton", "SkeletalMesh",
//       "StaticMesh", "Blueprint", "Material", "Texture2D", "SoundBase").
//       You may also pass fully-qualified paths like "/Script/Engine.AnimSequence"
//       and those are forwarded unchanged.
//   paths (array of strings, optional)   — default ["/Game/"]
//   recursive (bool, optional)           — default true  (recursive folder search)
//   recursive_classes (bool, optional)   — default true  (include subclasses)
//   include_engine (bool, optional)      — default false (also search /Engine/)
//   max_results (int, optional)          — default 1000, capped at 5000
// -----------------------------------------------------------------------------
namespace
{
    // Short-name -> FTopLevelAssetPath for classes that ship with the engine.
    // Passthrough for any "/Script/..." path the caller already resolved.
    static FTopLevelAssetPath ResolveAssetClassPath(const FString& InName)
    {
        const FString Name = InName.TrimStartAndEnd();

        // Pre-qualified path: "/Script/Module.ClassName"
        if (Name.StartsWith(TEXT("/Script/")) && Name.Contains(TEXT(".")))
        {
            return FTopLevelAssetPath(Name);
        }

        struct FKnownClass { const TCHAR* ShortName; const TCHAR* PackageName; const TCHAR* ClassName; };
        static const FKnownClass Known[] = {
            // Animation
            { TEXT("AnimSequence"),         TEXT("/Script/Engine"),    TEXT("AnimSequence") },
            { TEXT("AnimMontage"),          TEXT("/Script/Engine"),    TEXT("AnimMontage") },
            { TEXT("BlendSpace"),           TEXT("/Script/Engine"),    TEXT("BlendSpace") },
            { TEXT("BlendSpace1D"),         TEXT("/Script/Engine"),    TEXT("BlendSpace1D") },
            { TEXT("AimOffsetBlendSpace"),  TEXT("/Script/Engine"),    TEXT("AimOffsetBlendSpace") },
            { TEXT("AimOffsetBlendSpace1D"),TEXT("/Script/Engine"),    TEXT("AimOffsetBlendSpace1D") },
            { TEXT("AnimBlueprint"),        TEXT("/Script/Engine"),    TEXT("AnimBlueprint") },
            { TEXT("AnimComposite"),        TEXT("/Script/Engine"),    TEXT("AnimComposite") },
            { TEXT("Skeleton"),             TEXT("/Script/Engine"),    TEXT("Skeleton") },
            { TEXT("SkeletalMesh"),         TEXT("/Script/Engine"),    TEXT("SkeletalMesh") },
            { TEXT("PhysicsAsset"),         TEXT("/Script/Engine"),    TEXT("PhysicsAsset") },
            // Meshes
            { TEXT("StaticMesh"),           TEXT("/Script/Engine"),    TEXT("StaticMesh") },
            // Materials
            { TEXT("Material"),             TEXT("/Script/Engine"),    TEXT("Material") },
            { TEXT("MaterialInterface"),    TEXT("/Script/Engine"),    TEXT("MaterialInterface") },
            { TEXT("MaterialInstance"),     TEXT("/Script/Engine"),    TEXT("MaterialInstance") },
            { TEXT("MaterialInstanceConstant"), TEXT("/Script/Engine"), TEXT("MaterialInstanceConstant") },
            // Textures
            { TEXT("Texture"),              TEXT("/Script/Engine"),    TEXT("Texture") },
            { TEXT("Texture2D"),            TEXT("/Script/Engine"),    TEXT("Texture2D") },
            { TEXT("TextureCube"),          TEXT("/Script/Engine"),    TEXT("TextureCube") },
            // Blueprints & classes
            { TEXT("Blueprint"),            TEXT("/Script/Engine"),    TEXT("Blueprint") },
            { TEXT("UserDefinedEnum"),      TEXT("/Script/Engine"),    TEXT("UserDefinedEnum") },
            { TEXT("UserDefinedStruct"),    TEXT("/Script/Engine"),    TEXT("UserDefinedStruct") },
            { TEXT("DataTable"),            TEXT("/Script/Engine"),    TEXT("DataTable") },
            { TEXT("DataAsset"),            TEXT("/Script/Engine"),    TEXT("DataAsset") },
            // Audio
            { TEXT("SoundBase"),            TEXT("/Script/Engine"),    TEXT("SoundBase") },
            { TEXT("SoundWave"),            TEXT("/Script/Engine"),    TEXT("SoundWave") },
            { TEXT("SoundCue"),             TEXT("/Script/Engine"),    TEXT("SoundCue") },
            // Niagara (if plugin enabled)
            { TEXT("NiagaraSystem"),        TEXT("/Script/Niagara"),   TEXT("NiagaraSystem") },
        };

        for (const FKnownClass& K : Known)
        {
            if (Name.Equals(K.ShortName, ESearchCase::IgnoreCase))
            {
                return FTopLevelAssetPath(FName(K.PackageName), FName(K.ClassName));
            }
        }

        // Last-resort: try reflection for any engine-loaded UClass with this short name.
        if (UClass* Found = FindFirstObject<UClass>(*Name, EFindFirstObjectOptions::NativeFirst))
        {
            return Found->GetClassPathName();
        }

        return FTopLevelAssetPath();
    }
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSearchAssets(const TSharedPtr<FJsonObject>& Params)
{
    // --- Parse class_names (required) ---
    const TArray<TSharedPtr<FJsonValue>>* ClassNamesJson = nullptr;
    if (!Params->TryGetArrayField(TEXT("class_names"), ClassNamesJson) || ClassNamesJson->Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Missing required parameter 'class_names' (non-empty array of asset class short names or /Script/Module.Class paths)"));
    }

    TArray<FString> RequestedClassNames;
    TArray<FTopLevelAssetPath> ResolvedClassPaths;
    TArray<FString> UnresolvedClassNames;
    for (const TSharedPtr<FJsonValue>& Value : *ClassNamesJson)
    {
        if (!Value.IsValid() || Value->Type != EJson::String)
        {
            continue;
        }
        const FString Short = Value->AsString();
        RequestedClassNames.Add(Short);

        const FTopLevelAssetPath Path = ResolveAssetClassPath(Short);
        if (Path.IsValid())
        {
            ResolvedClassPaths.AddUnique(Path);
        }
        else
        {
            UnresolvedClassNames.Add(Short);
        }
    }

    if (ResolvedClassPaths.Num() == 0)
    {
        TSharedPtr<FJsonObject> Err = FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("No class_names could be resolved to an asset class. Pass either a known short name (e.g. 'AnimSequence') or a fully-qualified path (e.g. '/Script/Engine.AnimSequence')."));
        TArray<TSharedPtr<FJsonValue>> UnresolvedJson;
        for (const FString& Unresolved : UnresolvedClassNames)
        {
            UnresolvedJson.Add(MakeShared<FJsonValueString>(Unresolved));
        }
        Err->SetArrayField(TEXT("unresolved_class_names"), UnresolvedJson);
        return Err;
    }

    // --- Parse paths (optional; default ["/Game/"]) ---
    TArray<FString> SearchPaths;
    const TArray<TSharedPtr<FJsonValue>>* PathsJson = nullptr;
    if (Params->TryGetArrayField(TEXT("paths"), PathsJson))
    {
        for (const TSharedPtr<FJsonValue>& Value : *PathsJson)
        {
            if (Value.IsValid() && Value->Type == EJson::String)
            {
                FString P = Value->AsString().TrimStartAndEnd();
                if (P.IsEmpty()) continue;
                if (!P.StartsWith(TEXT("/"))) P = TEXT("/") + P;
                // Trailing slash not required by AssetRegistry but keep paths consistent
                SearchPaths.AddUnique(P);
            }
        }
    }
    if (SearchPaths.Num() == 0)
    {
        SearchPaths.Add(TEXT("/Game/"));
    }

    bool bIncludeEngine = false;
    Params->TryGetBoolField(TEXT("include_engine"), bIncludeEngine);
    if (bIncludeEngine)
    {
        SearchPaths.AddUnique(TEXT("/Engine/"));
    }

    bool bRecursivePaths = true;
    Params->TryGetBoolField(TEXT("recursive"), bRecursivePaths);

    bool bRecursiveClasses = true;
    Params->TryGetBoolField(TEXT("recursive_classes"), bRecursiveClasses);

    int32 MaxResults = 1000;
    if (Params->HasField(TEXT("max_results")))
    {
        double Raw = 0.0;
        if (Params->TryGetNumberField(TEXT("max_results"), Raw))
        {
            MaxResults = FMath::Clamp((int32)Raw, 1, 5000);
        }
    }

    // --- Build filter & query AssetRegistry ---
    FAssetRegistryModule& AssetRegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    for (const FTopLevelAssetPath& ClassPath : ResolvedClassPaths)
    {
        Filter.ClassPaths.Add(ClassPath);
    }
    for (const FString& P : SearchPaths)
    {
        Filter.PackagePaths.Add(FName(*P));
    }
    Filter.bRecursivePaths = bRecursivePaths;
    Filter.bRecursiveClasses = bRecursiveClasses;

    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);

    // --- Format response ---
    const int32 TotalFound = AssetDataArray.Num();
    const int32 ReturnCount = FMath::Min(TotalFound, MaxResults);

    TArray<TSharedPtr<FJsonValue>> AssetArray;
    AssetArray.Reserve(ReturnCount);
    for (int32 i = 0; i < ReturnCount; ++i)
    {
        const FAssetData& AssetData = AssetDataArray[i];
        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        AssetObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());
        AssetObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.ToString());
        AssetArray.Add(MakeShared<FJsonValueObject>(AssetObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetArrayField(TEXT("assets"), AssetArray);
    Result->SetNumberField(TEXT("count"), ReturnCount);
    Result->SetNumberField(TEXT("total_found"), TotalFound);
    Result->SetBoolField(TEXT("truncated"), TotalFound > ReturnCount);

    // Echo back what was searched so the caller can diagnose empty results.
    TArray<TSharedPtr<FJsonValue>> SearchPathsJson;
    for (const FString& P : SearchPaths) { SearchPathsJson.Add(MakeShared<FJsonValueString>(P)); }
    Result->SetArrayField(TEXT("searched_paths"), SearchPathsJson);

    TArray<TSharedPtr<FJsonValue>> ResolvedClassJson;
    for (const FTopLevelAssetPath& ClassPath : ResolvedClassPaths)
    {
        ResolvedClassJson.Add(MakeShared<FJsonValueString>(ClassPath.ToString()));
    }
    Result->SetArrayField(TEXT("resolved_class_paths"), ResolvedClassJson);

    if (UnresolvedClassNames.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> UnresolvedJson;
        for (const FString& U : UnresolvedClassNames)
        {
            UnresolvedJson.Add(MakeShared<FJsonValueString>(U));
        }
        Result->SetArrayField(TEXT("unresolved_class_names"), UnresolvedJson);
    }

    return Result;
}


// ============================================================================
// Escape-hatch commands: console, editor Python, and AI Assistant bridge
// ============================================================================
//
// These commands are intentionally generic so that MCP clients can reach
// anything the Unreal Editor exposes — including first- and third-party
// "AI Assistant" style plugins (e.g. EditorAIAssistantSubsystem and similar
// community plugins). Rather than hard-coding a specific plugin's API, we
// expose:
//
//   * execute_console_command : run any Unreal console command via GEditor->Exec
//   * execute_editor_python   : run Python in-editor by routing through the
//                               built-in `py` console command (requires the
//                               "Python Editor Script Plugin" to be enabled)
//   * ask_ai_assistant        : convenience wrapper that builds a small Python
//                               snippet which discovers an AI-Assistant style
//                               editor subsystem, sends it a message, and
//                               returns the subsystem's response string
//   * list_editor_subsystems  : discovery helper that lists loaded editor
//                               subsystems so an agent can find the right one
//
// Output from console commands is captured through a small FOutputDevice
// shim so we can send it back in the JSON response.

class FMCPStringOutputDevice : public FOutputDevice
{
public:
    FString Output;

    virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
    {
        Output.Append(V);
        Output.AppendChar(TEXT('\n'));
    }

    // Deliberately single-threaded. All callers run on the game thread
    // (commands are dispatched via AsyncTask(ENamedThreads::GameThread, ...)
    // in EpicUnrealMCPBridge), so we don't need multi-thread safety here
    // and claiming it would be misleading (see FOutputDevice docs).
    virtual bool CanBeUsedOnAnyThread() const override { return false; }
    virtual bool CanBeUsedOnMultipleThreads() const override { return false; }
};

static bool MCP_RunExecCommand(const FString& Command, FString& OutCapturedText, FString& OutError)
{
    FMCPStringOutputDevice Ar;

    UWorld* World = nullptr;
#if WITH_EDITOR
    if (GEditor)
    {
        World = GEditor->GetEditorWorldContext().World();
    }
#endif
    if (!World)
    {
        World = GWorld;
    }

    bool bHandled = false;
#if WITH_EDITOR
    if (GEditor)
    {
        bHandled = GEditor->Exec(World, *Command, Ar);
    }
    else
#endif
    if (GEngine)
    {
        bHandled = GEngine->Exec(World, *Command, Ar);
    }
    else
    {
        OutError = TEXT("No GEditor/GEngine available to execute console command");
        return false;
    }

    OutCapturedText = Ar.Output;
    if (!bHandled)
    {
        // Not fatal — some commands are handled by CVars and return false.
        OutError = TEXT("Command was not explicitly handled (this is usually fine for CVars).");
    }
    return true;
}

// Wrap an arbitrary (possibly multi-line) Python snippet in a single-line
// `py` console command. Some engine versions split multi-line Exec strings at
// the first newline, so we base64-encode the source and exec() it from a
// one-liner bootstrap. This is much more robust across UE versions.
static FString MCP_WrapPythonAsSingleLineExec(const FString& PythonSource)
{
    const FString Encoded = FBase64::Encode(PythonSource);
    return FString::Printf(
        TEXT("py import base64 as _b64; exec(compile(_b64.b64decode('%s').decode('utf-8'), '<mcp>', 'exec'))"),
        *Encoded);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleExecuteConsoleCommand(const TSharedPtr<FJsonObject>& Params)
{
    FString Command;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("command"), Command) || Command.IsEmpty())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required string parameter 'command'"));
    }

    FString Captured;
    FString Warning;
    if (!MCP_RunExecCommand(Command, Captured, Warning))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(Warning);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("command"), Command);
    Result->SetStringField(TEXT("output"), Captured);
    if (!Warning.IsEmpty())
    {
        Result->SetStringField(TEXT("warning"), Warning);
    }
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleExecuteEditorPython(const TSharedPtr<FJsonObject>& Params)
{
    FString Code;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("code"), Code) || Code.IsEmpty())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required string parameter 'code' (Python source to execute)"));
    }

    // Route through the Unreal "py" console command so we don't have to take a
    // direct dependency on the PythonScriptPlugin module. This works whenever
    // the "Python Editor Script Plugin" is enabled in the project (which is
    // the default for this repo's FlopperamUnrealMCP project).
    //
    // We base64-wrap the code into a single-line `py` bootstrap so that
    // multi-line / special-character user code survives the `FExec` pipeline
    // unchanged on every supported engine version.
    const FString Command = MCP_WrapPythonAsSingleLineExec(Code);

    FString Captured;
    FString Warning;
    if (!MCP_RunExecCommand(Command, Captured, Warning))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(Warning);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("output"), Captured);
    if (!Warning.IsEmpty())
    {
        Result->SetStringField(TEXT("warning"), Warning);
    }
    // Make the hint obvious if the plugin isn't enabled.
    if (Captured.Contains(TEXT("Command not recognized")) ||
        Captured.Contains(TEXT("Unknown command")))
    {
        Result->SetStringField(TEXT("hint"),
            TEXT("The 'py' console command was not recognized. Enable the 'Python Editor Script Plugin' in Edit > Plugins and restart the editor."));
    }
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleAskAIAssistant(const TSharedPtr<FJsonObject>& Params)
{
    FString Message;
    if (!Params.IsValid() || !Params->TryGetStringField(TEXT("message"), Message) || Message.IsEmpty())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required string parameter 'message'"));
    }

    FString SubsystemHint;
    Params->TryGetStringField(TEXT("subsystem"), SubsystemHint);
    FString MethodHint;
    Params->TryGetStringField(TEXT("method"), MethodHint);

    // Base64-encode free-form strings so we never have to worry about escaping
    // quotes, backslashes, or newlines across the C++ → console → Python boundary.
    const FString MessageB64   = FBase64::Encode(Message);
    const FString SubsystemB64 = FBase64::Encode(SubsystemHint);
    const FString MethodB64    = FBase64::Encode(MethodHint);

    // This Python snippet tries a number of well-known subsystem/method names
    // used by Epic's AI Assistant plugin and by popular community plugins
    // (e.g. UE5AgentPython). It returns a JSON-safe payload so the MCP client
    // can parse a structured response.
    const FString Py = FString::Printf(TEXT(
        "import unreal, json, base64\n"
        "msg = base64.b64decode('%s').decode('utf-8')\n"
        "user_subsystem = base64.b64decode('%s').decode('utf-8')\n"
        "user_method = base64.b64decode('%s').decode('utf-8')\n"
        "candidate_subsystems = [s for s in [user_subsystem] if s] + [\n"
        "    'EditorAIAssistantSubsystem', 'AIAssistantSubsystem',\n"
        "    'CopilotEditorSubsystem', 'LLMEditorSubsystem',\n"
        "    'ChatGPTEditorSubsystem', 'UE5AgentPythonSubsystem',\n"
        "]\n"
        "candidate_methods = [m for m in [user_method] if m] + [\n"
        "    'send_chat_message', 'send_message', 'ask', 'chat', 'prompt', 'query'\n"
        "]\n"
        "result = {'success': False, 'tried': [], 'message': msg}\n"
        "for sub_name in candidate_subsystems:\n"
        "    cls = getattr(unreal, sub_name, None)\n"
        "    if cls is None:\n"
        "        result['tried'].append({'subsystem': sub_name, 'status': 'class_not_found'})\n"
        "        continue\n"
        "    try:\n"
        "        sub = unreal.get_editor_subsystem(cls)\n"
        "    except Exception as e:\n"
        "        result['tried'].append({'subsystem': sub_name, 'status': 'get_failed', 'error': str(e)})\n"
        "        continue\n"
        "    if sub is None:\n"
        "        result['tried'].append({'subsystem': sub_name, 'status': 'not_loaded'})\n"
        "        continue\n"
        "    for method_name in candidate_methods:\n"
        "        fn = getattr(sub, method_name, None)\n"
        "        if not callable(fn):\n"
        "            continue\n"
        "        try:\n"
        "            reply = fn(msg)\n"
        "            result.update({'success': True, 'subsystem': sub_name, 'method': method_name,\n"
        "                           'reply': str(reply) if reply is not None else ''})\n"
        "            break\n"
        "        except Exception as e:\n"
        "            result['tried'].append({'subsystem': sub_name, 'method': method_name, 'error': str(e)})\n"
        "    if result.get('success'):\n"
        "        break\n"
        "if not result['success']:\n"
        "    result['error'] = 'No AI Assistant subsystem/method responded. Use list_editor_subsystems to discover options, or pass explicit subsystem/method arguments.'\n"
        "print('__MCP_AI_ASSISTANT_BEGIN__' + json.dumps(result) + '__MCP_AI_ASSISTANT_END__')\n"
    ), *MessageB64, *SubsystemB64, *MethodB64);

    FString Captured;
    FString Warning;
    if (!MCP_RunExecCommand(MCP_WrapPythonAsSingleLineExec(Py), Captured, Warning))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(Warning);
    }

    // Extract the embedded JSON payload if present.
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    const FString BeginTag = TEXT("__MCP_AI_ASSISTANT_BEGIN__");
    const FString EndTag   = TEXT("__MCP_AI_ASSISTANT_END__");
    const int32 BeginIdx = Captured.Find(BeginTag);
    const int32 EndIdx   = Captured.Find(EndTag);
    if (BeginIdx != INDEX_NONE && EndIdx != INDEX_NONE && EndIdx > BeginIdx)
    {
        const FString Payload = Captured.Mid(BeginIdx + BeginTag.Len(), EndIdx - (BeginIdx + BeginTag.Len()));
        TSharedPtr<FJsonObject> Parsed;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
        if (FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid())
        {
            Parsed->SetStringField(TEXT("raw_output"), Captured);
            // Ensure the top-level success flag is always present.
            if (!Parsed->HasField(TEXT("success")))
            {
                Parsed->SetBoolField(TEXT("success"), false);
            }
            return Parsed;
        }
    }

    Result->SetBoolField(TEXT("success"), false);
    Result->SetStringField(TEXT("error"), TEXT("Could not parse AI Assistant response. The Python Editor Script Plugin may be disabled, or no AI-Assistant subsystem is available."));
    Result->SetStringField(TEXT("raw_output"), Captured);
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleListEditorSubsystems(const TSharedPtr<FJsonObject>& Params)
{
    const FString Py = TEXT(
        "import unreal, json\n"
        "names = []\n"
        "for attr in dir(unreal):\n"
        "    try:\n"
        "        cls = getattr(unreal, attr)\n"
        "    except Exception:\n"
        "        continue\n"
        "    if not isinstance(cls, type):\n"
        "        continue\n"
        "    try:\n"
        "        if issubclass(cls, unreal.EditorSubsystem) and cls is not unreal.EditorSubsystem:\n"
        "            names.append(attr)\n"
        "    except Exception:\n"
        "        continue\n"
        "print('__MCP_SUBSYSTEMS_BEGIN__' + json.dumps(sorted(names)) + '__MCP_SUBSYSTEMS_END__')\n"
    );

    FString Captured;
    FString Warning;
    if (!MCP_RunExecCommand(MCP_WrapPythonAsSingleLineExec(Py), Captured, Warning))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(Warning);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    const FString BeginTag = TEXT("__MCP_SUBSYSTEMS_BEGIN__");
    const FString EndTag   = TEXT("__MCP_SUBSYSTEMS_END__");
    const int32 BeginIdx = Captured.Find(BeginTag);
    const int32 EndIdx   = Captured.Find(EndTag);
    if (BeginIdx != INDEX_NONE && EndIdx != INDEX_NONE && EndIdx > BeginIdx)
    {
        const FString Payload = Captured.Mid(BeginIdx + BeginTag.Len(), EndIdx - (BeginIdx + BeginTag.Len()));
        TArray<TSharedPtr<FJsonValue>> Parsed;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
        if (FJsonSerializer::Deserialize(Reader, Parsed))
        {
            Result->SetBoolField(TEXT("success"), true);
            Result->SetArrayField(TEXT("subsystems"), Parsed);
            return Result;
        }
    }

    Result->SetBoolField(TEXT("success"), false);
    Result->SetStringField(TEXT("error"), TEXT("Could not enumerate subsystems. Ensure the Python Editor Script Plugin is enabled."));
    Result->SetStringField(TEXT("raw_output"), Captured);
    return Result;
}
