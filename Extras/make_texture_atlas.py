from pathlib import Path
file_dir = Path(__file__).parent
parent_dir = file_dir.parent
scripts_dir = str(parent_dir.absolute())+"/Scripts"
print(f"DEBUG:\nfile_dir=\"{file_dir}\"\nscript_dir=\"{scripts_dir}\"")

import bpy
import sys

sys.path.append(file_dir)
sys.path.append(parent_dir)
sys.path.append(scripts_dir)

import game_readiness_tools

intermediate_folder_path = str(parent_dir)
texture_size = 2048
bake_quality = 1
bake_single = False

if bake_single:
    bake_list = []
    bpy.ops.object.select_all(action='DESELECT')
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.visible_get():
            obj.select_set(True)
            bake_list.append(obj)

    atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(bake_list, intermediate_folder_path, texture_size, bake_quality)

else:
    for obj in bpy.data.objects:
        bpy.ops.object.select_all(action='DESELECT')
        if obj.type == 'MESH' and obj.visible_get():
            obj.select_set(True)
            atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(obj, intermediate_folder_path, texture_size, bake_quality)
