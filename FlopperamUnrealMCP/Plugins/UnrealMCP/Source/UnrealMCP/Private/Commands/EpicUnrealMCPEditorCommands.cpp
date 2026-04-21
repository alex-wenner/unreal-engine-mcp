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
