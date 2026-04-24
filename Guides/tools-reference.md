# 🔧 Advanced Tools Reference

Reference documentation for the Unreal MCP Advanced Server. The canonical tool list is generated from `@mcp.tool()` functions in `Python/unreal_mcp_server_advanced.py`.

## 🏗️ World Building Tools

### create_town
Create complete urban environments with buildings, roads, and infrastructure.

**Parameters:**
- `town_size` (string): "small", "medium", "large", or "metropolis"
- `architectural_style` (string): "modern", "medieval", "suburban", "downtown", "mixed", or "futuristic"  
- `building_density` (float, 0.0-1.0): How packed the buildings are
- `location` (array): [X, Y, Z] world position for town center
- `include_infrastructure` (bool): Add roads, utilities, etc.
- `name_prefix` (string): Prefix for spawned building actors

**Example:**
```bash
create_town(town_size="medium", architectural_style="modern", building_density=0.8, location=[0, 0, 0])
```

### construct_house  
Build realistic multi-room houses with architectural details.

**Parameters:**
- `width` (int): House width in centimeters (default: 1200)
- `depth` (int): House depth in centimeters (default: 1000)
- `height` (int): Wall height in centimeters (default: 600)
- `location` (array): House center position
- `house_style` (string): "modern", "cottage", or "mansion"
- `mesh` (string): Static mesh asset path
- `name_prefix` (string): Prefix for house components

**Features:**
- **Foundation & Floor**: Proper structural base
- **Room Division**: Interior walls creating realistic spaces
- **Windows & Doors**: Authentic openings with proper sizing
- **Pitched Roof**: Angled rooftop instead of flat surface
- **Style Variations**: Different proportions and decorative elements

**Examples:**
```bash
# Modern family home
construct_house(house_style="modern", location=[0, 0, 0])

# Large mansion
construct_house(width=1500, depth=1200, house_style="mansion", location=[2000, 0, 0])

# Cozy cottage  
construct_house(house_style="cottage", location=[-1000, 1000, 0])
```

### create_tower
Build architectural towers with various styles and decorative elements.

**Parameters:**
- `height` (int): Number of vertical levels (default: 10)
- `base_size` (int): Base diameter/width (default: 4)
- `tower_style` (string): "cylindrical", "square", or "tapered"
- `block_size` (float): Size of building blocks in cm
- `location` (array): Tower base center position
- `mesh` (string): Static mesh for blocks
- `name_prefix` (string): Actor naming prefix

**Styles:**
- **Cylindrical**: Round tower with blocks in circular pattern
- **Square**: Hollow square tower with corner reinforcements  
- **Tapered**: Tower that narrows toward the top

**Example:**
```bash
create_tower(height=15, base_size=6, tower_style="cylindrical", location=[1000, 0, 0])
```

### create_arch
Create decorative arch structures using blocks arranged in semicircles.

**Parameters:**
- `radius` (float): Arch radius in centimeters (default: 300)
- `segments` (int): Number of blocks forming the arch (default: 6)
- `location` (array): Arch center base position
- `mesh` (string): Static mesh asset path
- `name_prefix` (string): Actor naming prefix

## 🧩 Level Design Tools

### create_maze
Generate solvable mazes using recursive backtracking algorithm.

**Parameters:**
- `rows` (int): Maze height in cells (default: 8)
- `cols` (int): Maze width in cells (default: 8)  
- `cell_size` (float): Size of each maze cell in cm (default: 300)
- `wall_height` (int): Height of walls in block layers (default: 3)
- `location` (array): Maze center position

**Features:**
- **Guaranteed Solvable**: Uses recursive backtracking for valid paths
- **Clear Entrance/Exit**: Marked with distinctive objects
- **Open Top Design**: Walls are limited height for aerial viewing
- **Connected Layout**: Every cell is reachable from the entrance; recursive backtracking may still create dead ends as part of the challenge

**Example:**
```bash
create_maze(rows=12, cols=12, wall_height=4, cell_size=250, location=[0, 0, 0])
```

### create_pyramid
Build stepped pyramids from stacked blocks.

**Parameters:**
- `base_size` (int): Number of blocks on base edge (default: 3)
- `block_size` (float): Edge length of each block in cm (default: 100)
- `location` (array): Pyramid base center
- `mesh` (string): Static mesh asset path
- `name_prefix` (string): Actor naming prefix

### create_wall
Generate straight walls from repeated block elements.

**Parameters:**
- `length` (int): Number of blocks along wall (default: 5)
- `height` (int): Number of block layers vertically (default: 2)
- `block_size` (float): Block dimensions in cm (default: 100)
- `location` (array): Wall starting position
- `orientation` (string): Direction to extend - "x" or "y"
- `mesh` (string): Static mesh asset path
- `name_prefix` (string): Actor naming prefix

### create_staircase
Build stepped staircases with configurable dimensions.

**Parameters:**
- `steps` (int): Number of steps (default: 5)
- `step_size` (array): [width, depth, height] of each step (default: [100, 100, 50])
- `location` (array): Staircase starting position
- `mesh` (string): Static mesh asset path
- `name_prefix` (string): Actor naming prefix

## ⚛️ Physics & Materials

### spawn_physics_blueprint_actor 
Create actors with custom physics properties and materials.

**Parameters:**
- `name` (string): Actor name (must be unique)
- `mesh_path` (string): Path to static mesh asset
- `location` (array): Spawn position (default: [0, 0, 0])
- `mass` (float): Physics mass in kg (default: 1.0)
- `simulate_physics` (bool): Enable physics simulation (default: true)
- `gravity_enabled` (bool): Enable gravity effects (default: true)

**Process:**
1. Creates temporary Blueprint class
2. Adds StaticMeshComponent with specified mesh
3. Configures physics properties
4. Compiles Blueprint and spawns actor

## 📦 Fab-Safe Asset Workflows

These tools work with local files and project content that is already available/licensed. They intentionally do not automate Fab authentication, purchases, or downloads.

### detect_fab_status / open_fab_browser
Detect whether Fab is installed/enabled, and open the editor Fab UI when available.

### import_asset / bulk_import_assets
Import already-downloaded local files into a `/Game/...` content path.

### place_imported_asset / tag_imported_assets
Place imported StaticMesh, SkeletalMesh, or Actor Blueprint assets in the level, then tag imported assets with editor metadata for later organization.

### move_asset / rename_asset / delete_asset / fix_redirectors
Organize imported/project assets and clean up redirectors after move/rename/delete operations.

### list_asset_dependencies / list_asset_references / generate_imported_asset_manifest
Inspect asset relationships and generate manifests for imported asset batches.

## 🏃 GASP / ALS Detection and Validation

### detect_gasp_assets / validate_motion_matching_setup
Detect Game Animation Sample Project-style content, Pose Search / Motion Matching prerequisites, and common validation issues.
Pose Search and IK Rig asset classes only resolve when their optional Unreal plugins are installed and enabled; missing plugins are reported as validation issues rather than installed automatically.

### setup_gasp_character / retarget_to_gasp
Wizard-style dry-run tools that validate prerequisites and report safe setup/retargeting actions.

### detect_als_plugin / validate_als_character
Detect ALS plugin variants/content and validate candidate ALS character or skeleton assets.

### setup_als_character / retarget_to_als
Wizard-style dry-run tools that validate prerequisites and report safe setup/retargeting actions.

## 🧹 Blueprint Cleanup and C++ Alternatives

### audit_blueprint
Inspect a Blueprint for maintainability issues such as disconnected nodes, dense graphs, and oversized variable sets.

### organize_blueprint_graph
Lay out a Blueprint graph in a predictable grid. Runs as a dry-run by default and can apply positions when requested.

### create_cpp_class
Generate safe Unreal C++ `.h` / `.cpp` class stubs for supported parent types: Actor, Character, Pawn, GameModeBase, PlayerController, ActorComponent, and UserWidget.

### validate_superhero_game_stack
Check common superhero-game prerequisites: Enhanced Input, GASP/Pose Search/Motion Warping, Gameplay Abilities, CommonUI, flight-related assets, menus, and customization assets.

### scaffold_superhero_cpp_classes
Preview or generate a starter C++ architecture for superhero projects: character, flight component, customization component, game mode, player controller, and menu widget.


## 🎨 Blueprint System

### create_blueprint
Create new Blueprint classes for custom actors.

**Parameters:**
- `name` (string): Blueprint name (must be unique)
- `parent_class` (string): Base class - typically "Actor"

### add_component_to_blueprint
Add components to existing Blueprint classes.

**Parameters:**
- `blueprint_name` (string): Target Blueprint name
- `component_type` (string): Component class name
- `component_name` (string): Name for the new component
- `location` (array): Relative position within Blueprint
- `rotation` (array): Relative rotation in degrees
- `scale` (array): Relative scale factors
- `component_properties` (object): Additional component settings

**Common Component Types:**
- `StaticMeshComponent`: 3D geometry rendering
- `CameraComponent`: Viewport and rendering cameras  
- `LightComponent`: Lighting sources
- `AudioComponent`: Sound playback

### set_static_mesh_properties
Configure mesh assets on StaticMeshComponents.

**Parameters:**
- `blueprint_name` (string): Blueprint containing the component
- `component_name` (string): StaticMeshComponent to modify
- `static_mesh` (string): Asset path to mesh (default: "/Engine/BasicShapes/Cube.Cube")

**Available Basic Meshes:**
- `/Engine/BasicShapes/Cube.Cube`
- `/Engine/BasicShapes/Sphere.Sphere`  
- `/Engine/BasicShapes/Cylinder.Cylinder`
- `/Engine/BasicShapes/Plane.Plane`

### set_physics_properties
Configure physics simulation parameters on components.

**Parameters:**
- `blueprint_name` (string): Blueprint containing component
- `component_name` (string): Component to configure
- `mass` (float): Object mass in kilograms (default: 1.0)
- `linear_damping` (float): Resistance to linear motion (default: 0.01)
- `angular_damping` (float): Resistance to rotation (default: 0.0)
- `simulate_physics` (bool): Enable physics simulation (default: true)
- `gravity_enabled` (bool): Enable gravity effects (default: true)

### set_mesh_material_color
Apply colored materials to mesh components.

**Parameters:**
- `blueprint_name` (string): Blueprint containing component
- `component_name` (string): StaticMeshComponent to color
- `color` (array): [R, G, B, A] values (0.0-1.0 range)
- `material_path` (string): Material asset path 
- `parameter_name` (string): Material parameter to modify

**Common Colors:**
- Red: `[1.0, 0.0, 0.0, 1.0]`
- Green: `[0.0, 1.0, 0.0, 1.0]` 
- Blue: `[0.0, 0.0, 1.0, 1.0]`
- Yellow: `[1.0, 1.0, 0.0, 1.0]`
- Purple: `[1.0, 0.0, 1.0, 1.0]`
- White: `[1.0, 1.0, 1.0, 1.0]`

### compile_blueprint
Compile Blueprint classes to apply changes.

**Parameters:**
- `blueprint_name` (string): Blueprint to compile

**Note:** Always compile Blueprints before spawning actors from them.

### spawn_blueprint_actor
Create actor instances from compiled Blueprint classes.

**Parameters:**
- `blueprint_name` (string): Source Blueprint class
- `actor_name` (string): Name for spawned actor
- `location` (array): World spawn position
- `rotation` (array): World rotation in degrees

## 🎯 Actor Management

### get_actors_in_level
List all actors currently in the level.

**Returns:** Array of actor information including names, types, and transforms.

### find_actors_by_name
Search for actors using name patterns.

**Parameters:**
- `pattern` (string): Search pattern (supports wildcards)

### spawn_actor  
Create basic actor types directly.

**Parameters:**
- `name` (string): Actor name
- `type` (string): Actor class name
- `location` (array): Spawn position (default: [0, 0, 0])
- `rotation` (array): Spawn rotation (default: [0, 0, 0])

**Common Types:**
- `StaticMeshActor`: Basic 3D objects
- `CameraActor`: Viewport cameras
- `LightActor`: Scene lighting

### delete_actor
Remove actors from the level.

**Parameters:**
- `name` (string): Name of actor to delete

### set_actor_transform
Modify actor position, rotation, and scale.

**Parameters:**
- `name` (string): Actor to transform
- `location` (array): New world position (optional)
- `rotation` (array): New rotation in degrees (optional)  
- `scale` (array): New scale factors (optional)

---

## 💡 Usage Tips

### Performance Optimization
- Use advanced composition tools instead of individual spawning
- Keep total actor counts reasonable (< 1000 actors)
- Use physics sparingly for better performance

### Naming Conventions
- Use descriptive, unique names for all actors
- Include prefixes for grouped objects (e.g., "House1_Wall", "House1_Roof")
- Avoid special characters in actor names

### Coordinate Guidelines  
- Place objects at Z > 0 to avoid ground clipping
- Use large separation distances for multiple structures
- Remember Unreal uses centimeters (100 = 1 meter)

### Blueprint Workflow
1. Create Blueprint class
2. Add required components  
3. Set component properties (mesh, physics, materials)
4. Compile Blueprint
5. Spawn actors from compiled Blueprint

---
