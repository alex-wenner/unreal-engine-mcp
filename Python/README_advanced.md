# Unreal MCP Advanced Server

A feature-rich Unreal MCP server that exposes **75 tools** across editor actor control, Blueprint authoring, graph inspection/editing/cleanup, C++ class scaffolding, asset discovery/import, materials, physics, animation-system validation, and advanced world generation.

## What's Included

This server contains tools for advanced level building and composition plus lower-level editor automation:

### Essential Actor Management
- `get_actors_in_level()` - List all actors
- `find_actors_by_name(pattern)` - Find actors by pattern
- `spawn_actor(name, actor_type, location, rotation)` - Create basic actors
- `delete_actor(name)` - Remove actors
- `set_actor_transform(name, location, rotation, scale)` - Modify transforms

### Essential Blueprint Tools
*Minimal set needed for physics actors*
- `create_blueprint(name, parent_class)` - Create Blueprint classes
- `add_component_to_blueprint()` - Add components to Blueprints
- `set_static_mesh_properties()` - Set mesh properties
- `set_physics_properties()` - Configure physics
- `compile_blueprint(name)` - Compile Blueprint changes
- `spawn_physics_blueprint_actor()` - Create and spawn a Blueprint-backed physics actor

### Advanced Composition Tools
*The main focus - advanced building and composition tools from the merge request*
- `create_pyramid(base_size, block_size, location, ...)` - Build pyramids
- `create_wall(length, height, block_size, location, orientation, ...)` - Generate walls
- `create_tower(levels, block_size, location, ...)` - Stack towers
- `create_staircase(steps, step_size, location, ...)` - Build staircases
- `construct_house(width, depth, height, location, ...)` - **Enhanced** game-ready houses
- `create_arch(radius, segments, location, ...)` - Arch structures
- `spawn_physics_blueprint_actor (name, mesh_path, location, mass, ...)` - Physics objects
- `create_maze(rows, cols, cell_size, wall_height, location)` - Grid mazes

### Fab-Safe Asset and Animation Workflows
- `detect_fab_status()` / `open_fab_browser()` - Inspect/open Fab without automating auth, purchases, or downloads
- `import_asset()` / `bulk_import_assets()` - Import already-downloaded local files into project content
- `place_imported_asset()` / `tag_imported_assets()` - Use and organize imported StaticMesh, SkeletalMesh, or Actor Blueprint assets
- `move_asset()` / `rename_asset()` / `delete_asset()` / `fix_redirectors()` - Manage project assets safely after import
- `list_asset_dependencies()` / `list_asset_references()` / `generate_imported_asset_manifest()` - Inspect and summarize asset batches
- `detect_gasp_assets()` / `validate_motion_matching_setup()` - Detect GASP/Pose Search/Motion Matching prerequisites
- `detect_als_plugin()` / `validate_als_character()` - Detect ALS plugin/content prerequisites
- `setup_gasp_character()`, `retarget_to_gasp()`, `setup_als_character()`, `retarget_to_als()` - Dry-run wizard entry points for setup/retargeting workflows

### Blueprint Cleanup, C++ Alternatives, and Superhero Scaffolding
- `audit_blueprint()` - Find disconnected nodes, dense graphs, and maintainability issues
- `organize_blueprint_graph()` - Dry-run or apply predictable Blueprint graph layout
- `create_cpp_class()` - Generate safe Unreal C++ stubs for supported gameplay/framework classes
- `validate_superhero_game_stack()` - Check GASP movement, flight, menu, customization, and ability-system readiness
- `scaffold_superhero_cpp_classes()` - Preview or generate starter C++ classes for a superhero game

## Enhanced House Construction

The `construct_house` function has been significantly improved:

### Key Improvements:
- **Faster Spawning**: Uses large wall segments instead of individual blocks (20-30 actors vs 300+)
- **Realistic Proportions**: Default 12m x 10m x 6m house with proper room sizes
- **Smooth Walls**: Thin 20cm walls using scaled actors for clean appearance
- **Architectural Features**: 
  - Proper foundation and floor
  - Door opening (1.2m x 2.4m)
  - Window cutouts with proper placement
  - Pitched roof with realistic angle and overhang
  - Style-specific details (chimney, porch, garage)

### House Styles:
- **Modern**: Clean lines, garage door, flat details
- **Cottage**: Smaller size (80%), chimney, cozy proportions  
- **Mansion**: Larger size (150%), front porch with columns, chimney

### Example Usage:
```python
# Create a modern house
construct_house(house_style="modern")

# Create a cottage at specific location
construct_house(location=[1000, 0, 0], house_style="cottage")

# Create a large mansion
construct_house(width=1500, depth=1200, house_style="mansion")
```

## Not Yet Included

The server is designed to grow toward full editor parity. Current gaps include UMG/widget editing, viewport screenshots/camera control, Sequencer automation, project settings, and fully automatic GASP/ALS Blueprint mutation or IK retarget execution.

## Usage

Run the MCP server:

```bash
python unreal_mcp_server_advanced.py
```

## Benefits

- **Feature-rich**: 75 tools covering composition, Blueprint graph work/cleanup, C++ scaffolding, materials, asset discovery/import, and animation workflow validation
- **Focused**: Concentrates on editor automation for AI-assisted Unreal workflows
- **Faster**: Reduced startup time and smaller tool list
- **Maintainable**: Easier to understand and modify
- **Self-contained**: No external tool dependencies

The canonical tool manifest is the set of `@mcp.tool()` functions in `unreal_mcp_server_advanced.py`; tests can parse that manifest to catch documentation drift.
