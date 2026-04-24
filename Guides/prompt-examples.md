# Prompt Examples

These examples use generic MCP tools. Prefer configurable tools such as `create_composition`, `search_assets`, `validate_gameplay_stack`, and `scaffold_cpp_classes` instead of domain-specific one-off tool names.

## Generic Level Composition

```python
create_composition(
    composition_type="building",
    parameters={"width": 1200, "depth": 800, "location": [0, 0, 0]},
)

create_composition(
    composition_type="maze",
    parameters={"rows": 12, "cols": 12, "wall_height": 3, "cell_size": 250},
)

create_composition(
    composition_type="settlement",
    parameters={"town_size": "medium", "building_density": 0.8, "location": [0, 0, 0]},
)
```

## Asset Discovery and Validation

```python
search_assets(
    class_names=["Blueprint", "AnimBlueprint", "SkeletalMesh", "DataAsset"],
    paths=["/Game/"],
)

validate_gameplay_stack(
    required_plugins=["EnhancedInput", "GameplayAbilities"],
    asset_keywords=["Movement", "Menu", "Customization"],
    paths=["/Game/"],
)
```

## Blueprint Cleanup

```python
audit_blueprint("/Game/Blueprints/BP_Player.BP_Player")
organize_blueprint_graph(
    blueprint_path="/Game/Blueprints/BP_Player.BP_Player",
    graph_name="EventGraph",
    dry_run=True,
)
```

## C++ as an Alternative to Blueprint Logic

```python
scaffold_cpp_classes(
    class_specs=[
        {"class_name": "GameplayCharacter", "parent_class": "Character", "subfolder": "Characters"},
        {"class_name": "MovementAbilityComponent", "parent_class": "ActorComponent", "subfolder": "Abilities"},
        {"class_name": "MainMenuWidget", "parent_class": "UserWidget", "subfolder": "UI"},
    ],
    dry_run=True,
)
```
