"""Create Blend file from fbx/dtu

This script is used to create a blend file from fbx/dtu file which is exported from Daz Studio.

- Developed and tested with Blender 3.6 (Python 3.10) and Daz Studio 4.22

USAGE: blender.exe --background --python create_blend.py <fbx file>

EXAMPLE:

    blender.exe --background --python create_blend.py "C:/Users/username/Documents/DAZ 3D/DazToBlender/Export/Genesis8Female.fbx"

"""

TEXTURE_ATLAS_SIZE_DEFAULT = 1024

g_logfile = ""

def _print_usage():
    print("\nUSAGE: blender.exe --background --python create_blend.py <fbx file>\n")

from pathlib import Path
script_dir = str(Path(__file__).parent.absolute())

import sys
import os
import json
import re
import shutil
import mathutils

try:
    import bpy
except:
    print("DEBUG: blender python libraries not detected, continuing for pydoc mode.")

try:
    import blender_tools
    import game_readiness_tools
except:
    sys.path.append(script_dir)
    import blender_tools
    import game_readiness_tools

try:
    import DTB
    from DTB import *
    g_daz_addon_loaded = True
except:
    print("DEBUG: DazToBlender addon not detected, continuing in fallback mode.")
    g_daz_addon_loaded = False


def _add_to_log(message):
    if (g_logfile == ""):
        logfile = script_dir + "/create_blend.log"
    else:
        logfile = g_logfile

    print(str(message))
    with open(logfile, "a") as file:
        file.write(str(message) + "\n")

def _main(argv):
    try:
        line = str(argv[-1])
    except:
        _print_usage()
        return

    try:
        start, stop = re.search("#([0-9]*)\.", line).span(0)
        token_id = int(line[start+1:stop-1])
        print(f"DEBUG: token_id={token_id}")
    except:
        print(f"ERROR: unable to parse token_id from '{line}'")
        token_id = 0

    blender_tools.delete_all_items()
    blender_tools.switch_to_layout_mode()

    fbxPath = line.replace("\\","/").strip()
    if (not os.path.exists(fbxPath)):
        _add_to_log("ERROR: main(): fbx file not found: " + str(fbxPath))
        exit(1)
        return

    # prepare intermediate folder paths
    blenderFilePath = fbxPath.replace(".fbx", ".blend")
    intermediate_folder_path = os.path.dirname(fbxPath)

    if ("B_FIG" in fbxPath):
        jsonPath = fbxPath.replace("B_FIG.fbx", "FIG.dtu")
    elif ("B_ENV" in fbxPath):
        jsonPath = fbxPath.replace("B_ENV.fbx", "ENV.dtu")
    else:
        jsonPath = fbxPath.replace(".fbx", ".dtu")

    use_blender_tools = False
    output_blend_filepath = ""
    texture_atlas_mode = ""
    texture_atlas_size = 0
    export_rig_mode = ""
    enable_gpu_baking = False
    enable_embed_textures = False
    try:
        with open(jsonPath, "r") as file:
            json_obj = json.load(file)
        use_blender_tools = json_obj["Use Blender Tools"]
        output_blend_filepath = json_obj["Output Blend Filepath"]
        texture_atlas_mode = json_obj["Texture Atlas Mode"]
        texture_atlas_size = json_obj["Texture Atlas Size"]
        export_rig_mode = json_obj["Export Rig Mode"]
        enable_gpu_baking = json_obj["Enable GPU Baking"]
        enable_embed_textures = json_obj["Embed Textures"]
    except:
        print("ERROR: error occured while reading json file: " + str(jsonPath))

    if texture_atlas_size == 0:
        texture_atlas_size = TEXTURE_ATLAS_SIZE_DEFAULT

    if output_blend_filepath != "":
        blenderFilePath = output_blend_filepath

    if g_daz_addon_loaded and not use_blender_tools:

        # load DazToBlender addon
        _add_to_log("DEBUG: main(): loading DazToBlender addon")
        DTB.Global.bNonInteractiveMode = 1
        DTB.Global.nSceneScaleOverride = float(bpy.context.window_manager.scene_scale)

        sDtuFolderPath = os.path.dirname(jsonPath)
        oDtu = DTB.DataBase.DtuLoader()
        with open(jsonPath, "r") as data:
            oDtu.dtu_dict = json.load(data)

        DTB.Global.clear_variables()
        DTB.Global.setHomeTown(sDtuFolderPath)
        DTB.Global.load_asset_name()

        asset_type = oDtu.get_asset_type()
        if asset_type == "Environment" or asset_type == "StaticMesh":
            bpy.ops.import_dtu.env()
        else:
            bpy.ops.import_dtu.fig()

        DTB.Global.bNonInteractiveMode = 0

    else:
        # load FBX
        _add_to_log("DEBUG: main(): loading fbx file: " + str(fbxPath))
        blender_tools.import_fbx(fbxPath)

        blender_tools.center_all_viewports()
        _add_to_log("DEBUG: main(): loading json file: " + str(jsonPath))
        dtu_dict = blender_tools.process_dtu(jsonPath)

    debug_blend_file = fbxPath.replace(".fbx", "_debug.blend")
    bpy.ops.wm.save_as_mainfile(filepath=debug_blend_file)

    make_uv = True
    if texture_atlas_mode == "per_mesh":
        bake_quality = 1
        for obj in bpy.data.objects:
            if obj.type == 'MESH' and obj.visible_get():
                atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(obj, intermediate_folder_path, texture_atlas_size, bake_quality, make_uv, enable_gpu_baking)
    elif texture_atlas_mode == "single_atlas":
        texture_size = 2048
        bake_quality = 1
        # collect all meshes
        obj_list = []
        for obj in bpy.data.objects:
            if obj.type == 'MESH' and obj.visible_get():
                obj_list.append(obj)
        atlas, atlas_material, _ = game_readiness_tools.convert_to_atlas(obj_list, intermediate_folder_path, texture_atlas_size, bake_quality, make_uv, enable_gpu_baking)

    # remove missing or unused images
    print("DEBUG: deleting missing or unused images...")
    for image in bpy.data.images:
        is_missing = False
        if image.filepath:
            imagePath = bpy.path.abspath(image.filepath)
            if (not os.path.exists(imagePath)):
                is_missing = True

        is_unused = False
        if image.users == 0:
            is_unused = True

        if is_missing or is_unused:
            bpy.data.images.remove(image)

    # cleanup all unused and unlinked data blocks
    print("DEBUG: main(): cleaning up unused data blocks...")
    bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)

    # pack images
    if enable_embed_textures:
        print("DEBUG: packing images...")
        bpy.ops.file.pack_all()

    add_leaf_bones = False
    if export_rig_mode == "unreal" or export_rig_mode == "metahuman":
        # apply all transformations on armature
        for obj in bpy.data.objects:
            bpy.ops.object.select_all(action='DESELECT')
            if obj.type == 'ARMATURE':
                obj.select_set(True)
                bpy.context.view_layer.objects.active = obj
                bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if export_rig_mode == "mixamo":
        add_leaf_bones = True

    bpy.ops.wm.save_as_mainfile(filepath=blenderFilePath, )
    _add_to_log("DEBUG: main(): blend file saved: " + str(blenderFilePath))

    fbx_output_file_path = blenderFilePath.replace(".blend", ".fbx")
    try:
        bpy.ops.export_scene.fbx(filepath=fbx_output_file_path, 
                                 add_leaf_bones = add_leaf_bones,
                                 path_mode = "COPY",
                                 embed_textures = enable_embed_textures,
                                 use_visible = True,
                                 use_custom_props = True,
                                 )
        _add_to_log("DEBUG: save completed.")
    except Exception as e:
        _add_to_log("ERROR: unable to save Roblox FBX file: " + fbx_output_file_path)
        _add_to_log("EXCEPTION: " + str(e))

    return

# Execute main()
if __name__=='__main__':
    print("Starting script...")
    _add_to_log("Starting script... DEBUG: sys.argv=" + str(sys.argv))
    _main(sys.argv[4:])
    print("script completed.")
    exit(0)
