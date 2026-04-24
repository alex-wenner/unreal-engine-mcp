// Connects two Blueprint nodes via their pins
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class UK2Node;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
enum EEdGraphPinDirection : int;

/**
 * Utility class for connecting Blueprint nodes
 */
class UNREALMCP_API FBPConnector
{
public:
    /**
     * Connects two Blueprint nodes via their pins
     * @param Params JSON containing blueprint_name, source_node_id, source_pin_name, target_node_id, target_pin_name
     * @return JSON with success and connection details. On failure, an "error_details" object may be attached
     *         containing contextual information (available nodes, available pins with types, schema message, etc.)
     *         to help callers self-correct.
     */
    static TSharedPtr<FJsonObject> ConnectNodes(const TSharedPtr<FJsonObject>& Params);

    /**
     * Disconnects two Blueprint nodes via their pins
     * @param Params JSON containing blueprint_name, source_node_id, source_pin_name, target_node_id, target_pin_name
     * @return JSON with success and connection details. On failure, an "error_details" object may be attached.
     */
    static TSharedPtr<FJsonObject> DisconnectNodes(const TSharedPtr<FJsonObject>& Params);

private:
    /**
     * Finds a node by its ID in the graph
     */
    static UK2Node* FindNodeById(UEdGraph* Graph, const FString& NodeId);

    /**
     * Finds a pin by its name in a node
     */
    static UEdGraphPin* FindPinByName(UK2Node* Node, const FString& PinName, EEdGraphPinDirection Direction);

    /**
     * Builds a JSON array describing all nodes in a graph (id, name, title) — used as diagnostic info
     * when the caller passes a node id that we can't resolve.
     */
    static TArray<TSharedPtr<FJsonValue>> BuildNodeSummaries(UEdGraph* Graph);

    /**
     * Builds a JSON array describing a node's pins (name, direction, category, sub-category, container type)
     * — used as diagnostic info when the caller passes a pin name we can't resolve.
     */
    static TArray<TSharedPtr<FJsonValue>> BuildPinSummaries(UEdGraphNode* Node);

    /**
     * Builds a short human-readable description of a pin's type (e.g. "exec", "float", "object:Actor").
     */
    static FString DescribePinType(UEdGraphPin* Pin);
};
