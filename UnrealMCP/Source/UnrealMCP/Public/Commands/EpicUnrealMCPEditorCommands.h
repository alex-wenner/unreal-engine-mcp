#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Editor-related MCP commands
 * Handles viewport control, actor manipulation, and level management
 */
class UNREALMCP_API FEpicUnrealMCPEditorCommands
{
public:
    	FEpicUnrealMCPEditorCommands();

    // Handle editor commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Actor manipulation commands
    TSharedPtr<FJsonObject> HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params);

    // Blueprint actor spawning
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);

    // Generalized asset search (by class + paths) — built on FARFilter / FTopLevelAssetPath
    TSharedPtr<FJsonObject> HandleSearchAssets(const TSharedPtr<FJsonObject>& Params);

    // Fab-safe local import and asset-management commands
    TSharedPtr<FJsonObject> HandleDetectUnrealPlugins(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleOpenFabBrowser(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleImportAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleBulkImportAssets(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateMaterialInstanceFromImport(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandlePlaceImportedAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleTagImportedAssets(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleManageAsset(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFixRedirectors(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListAssetDependencies(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGenerateImportedAssetManifest(const TSharedPtr<FJsonObject>& Params);

    // C++ source generation commands
    TSharedPtr<FJsonObject> HandleCreateCppClass(const TSharedPtr<FJsonObject>& Params);
};
