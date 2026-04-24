#include "Commands/BlueprintGraph/BPConnector.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "K2Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"

TSharedPtr<FJsonObject> FBPConnector::ConnectNodes(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    // Extract parameters
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString SourceNodeId = Params->GetStringField(TEXT("source_node_id"));
    FString SourcePinName = Params->GetStringField(TEXT("source_pin_name"));
    FString TargetNodeId = Params->GetStringField(TEXT("target_node_id"));
    FString TargetPinName = Params->GetStringField(TEXT("target_pin_name"));

    FString FunctionName;
    Params->TryGetStringField(TEXT("function_name"), FunctionName);

    // Load Blueprint - handle both full paths and simple names
    UBlueprint* Blueprint = nullptr;
    FString BlueprintPath = BlueprintName;

    // If no path prefix, assume /Game/Blueprints/
    if (!BlueprintPath.StartsWith(TEXT("/")))
    {
        BlueprintPath = TEXT("/Game/Blueprints/") + BlueprintPath;
    }

    // Add .Blueprint suffix if not present
    if (!BlueprintPath.Contains(TEXT(".")))
    {
        BlueprintPath += TEXT(".") + FPaths::GetBaseFilename(BlueprintPath);
    }

    // Try to load the Blueprint
    Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);

    // If not found, try with UEditorAssetLibrary
    if (!Blueprint)
    {
        FString AssetPath = BlueprintPath;
        if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
            Blueprint = Cast<UBlueprint>(Asset);
        }
    }

    if (!Blueprint)
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Blueprint not found"));

        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("requested_blueprint"), BlueprintName);
        Details->SetStringField(TEXT("resolved_path"), BlueprintPath);
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    // Get graph
    UEdGraph* Graph = nullptr;

    if (!FunctionName.IsEmpty())
    {
        // Strategy 1: Try exact name match with GetFName()
        for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
        {
            if (FuncGraph && (FuncGraph->GetFName().ToString() == FunctionName ||
                              (FuncGraph->GetOuter() && FuncGraph->GetOuter()->GetFName().ToString() == FunctionName)))
            {
                Graph = FuncGraph;
                break;
            }
        }

        // Strategy 2: Fallback - partial match for auto-generated names
        if (!Graph)
        {
            for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
            {
                if (FuncGraph && FuncGraph->GetFName().ToString().Contains(FunctionName))
                {
                    Graph = FuncGraph;
                    break;
                }
            }
        }

        if (!Graph)
        {
            Result->SetBoolField(TEXT("success"), false);
            Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));

            // Diagnostic: list available function graphs on this blueprint
            TArray<TSharedPtr<FJsonValue>> AvailableFuncs;
            for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
            {
                if (FuncGraph)
                {
                    AvailableFuncs.Add(MakeShared<FJsonValueString>(FuncGraph->GetFName().ToString()));
                }
            }
            TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
            Details->SetArrayField(TEXT("available_function_graphs"), AvailableFuncs);
            Result->SetObjectField(TEXT("error_details"), Details);
            return Result;
        }
    }
    else
    {
        // Use event graph if no function specified
        if (Blueprint->UbergraphPages.Num() == 0)
        {
            Result->SetBoolField(TEXT("success"), false);
            Result->SetStringField(TEXT("error"), TEXT("Blueprint has no event graph"));
            return Result;
        }

        Graph = Blueprint->UbergraphPages[0];
    }

    if (!Graph)
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Graph not found"));
        return Result;
    }

    // Find nodes
    UK2Node* SourceNode = FindNodeById(Graph, SourceNodeId);
    UK2Node* TargetNode = FindNodeById(Graph, TargetNodeId);

    if (!SourceNode || !TargetNode)
    {
        Result->SetBoolField(TEXT("success"), false);

        TArray<FString> MissingIds;
        if (!SourceNode) { MissingIds.Add(SourceNodeId); }
        if (!TargetNode) { MissingIds.Add(TargetNodeId); }
        Result->SetStringField(TEXT("error"),
            FString::Printf(TEXT("Node not found: %s"), *FString::Join(MissingIds, TEXT(", "))));

        // Diagnostic: list every node id + title in the graph so the caller can self-correct
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetArrayField(TEXT("available_nodes"), BuildNodeSummaries(Graph));
        TArray<TSharedPtr<FJsonValue>> MissingJson;
        for (const FString& Id : MissingIds)
        {
            MissingJson.Add(MakeShared<FJsonValueString>(Id));
        }
        Details->SetArrayField(TEXT("missing_node_ids"), MissingJson);
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    // Find pins
    UEdGraphPin* SourcePin = FindPinByName(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPinByName(TargetNode, TargetPinName, EGPD_Input);

    if (!SourcePin || !TargetPin)
    {
        Result->SetBoolField(TEXT("success"), false);

        TArray<FString> MissingPins;
        if (!SourcePin) { MissingPins.Add(FString::Printf(TEXT("output '%s' on %s"), *SourcePinName, *SourceNodeId)); }
        if (!TargetPin) { MissingPins.Add(FString::Printf(TEXT("input '%s' on %s"), *TargetPinName, *TargetNodeId)); }
        Result->SetStringField(TEXT("error"),
            FString::Printf(TEXT("Pin not found: %s"), *FString::Join(MissingPins, TEXT(", "))));

        // Diagnostic: list all pins on each node so the caller can pick the right one
        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        TSharedPtr<FJsonObject> SourcePins = MakeShared<FJsonObject>();
        SourcePins->SetStringField(TEXT("node_id"), SourceNodeId);
        SourcePins->SetArrayField(TEXT("pins"), BuildPinSummaries(SourceNode));
        Details->SetObjectField(TEXT("source_node"), SourcePins);

        TSharedPtr<FJsonObject> TargetPins = MakeShared<FJsonObject>();
        TargetPins->SetStringField(TEXT("node_id"), TargetNodeId);
        TargetPins->SetArrayField(TEXT("pins"), BuildPinSummaries(TargetNode));
        Details->SetObjectField(TEXT("target_node"), TargetPins);

        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    // Validate compatibility using the K2 schema (handles exec, wildcards, struct/object conversions,
    // BREAK_OTHERS_A/B and MAKE_WITH_CONVERSION cases correctly — unlike a raw category compare).
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    const FPinConnectionResponse ConnectionResponse = K2Schema->CanCreateConnection(SourcePin, TargetPin);

    const bool bCanConnect =
        ConnectionResponse.Response == CONNECT_RESPONSE_MAKE ||
        ConnectionResponse.Response == CONNECT_RESPONSE_BREAK_OTHERS_A ||
        ConnectionResponse.Response == CONNECT_RESPONSE_BREAK_OTHERS_B ||
        ConnectionResponse.Response == CONNECT_RESPONSE_BREAK_OTHERS_AB ||
        ConnectionResponse.Response == CONNECT_RESPONSE_MAKE_WITH_CONVERSION;

    if (!bCanConnect)
    {
        Result->SetBoolField(TEXT("success"), false);
        const FString SchemaMessage = ConnectionResponse.Message.ToString();
        if (!SchemaMessage.IsEmpty())
        {
            Result->SetStringField(TEXT("error"),
                FString::Printf(TEXT("Pins not compatible: %s"), *SchemaMessage));
        }
        else
        {
            Result->SetStringField(TEXT("error"), TEXT("Pins not compatible"));
        }

        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("schema_message"), SchemaMessage);
        Details->SetStringField(TEXT("source_pin_type"), DescribePinType(SourcePin));
        Details->SetStringField(TEXT("target_pin_type"), DescribePinType(TargetPin));
        Details->SetNumberField(TEXT("schema_response_code"), (int32)ConnectionResponse.Response);
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    // Use the schema's TryCreateConnection so conversion nodes etc. are inserted when required.
    // Fall back to MakeLinkTo if the schema can't perform it for some reason.
    if (!K2Schema->TryCreateConnection(SourcePin, TargetPin))
    {
        SourcePin->MakeLinkTo(TargetPin);
    }

    // Recompile
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Return
    Result->SetBoolField(TEXT("success"), true);

    TSharedPtr<FJsonObject> ConnectionInfo = MakeShared<FJsonObject>();
    ConnectionInfo->SetStringField(TEXT("source_node"), SourceNodeId);
    ConnectionInfo->SetStringField(TEXT("source_pin"), SourcePinName);
    ConnectionInfo->SetStringField(TEXT("target_node"), TargetNodeId);
    ConnectionInfo->SetStringField(TEXT("target_pin"), TargetPinName);
    ConnectionInfo->SetStringField(TEXT("connection_type"), SourcePin->PinType.PinCategory.ToString());
    ConnectionInfo->SetStringField(TEXT("source_pin_type"), DescribePinType(SourcePin));
    ConnectionInfo->SetStringField(TEXT("target_pin_type"), DescribePinType(TargetPin));

    Result->SetObjectField(TEXT("connection"), ConnectionInfo);

    return Result;
}

TSharedPtr<FJsonObject> FBPConnector::DisconnectNodes(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString SourceNodeId = Params->GetStringField(TEXT("source_node_id"));
    FString SourcePinName = Params->GetStringField(TEXT("source_pin_name"));
    FString TargetNodeId = Params->GetStringField(TEXT("target_node_id"));
    FString TargetPinName = Params->GetStringField(TEXT("target_pin_name"));

    FString FunctionName;
    Params->TryGetStringField(TEXT("function_name"), FunctionName);

    FString BlueprintPath = BlueprintName;
    if (!BlueprintPath.StartsWith(TEXT("/")))
    {
        BlueprintPath = TEXT("/Game/Blueprints/") + BlueprintPath;
    }
    if (!BlueprintPath.Contains(TEXT(".")))
    {
        BlueprintPath += TEXT(".") + FPaths::GetBaseFilename(BlueprintPath);
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint && UEditorAssetLibrary::DoesAssetExist(BlueprintPath))
    {
        Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    }

    if (!Blueprint)
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Blueprint not found"));

        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("requested_blueprint"), BlueprintName);
        Details->SetStringField(TEXT("resolved_path"), BlueprintPath);
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    UEdGraph* Graph = nullptr;
    if (!FunctionName.IsEmpty())
    {
        for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
        {
            if (FuncGraph && (FuncGraph->GetFName().ToString() == FunctionName ||
                              FuncGraph->GetFName().ToString().Contains(FunctionName)))
            {
                Graph = FuncGraph;
                break;
            }
        }

        if (!Graph)
        {
            Result->SetBoolField(TEXT("success"), false);
            Result->SetStringField(TEXT("error"), FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));

            TArray<TSharedPtr<FJsonValue>> AvailableFuncs;
            for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
            {
                if (FuncGraph)
                {
                    AvailableFuncs.Add(MakeShared<FJsonValueString>(FuncGraph->GetFName().ToString()));
                }
            }
            TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
            Details->SetArrayField(TEXT("available_function_graphs"), AvailableFuncs);
            Result->SetObjectField(TEXT("error_details"), Details);
            return Result;
        }
    }
    else if (Blueprint->UbergraphPages.Num() > 0)
    {
        Graph = Blueprint->UbergraphPages[0];
    }

    if (!Graph)
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Graph not found"));
        return Result;
    }

    UK2Node* SourceNode = FindNodeById(Graph, SourceNodeId);
    UK2Node* TargetNode = FindNodeById(Graph, TargetNodeId);
    if (!SourceNode || !TargetNode)
    {
        Result->SetBoolField(TEXT("success"), false);
        TArray<FString> MissingIds;
        if (!SourceNode) { MissingIds.Add(SourceNodeId); }
        if (!TargetNode) { MissingIds.Add(TargetNodeId); }
        Result->SetStringField(TEXT("error"),
            FString::Printf(TEXT("Node not found: %s"), *FString::Join(MissingIds, TEXT(", "))));

        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetArrayField(TEXT("available_nodes"), BuildNodeSummaries(Graph));
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    UEdGraphPin* SourcePin = FindPinByName(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPinByName(TargetNode, TargetPinName, EGPD_Input);
    if (!SourcePin || !TargetPin)
    {
        Result->SetBoolField(TEXT("success"), false);
        TArray<FString> MissingPins;
        if (!SourcePin) { MissingPins.Add(FString::Printf(TEXT("output '%s' on %s"), *SourcePinName, *SourceNodeId)); }
        if (!TargetPin) { MissingPins.Add(FString::Printf(TEXT("input '%s' on %s"), *TargetPinName, *TargetNodeId)); }
        Result->SetStringField(TEXT("error"),
            FString::Printf(TEXT("Pin not found: %s"), *FString::Join(MissingPins, TEXT(", "))));

        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        TSharedPtr<FJsonObject> SourcePins = MakeShared<FJsonObject>();
        SourcePins->SetStringField(TEXT("node_id"), SourceNodeId);
        SourcePins->SetArrayField(TEXT("pins"), BuildPinSummaries(SourceNode));
        Details->SetObjectField(TEXT("source_node"), SourcePins);

        TSharedPtr<FJsonObject> TargetPins = MakeShared<FJsonObject>();
        TargetPins->SetStringField(TEXT("node_id"), TargetNodeId);
        TargetPins->SetArrayField(TEXT("pins"), BuildPinSummaries(TargetNode));
        Details->SetObjectField(TEXT("target_node"), TargetPins);
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    if (!SourcePin->LinkedTo.Contains(TargetPin))
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Pins are not currently connected"));

        TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("source_pin_type"), DescribePinType(SourcePin));
        Details->SetStringField(TEXT("target_pin_type"), DescribePinType(TargetPin));
        Result->SetObjectField(TEXT("error_details"), Details);
        return Result;
    }

    SourcePin->BreakLinkTo(TargetPin);
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    Result->SetBoolField(TEXT("success"), true);
    TSharedPtr<FJsonObject> ConnectionInfo = MakeShared<FJsonObject>();
    ConnectionInfo->SetStringField(TEXT("source_node"), SourceNodeId);
    ConnectionInfo->SetStringField(TEXT("source_pin"), SourcePinName);
    ConnectionInfo->SetStringField(TEXT("target_node"), TargetNodeId);
    ConnectionInfo->SetStringField(TEXT("target_pin"), TargetPinName);
    Result->SetObjectField(TEXT("disconnected"), ConnectionInfo);
    return Result;
}

UK2Node* FBPConnector::FindNodeById(UEdGraph* Graph, const FString& NodeId)
{
    if (!Graph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Try matching by NodeGuid first
        if (Node->NodeGuid.ToString().Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UK2Node* K2Node = Cast<UK2Node>(Node);
            return K2Node;  // Return even if nullptr (caller will handle)
        }

        // Try matching by GetName()
        if (Node->GetName().Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UK2Node* K2Node = Cast<UK2Node>(Node);
            return K2Node;  // Return even if nullptr (caller will handle)
        }
    }

    return nullptr;
}

UEdGraphPin* FBPConnector::FindPinByName(UK2Node* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->PinName.ToString() == PinName && Pin->Direction == Direction)
        {
            return Pin;
        }
    }
    return nullptr;
}

TArray<TSharedPtr<FJsonValue>> FBPConnector::BuildNodeSummaries(UEdGraph* Graph)
{
    TArray<TSharedPtr<FJsonValue>> NodeArray;
    if (!Graph)
    {
        return NodeArray;
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
        NodeObj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
        NodeObj->SetStringField(TEXT("name"), Node->GetName());
        NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
        NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
        NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }
    return NodeArray;
}

TArray<TSharedPtr<FJsonValue>> FBPConnector::BuildPinSummaries(UEdGraphNode* Node)
{
    TArray<TSharedPtr<FJsonValue>> PinArray;
    if (!Node)
    {
        return PinArray;
    }

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin)
        {
            continue;
        }

        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Output ? TEXT("output") : TEXT("input"));
        PinObj->SetStringField(TEXT("type"), DescribePinType(Pin));
        PinObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
        if (!Pin->PinType.PinSubCategory.IsNone())
        {
            PinObj->SetStringField(TEXT("sub_category"), Pin->PinType.PinSubCategory.ToString());
        }
        if (Pin->PinType.PinSubCategoryObject.IsValid())
        {
            PinObj->SetStringField(TEXT("sub_category_object"), Pin->PinType.PinSubCategoryObject->GetName());
        }
        PinObj->SetBoolField(TEXT("is_array"), Pin->PinType.IsArray());
        PinObj->SetBoolField(TEXT("is_reference"), Pin->PinType.bIsReference);
        PinArray.Add(MakeShared<FJsonValueObject>(PinObj));
    }
    return PinArray;
}

FString FBPConnector::DescribePinType(UEdGraphPin* Pin)
{
    if (!Pin)
    {
        return TEXT("<null>");
    }

    const FEdGraphPinType& PinType = Pin->PinType;
    FString Desc = PinType.PinCategory.ToString();
    if (PinType.PinSubCategoryObject.IsValid())
    {
        Desc += FString::Printf(TEXT(":%s"), *PinType.PinSubCategoryObject->GetName());
    }
    else if (!PinType.PinSubCategory.IsNone())
    {
        Desc += FString::Printf(TEXT(":%s"), *PinType.PinSubCategory.ToString());
    }
    if (PinType.IsArray())
    {
        Desc += TEXT("[]");
    }
    else if (PinType.IsSet())
    {
        Desc += TEXT("{set}");
    }
    else if (PinType.IsMap())
    {
        Desc += TEXT("{map}");
    }
    if (PinType.bIsReference)
    {
        Desc += TEXT("&");
    }
    return Desc;
}
