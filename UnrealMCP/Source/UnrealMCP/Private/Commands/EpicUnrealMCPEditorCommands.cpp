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
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Interfaces/IPluginManager.h"
#include "Framework/Docking/TabManager.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"

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
    else if (CommandType == TEXT("detect_unreal_plugins"))
    {
        return HandleDetectUnrealPlugins(Params);
    }
    else if (CommandType == TEXT("open_fab_browser"))
    {
        return HandleOpenFabBrowser(Params);
    }
    else if (CommandType == TEXT("import_asset"))
    {
        return HandleImportAsset(Params);
    }
    else if (CommandType == TEXT("bulk_import_assets"))
    {
        return HandleBulkImportAssets(Params);
    }
    else if (CommandType == TEXT("create_material_instance_from_import"))
    {
        return HandleCreateMaterialInstanceFromImport(Params);
    }
    else if (CommandType == TEXT("place_imported_asset"))
    {
        return HandlePlaceImportedAsset(Params);
    }
    else if (CommandType == TEXT("tag_imported_assets"))
    {
        return HandleTagImportedAssets(Params);
    }
    else if (CommandType == TEXT("manage_asset"))
    {
        return HandleManageAsset(Params);
    }
    else if (CommandType == TEXT("fix_redirectors"))
    {
        return HandleFixRedirectors(Params);
    }
    else if (CommandType == TEXT("list_asset_dependencies"))
    {
        return HandleListAssetDependencies(Params);
    }
    else if (CommandType == TEXT("generate_imported_asset_manifest"))
    {
        return HandleGenerateImportedAssetManifest(Params);
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
            // Animation tooling plugins
            { TEXT("PoseSearchDatabase"),    TEXT("/Script/PoseSearch"), TEXT("PoseSearchDatabase") },
            { TEXT("IKRigDefinition"),       TEXT("/Script/IKRig"),      TEXT("IKRigDefinition") },
            { TEXT("IKRetargeter"),          TEXT("/Script/IKRig"),      TEXT("IKRetargeter") },
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

namespace
{
    static TArray<FString> ReadStringArrayField(const TSharedPtr<FJsonObject>& Params, const FString& FieldName)
    {
        TArray<FString> Values;
        const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
        if (Params->TryGetArrayField(FieldName, JsonArray))
        {
            for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
            {
                if (Value.IsValid() && Value->Type == EJson::String)
                {
                    Values.Add(Value->AsString());
                }
            }
        }
        return Values;
    }

    static void SetStringArrayField(TSharedPtr<FJsonObject> JsonObject, const FString& FieldName, const TArray<FString>& Values)
    {
        TArray<TSharedPtr<FJsonValue>> JsonValues;
        for (const FString& Value : Values)
        {
            JsonValues.Add(MakeShared<FJsonValueString>(Value));
        }
        JsonObject->SetArrayField(FieldName, JsonValues);
    }

    static FString NormalizeContentPath(FString Path)
    {
        Path = Path.TrimStartAndEnd();
        if (Path.IsEmpty())
        {
            return TEXT("/Game/");
        }
        if (!Path.StartsWith(TEXT("/")))
        {
            Path = TEXT("/") + Path;
        }
        return Path;
    }

    static TSharedPtr<FJsonObject> ImportLocalFiles(const TArray<FString>& Files, const FString& DestinationPath, bool bReplaceExisting, bool bSave)
    {
        TArray<UAssetImportTask*> Tasks;
        for (const FString& File : Files)
        {
            if (!FPaths::FileExists(File))
            {
                continue;
            }
            UAssetImportTask* Task = NewObject<UAssetImportTask>();
            Task->Filename = File;
            Task->DestinationPath = DestinationPath;
            Task->bAutomated = true;
            Task->bReplaceExisting = bReplaceExisting;
            Task->bSave = bSave;
            Tasks.Add(Task);
        }

        if (Tasks.Num() == 0)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No existing local files were provided for import"));
        }

        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
        AssetToolsModule.Get().ImportAssetTasks(Tasks);

        TArray<TSharedPtr<FJsonValue>> ImportedAssets;
        TArray<TSharedPtr<FJsonValue>> FailedFiles;
        for (const UAssetImportTask* Task : Tasks)
        {
            if (!Task)
            {
                continue;
            }

            if (Task->ImportedObjectPaths.Num() == 0)
            {
                TSharedPtr<FJsonObject> Failed = MakeShared<FJsonObject>();
                Failed->SetStringField(TEXT("file_path"), Task->Filename);
                FailedFiles.Add(MakeShared<FJsonValueObject>(Failed));
                continue;
            }

            for (const FString& ImportedPath : Task->ImportedObjectPaths)
            {
                TSharedPtr<FJsonObject> Imported = MakeShared<FJsonObject>();
                Imported->SetStringField(TEXT("file_path"), Task->Filename);
                Imported->SetStringField(TEXT("asset_path"), ImportedPath);
                ImportedAssets.Add(MakeShared<FJsonValueObject>(Imported));
            }
        }

        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"), ImportedAssets.Num() > 0 && FailedFiles.Num() == 0);
        Result->SetArrayField(TEXT("imported_assets"), ImportedAssets);
        Result->SetArrayField(TEXT("failed_files"), FailedFiles);
        Result->SetNumberField(TEXT("count"), ImportedAssets.Num());
        Result->SetStringField(TEXT("destination_path"), DestinationPath);
        return Result;
    }

    static TSharedPtr<FJsonObject> AssetDataToManifestJson(const FAssetData& AssetData)
    {
        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        AssetObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());
        AssetObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.ToString());
        return AssetObj;
    }
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDetectUnrealPlugins(const TSharedPtr<FJsonObject>& Params)
{
    const TArray<FString> PluginNames = ReadStringArrayField(Params, TEXT("plugin_names"));
    if (PluginNames.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'plugin_names'"));
    }

    TArray<TSharedPtr<FJsonValue>> Plugins;
    for (const FString& PluginName : PluginNames)
    {
        TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
        TSharedPtr<FJsonObject> PluginObj = MakeShared<FJsonObject>();
        PluginObj->SetStringField(TEXT("name"), PluginName);
        PluginObj->SetBoolField(TEXT("installed"), Plugin.IsValid());
        PluginObj->SetBoolField(TEXT("enabled"), Plugin.IsValid() ? Plugin->IsEnabled() : false);
        if (Plugin.IsValid())
        {
            PluginObj->SetStringField(TEXT("friendly_name"), Plugin->GetFriendlyName());
            PluginObj->SetStringField(TEXT("base_dir"), Plugin->GetBaseDir());
            PluginObj->SetStringField(TEXT("content_dir"), Plugin->GetContentDir());
            PluginObj->SetStringField(TEXT("version_name"), Plugin->GetDescriptor().VersionName);
            PluginObj->SetBoolField(TEXT("can_contain_content"), Plugin->GetDescriptor().bCanContainContent);
        }
        Plugins.Add(MakeShared<FJsonValueObject>(PluginObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetArrayField(TEXT("plugins"), Plugins);
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleOpenFabBrowser(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<IPlugin> FabPlugin = IPluginManager::Get().FindPlugin(TEXT("Fab"));
    if (!FabPlugin.IsValid() || !FabPlugin->IsEnabled())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Fab plugin is not installed or enabled. Complete Fab install/auth/licensing in the editor; MCP only automates local/project content."));
    }

    TArray<FString> TriedTabs;
    const TCHAR* CandidateTabs[] = { TEXT("Fab"), TEXT("FabBrowser"), TEXT("FabTab"), TEXT("FabContentBrowser") };
    for (const TCHAR* CandidateTab : CandidateTabs)
    {
        TriedTabs.Add(CandidateTab);
        TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FName(CandidateTab)));
        if (Tab.IsValid())
        {
            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("success"), true);
            Result->SetStringField(TEXT("opened_tab"), CandidateTab);
            Result->SetStringField(TEXT("message"), TEXT("Opened Fab UI; use the editor for auth/licensing/downloads, then MCP can work with local/project assets."));
            return Result;
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), false);
    Result->SetStringField(TEXT("message"), TEXT("Fab is enabled, but this Unreal version did not expose a known Fab tab id."));
    SetStringArrayField(Result, TEXT("tried_tabs"), TriedTabs);
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleImportAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("file_path"), FilePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'file_path'"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'destination_path'"));
    }

    bool bReplaceExisting = false;
    Params->TryGetBoolField(TEXT("replace_existing"), bReplaceExisting);
    bool bSave = true;
    Params->TryGetBoolField(TEXT("save"), bSave);

    TArray<FString> Files;
    Files.Add(FilePath);
    return ImportLocalFiles(Files, NormalizeContentPath(DestinationPath), bReplaceExisting, bSave);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleBulkImportAssets(const TSharedPtr<FJsonObject>& Params)
{
    FString Folder;
    if (!Params->TryGetStringField(TEXT("folder"), Folder))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'folder'"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'destination_path'"));
    }

    TArray<FString> Extensions = ReadStringArrayField(Params, TEXT("extensions"));
    if (Extensions.Num() == 0)
    {
        Extensions = { TEXT("fbx"), TEXT("obj"), TEXT("gltf"), TEXT("glb"), TEXT("png"), TEXT("jpg"), TEXT("jpeg"), TEXT("tga"), TEXT("exr"), TEXT("wav"), TEXT("mp3") };
    }

    bool bRecursive = true;
    Params->TryGetBoolField(TEXT("recursive"), bRecursive);
    bool bReplaceExisting = false;
    Params->TryGetBoolField(TEXT("replace_existing"), bReplaceExisting);
    bool bSave = true;
    Params->TryGetBoolField(TEXT("save"), bSave);

    TArray<FString> Files;
    for (FString Extension : Extensions)
    {
        Extension.RemoveFromStart(TEXT("."));
        if (bRecursive)
        {
            TArray<FString> Found;
            IFileManager::Get().FindFilesRecursive(Found, *Folder, *FString::Printf(TEXT("*.%s"), *Extension), true, false);
            Files.Append(Found);
        }
        else
        {
            TArray<FString> Found;
            IFileManager::Get().FindFiles(Found, *(Folder / FString::Printf(TEXT("*.%s"), *Extension)), true, false);
            for (const FString& Name : Found)
            {
                Files.Add(Folder / Name);
            }
        }
    }
    Files.Sort();

    return ImportLocalFiles(Files, NormalizeContentPath(DestinationPath), bReplaceExisting, bSave);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCreateMaterialInstanceFromImport(const TSharedPtr<FJsonObject>& Params)
{
    FString Name;
    FString DestinationPath;
    FString ParentMaterialPath;
    if (!Params->TryGetStringField(TEXT("name"), Name) ||
        !Params->TryGetStringField(TEXT("destination_path"), DestinationPath) ||
        !Params->TryGetStringField(TEXT("parent_material"), ParentMaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters: 'name', 'destination_path', and 'parent_material'"));
    }

    UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *ParentMaterialPath);
    if (!Parent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not load parent material '%s'"), *ParentMaterialPath));
    }

    const FString PackagePath = NormalizeContentPath(DestinationPath) / Name;
    UPackage* Package = CreatePackage(*PackagePath);
    UMaterialInstanceConstant* MaterialInstance = NewObject<UMaterialInstanceConstant>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
    MaterialInstance->SetParentEditorOnly(Parent);
    MaterialInstance->PostEditChange();
    MaterialInstance->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(MaterialInstance);

    bool bSave = true;
    Params->TryGetBoolField(TEXT("save"), bSave);
    if (bSave)
    {
        UEditorAssetLibrary::SaveLoadedAsset(MaterialInstance);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_path"), PackagePath + TEXT(".") + Name);
    Result->SetStringField(TEXT("parent_material"), ParentMaterialPath);
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandlePlaceImportedAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'asset_path'"));
    }

    UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!Asset || !World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not load asset or editor world"));
    }

    FVector Location(0, 0, 0);
    FRotator Rotation(0, 0, 0);
    FVector Scale(1, 1, 1);
    if (Params->HasField(TEXT("location"))) Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    if (Params->HasField(TEXT("rotation"))) Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    if (Params->HasField(TEXT("scale"))) Scale = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));

    FString ActorName;
    Params->TryGetStringField(TEXT("name"), ActorName);
    FActorSpawnParameters SpawnParams;
    if (!ActorName.IsEmpty())
    {
        SpawnParams.Name = *ActorName;
    }

    AActor* NewActor = nullptr;
    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
    {
        AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
        if (MeshActor && MeshActor->GetStaticMeshComponent())
        {
            MeshActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
            NewActor = MeshActor;
        }
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
    {
        ASkeletalMeshActor* SkeletalActor = World->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), Location, Rotation, SpawnParams);
        if (SkeletalActor && SkeletalActor->GetSkeletalMeshComponent())
        {
            SkeletalActor->GetSkeletalMeshComponent()->SetSkeletalMeshAsset(SkeletalMesh);
            NewActor = SkeletalActor;
        }
    }
    else if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
    {
        if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
        {
            NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, Location, Rotation, SpawnParams);
        }
    }

    if (!NewActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Asset is not directly placeable. Supported: StaticMesh, SkeletalMesh, Actor Blueprint."));
    }

    NewActor->SetActorScale3D(Scale);
    return FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleTagImportedAssets(const TSharedPtr<FJsonObject>& Params)
{
    const TArray<FString> AssetPaths = ReadStringArrayField(Params, TEXT("asset_paths"));
    const TSharedPtr<FJsonObject>* Tags = nullptr;
    if (AssetPaths.Num() == 0 || !Params->TryGetObjectField(TEXT("tags"), Tags) || !Tags || !Tags->IsValid())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters 'asset_paths' and 'tags'"));
    }

    TArray<FString> Tagged;
    for (const FString& AssetPath : AssetPaths)
    {
        UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
        if (!Asset)
        {
            continue;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Tags)->Values)
        {
            UEditorAssetLibrary::SetMetadataTag(Asset, FName(*Pair.Key), Pair.Value.IsValid() ? Pair.Value->AsString() : FString());
        }
        UEditorAssetLibrary::SaveLoadedAsset(Asset);
        Tagged.Add(AssetPath);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), Tagged.Num() == AssetPaths.Num());
    SetStringArrayField(Result, TEXT("tagged_assets"), Tagged);
    Result->SetNumberField(TEXT("count"), Tagged.Num());
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleManageAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString Action;
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("action"), Action) || !Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameters 'action' and 'asset_path'"));
    }

    bool bSuccess = false;
    FString DestinationPath;
    if (Action.Equals(TEXT("delete"), ESearchCase::IgnoreCase))
    {
        bSuccess = UEditorAssetLibrary::DeleteAsset(AssetPath);
    }
    else
    {
        if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'destination_path' for move/rename"));
        }
        bSuccess = UEditorAssetLibrary::RenameAsset(AssetPath, DestinationPath);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), bSuccess);
    Result->SetStringField(TEXT("action"), Action);
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    if (!DestinationPath.IsEmpty())
    {
        Result->SetStringField(TEXT("destination_path"), DestinationPath);
    }
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleFixRedirectors(const TSharedPtr<FJsonObject>& Params)
{
    TArray<FString> Paths = ReadStringArrayField(Params, TEXT("paths"));
    if (Paths.Num() == 0)
    {
        Paths.Add(TEXT("/Game/"));
    }

    FARFilter Filter;
    Filter.ClassPaths.Add(UObjectRedirector::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    for (const FString& Path : Paths)
    {
        Filter.PackagePaths.Add(FName(*NormalizeContentPath(Path)));
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    TArray<FAssetData> RedirectorAssets;
    AssetRegistryModule.Get().GetAssets(Filter, RedirectorAssets);

    TArray<UObjectRedirector*> Redirectors;
    for (const FAssetData& AssetData : RedirectorAssets)
    {
        if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(AssetData.GetAsset()))
        {
            Redirectors.Add(Redirector);
        }
    }

    if (Redirectors.Num() > 0)
    {
        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
        AssetToolsModule.Get().FixupReferencers(Redirectors);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("redirectors_found"), RedirectorAssets.Num());
    Result->SetNumberField(TEXT("redirectors_fixed"), Redirectors.Num());
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleListAssetDependencies(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter 'asset_path'"));
    }

    bool bReferencers = false;
    Params->TryGetBoolField(TEXT("referencers"), bReferencers);
    const FString PackageName = AssetPath.Contains(TEXT(".")) ? FPackageName::ObjectPathToPackageName(AssetPath) : AssetPath;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    TArray<FName> Names;
    if (bReferencers)
    {
        AssetRegistryModule.Get().GetReferencers(FName(*PackageName), Names);
    }
    else
    {
        AssetRegistryModule.Get().GetDependencies(FName(*PackageName), Names);
    }

    TArray<FString> Values;
    for (const FName& Name : Names)
    {
        Values.Add(Name.ToString());
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("asset_path"), AssetPath);
    Result->SetStringField(TEXT("package"), PackageName);
    Result->SetStringField(TEXT("relationship"), bReferencers ? TEXT("referencers") : TEXT("dependencies"));
    Result->SetNumberField(TEXT("count"), Values.Num());
    SetStringArrayField(Result, bReferencers ? TEXT("referencers") : TEXT("dependencies"), Values);
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGenerateImportedAssetManifest(const TSharedPtr<FJsonObject>& Params)
{
    TArray<FString> Paths = ReadStringArrayField(Params, TEXT("paths"));
    if (Paths.Num() == 0)
    {
        Paths.Add(TEXT("/Game/"));
    }
    const TArray<FString> ClassNames = ReadStringArrayField(Params, TEXT("class_names"));

    int32 MaxResults = 1000;
    double RawMax = 0.0;
    if (Params->TryGetNumberField(TEXT("max_results"), RawMax))
    {
        MaxResults = FMath::Clamp((int32)RawMax, 1, 10000);
    }

    FARFilter Filter;
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    for (const FString& Path : Paths)
    {
        Filter.PackagePaths.Add(FName(*NormalizeContentPath(Path)));
    }
    for (const FString& ClassName : ClassNames)
    {
        const FTopLevelAssetPath ClassPath = ResolveAssetClassPath(ClassName);
        if (ClassPath.IsValid())
        {
            Filter.ClassPaths.Add(ClassPath);
        }
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    TArray<FAssetData> AssetDataArray;
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataArray);

    const int32 ReturnCount = FMath::Min(AssetDataArray.Num(), MaxResults);
    TArray<TSharedPtr<FJsonValue>> Assets;
    for (int32 Index = 0; Index < ReturnCount; ++Index)
    {
        Assets.Add(MakeShared<FJsonValueObject>(AssetDataToManifestJson(AssetDataArray[Index])));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetArrayField(TEXT("assets"), Assets);
    Result->SetNumberField(TEXT("count"), ReturnCount);
    Result->SetNumberField(TEXT("total_found"), AssetDataArray.Num());
    Result->SetBoolField(TEXT("truncated"), AssetDataArray.Num() > ReturnCount);
    return Result;
}
