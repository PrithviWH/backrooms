"""
Backrooms Level 1 (backroomsLevel2.umap) Architecture Analyzer
Extracts spatial data, lighting info, and material usage from the map file
"""

import subprocess
import json
import os
import sys
from pathlib import Path

PROJECT_ROOT = r"c:\Users\Administrator\Documents\Unreal Projects\Backrooms 5.8"
UPROJECT_FILE = os.path.join(PROJECT_ROOT, "Backrooms.uproject")
LEVEL_PATH = "/Game/Backrooms/Maps/backroomsLevel2"
OUTPUT_FILE = os.path.join(PROJECT_ROOT, "LEVEL1_ANALYSIS_DATA.json")

def run_unreal_commandlet(commandlet_args):
    """Execute Unreal Engine commandlet and capture output"""
    ue_editor = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    
    cmd = [
        ue_editor,
        UPROJECT_FILE,
        "-run=ResavePackages",
        "-unattended",
        "-nopause",
        "-nosplash",
        "-silent"
    ] + commandlet_args
    
    print(f"Running: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )
        return result.stdout, result.stderr, result.returncode
    except subprocess.TimeoutExpired:
        print("ERROR: Commandlet timed out after 5 minutes")
        return "", "Timeout", -1
    except Exception as e:
        print(f"ERROR: {e}")
        return "", str(e), -1

def analyze_level_via_logs():
    """
    Alternative approach: Parse the .umap file metadata
    Since direct commandlet access requires editor startup, we'll use
    file analysis and pattern matching
    """
    
    level_file = os.path.join(PROJECT_ROOT, "Content", "Backrooms", "Maps", "backroomsLevel2.umap")
    
    if not os.path.exists(level_file):
        print(f"ERROR: Level file not found at {level_file}")
        return None
    
    file_size_mb = os.path.getsize(level_file) / (1024 * 1024)
    
    analysis = {
        "level_name": "backroomsLevel2.umap",
        "level_path": LEVEL_PATH,
        "file_size_mb": round(file_size_mb, 2),
        "last_modified": os.path.getmtime(level_file),
        "analysis_method": "file_metadata",
        "note": "Full actor-level analysis requires Unreal Editor Python API or manual inspection"
    }
    
    return analysis

def generate_analysis_script():
    """
    Generate a Python script that can be run INSIDE Unreal Editor
    via the Python Plugin
    """
    
    script_content = '''
"""
Run this script inside Unreal Editor via:
Window -> Developer Tools -> Python Console
Then: exec(open(r"c:\\Users\\Administrator\\Documents\\Unreal Projects\\Backrooms 5.8\\ue_level_analyzer.py").read())
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
    output_path = r"c:\\Users\\Administrator\\Documents\\Unreal Projects\\Backrooms 5.8\\LEVEL1_ANALYSIS_DATA.json"
    with open(output_path, 'w') as f:
        json.dump(analysis_data, f, indent=2)
    
    print(f"\\nAnalysis complete! Data saved to: {output_path}")
    print(f"Total Actors: {analysis_data['total_actors']}")
    print(f"Lights: {len(analysis_data['lights'])}")
    print(f"Static Meshes: {len(analysis_data['meshes'])}")
    print(f"Unique Materials: {len(analysis_data['materials'])}")
    print(f"Volumes: {len(analysis_data['volumes'])}")
'''
    
    script_path = os.path.join(PROJECT_ROOT, "ue_level_analyzer.py")
    with open(script_path, 'w') as f:
        f.write(script_content)
    
    print(f"\n{'='*80}")
    print("UNREAL EDITOR PYTHON SCRIPT GENERATED")
    print(f"{'='*80}")
    print(f"Location: {script_path}")
    print("\nTo analyze Level 1, follow these steps:")
    print("1. Open Unreal Editor (double-click Backrooms.uproject)")
    print("2. Enable Python Editor Script Plugin if not already enabled:")
    print("   Edit -> Plugins -> Search 'Python' -> Enable 'Python Editor Script Plugin'")
    print("3. Open backroomsLevel2.umap in the editor")
    print("4. Open Python Console: Window -> Developer Tools -> Output Log -> Python tab")
    print("5. Run this command:")
    print(f"   exec(open(r'{script_path}').read())")
    print("6. Wait for analysis to complete (outputs LEVEL1_ANALYSIS_DATA.json)")
    print(f"{'='*80}\n")

if __name__ == "__main__":
    print("Backrooms Level 1 Architecture Analyzer")
    print("=" * 80)
    
    # Try basic file analysis
    print("\n[1/2] Performing file-level analysis...")
    basic_analysis = analyze_level_via_logs()
    
    if basic_analysis:
        print(f"\nFile Analysis Results:")
        print(f"  File Size: {basic_analysis['file_size_mb']} MB")
        print(f"  Level Path: {basic_analysis['level_path']}")
    
    # Generate the in-editor analysis script
    print("\n[2/2] Generating Unreal Editor Python analysis script...")
    generate_analysis_script()
    
    print("\nBasic analysis complete. Run the generated script in Unreal Editor for full analysis.")
