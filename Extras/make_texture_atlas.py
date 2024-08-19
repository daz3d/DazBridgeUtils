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
        if obj.type == 'MESH':
            obj.select_set(True)
            bake_list.append(obj)

    atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(bake_list, intermediate_folder_path, texture_size, bake_quality)

    # new_uv_name = "AtlasUV"
    # new_uv = game_readiness_tools.create_new_uv_layer(bake_list, new_uv_name)
    # game_readiness_tools.repack_uv(bake_list)

    # for obj in bake_list:
    #     atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(obj, intermediate_folder_path, texture_size, bake_quality, False)

    # game_readiness_tools.switch_uv(bake_list, new_uv_name)
    # game_readiness_tools.remove_other_uvs(bake_list, new_uv_name)

else:
    for obj in bpy.data.objects:
        bpy.ops.object.select_all(action='DESELECT')
        if obj.type == 'MESH':
            obj.select_set(True)
            atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(obj, intermediate_folder_path, texture_size, bake_quality)
