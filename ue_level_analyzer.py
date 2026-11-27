
"""
Run this script inside Unreal Editor via:
Window -> Developer Tools -> Python Console
Then: exec(open(r"c:\Users\Administrator\Documents\Unreal Projects\Backrooms 5.8\ue_level_analyzer.py").read())
"""

import unreal
import json

# Load the level
level_path = "/Game/Backrooms/Maps/backroomsLevel2"
level_asset = unreal.EditorAssetLibrary.load_asset(level_path)

if not level_asset:
    print(f"ERROR: Could not load level {level_path}")
else:
    print(f"Successfully loaded: {level_path}")
    
    # Get all actors in the level
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    editor_level_library = unreal.EditorLevelLibrary()
    
    all_actors = editor_level_library.get_all_level_actors()
    
    analysis_data = {
        "level_name": "backroomsLevel2",
        "total_actors": len(all_actors),
        "actor_breakdown": {},
        "lights": [],
        "meshes": [],
        "materials": {},
        "volumes": [],
        "spatial_bounds": {}
    }
    
    # Categorize actors
    for actor in all_actors:
        actor_class = actor.get_class().get_name()
        
        if actor_class not in analysis_data["actor_breakdown"]:
            analysis_data["actor_breakdown"][actor_class] = 0
        analysis_data["actor_breakdown"][actor_class] += 1
        
        # Extract light data
        if "Light" in actor_class:
            light_component = actor.get_editor_property("light_component")
            if light_component:
                light_data = {
                    "type": actor_class,
                    "location": str(actor.get_actor_location()),
                    "intensity": light_component.get_editor_property("intensity"),
                    "color": str(light_component.get_editor_property("light_color")),
                    "attenuation_radius": light_component.get_editor_property("attenuation_radius") if hasattr(light_component, "attenuation_radius") else "N/A"
                }
                analysis_data["lights"].append(light_data)
        
        # Extract static mesh data
        if actor_class == "StaticMeshActor":
            mesh_component = actor.get_component_by_class(unreal.StaticMeshComponent)
            if mesh_component:
                mesh = mesh_component.get_editor_property("static_mesh")
                if mesh:
                    mesh_name = mesh.get_name()
                    mesh_data = {
                        "mesh_name": mesh_name,
                        "location": str(actor.get_actor_location()),
                        "scale": str(actor.get_actor_scale3d()),
                        "materials": []
                    }
                    
                    # Get materials
                    num_materials = mesh_component.get_num_materials()
                    for i in range(num_materials):
                        mat = mesh_component.get_material(i)
                        if mat:
                            mat_name = mat.get_name()
                            mesh_data["materials"].append(mat_name)
                            
                            # Track material usage
                            if mat_name not in analysis_data["materials"]:
                                analysis_data["materials"][mat_name] = 0
                            analysis_data["materials"][mat_name] += 1
                    
                    analysis_data["meshes"].append(mesh_data)
        
        # Extract volume data
        if "Volume" in actor_class:
            volume_data = {
                "type": actor_class,
                "location": str(actor.get_actor_location()),
                "bounds": str(actor.get_actor_bounds())
            }
            analysis_data["volumes"].append(volume_data)
    
    # Calculate spatial bounds
    if all_actors:
        min_x = min_y = min_z = float('inf')
        max_x = max_y = max_z = float('-inf')
        
        for actor in all_actors:
            loc = actor.get_actor_location()
            min_x = min(min_x, loc.x)
            min_y = min(min_y, loc.y)
            min_z = min(min_z, loc.z)
            max_x = max(max_x, loc.x)
            max_y = max(max_y, loc.y)
            max_z = max(max_z, loc.z)
        
        analysis_data["spatial_bounds"] = {
            "min": {"x": min_x, "y": min_y, "z": min_z},
            "max": {"x": max_x, "y": max_y, "z": max_z},
            "dimensions_cm": {
                "x": max_x - min_x,
                "y": max_y - min_y,
                "z": max_z - min_z
            }
        }
    
    # Save to file
    output_path = r"c:\Users\Administrator\Documents\Unreal Projects\Backrooms 5.8\LEVEL1_ANALYSIS_DATA.json"
    with open(output_path, 'w') as f:
        json.dump(analysis_data, f, indent=2)
    
    print(f"\nAnalysis complete! Data saved to: {output_path}")
    print(f"Total Actors: {analysis_data['total_actors']}")
    print(f"Lights: {len(analysis_data['lights'])}")
    print(f"Static Meshes: {len(analysis_data['meshes'])}")
    print(f"Unique Materials: {len(analysis_data['materials'])}")
    print(f"Volumes: {len(analysis_data['volumes'])}")
