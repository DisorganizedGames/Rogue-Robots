import bpy

C = bpy.context
D = bpy.data

RELATIVE_ASSET_PATH = "//../../Rogue-Robots/Assets/Models/Modular Blocks/"

for img in D.images:
    fpath = img.filepath.strip('//')
    fname = fpath.split('\\')[-1].split('/')[-1]
    img.filepath = RELATIVE_ASSET_PATH + fname