import bpy
from bpy.types import Panel

C = bpy.context
D = bpy.data
O = bpy.ops
PI = 3.141592653589793

#######################################################
#                LEVEL BUILDER UI PANELS
#######################################################



class PANEL_PT_builder(Panel):
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_idname = "PANEL_PT_builder"
    bl_label = "Level Builder"
    bl_category = "Level Builder"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def draw(self, context):
        pass

class PANEL_PT_builder_grid(PANEL_PT_builder, Panel):
    bl_idname = "PANEL_PT_builder_grid"
    bl_label = "Grid"

    def draw(self, context):
        layout = self.layout
        layout.row().operator("object.grid_toggle_xray")
        settings = D.objects['grid']
        layout.row().label(text="Grid settings:")
        layout.row().prop(settings, "location")
        layout.row().prop(settings, "scale")
        layout.row().operator("object.create_grid")

class PANEL_PT_builder_brush(PANEL_PT_builder, Panel):
    bl_idname = "PANEL_PT_builder_brush"
    bl_label = "Brush"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        row = layout.row()
        if len(D.collections['Brush'].objects) != 1:
            row.label(text="Select a block as brush")
        else:
            b = D.collections['Brush'].objects[0]
            row.label(text="Brush: " + b.name)
        if context.object.name in D.collections['Collection'].objects.keys():
            row = layout.row()
            row.operator("object.select_block")

class PANEL_PT_builder_paint(PANEL_PT_builder, Panel):
    bl_idname = "PANEL_PT_builder_paint"
    bl_label = "Painter"

    def draw(self, context):
        layout = self.layout
        if (context.object.name in D.collections['Grid'].objects.keys()) and (len(D.collections['Brush'].objects) == 1):
            obj = context.object
            row = layout.row()
            row.label(text="Place at " + obj.name)
            row = layout.row()
            row.operator("object.add_block")
            row = layout.row()
            row.operator("object.rotate_block")
            row = layout.row()
            row.operator("object.flip_block")
        else:
            row = layout.row()
            row.label(text="Select cubes in grid to draw map")
        row = layout.row()
        row.operator("object.clear_map")

class PANEL_PT_builder_analyze(PANEL_PT_builder, Panel):
    bl_idname = "PANEL_PT_builder_analyze"
    bl_label = "Analyze"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.operator("object.analyze_map")


#######################################################
#                OPERATORS
#######################################################

class GridToggleXray(bpy.types.Operator):
    """Toggles X-ray on selected grid tiles"""
    bl_idname = "object.grid_toggle_xray"
    bl_label = "Toggle X-ray"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        for obj in D.collections['Settings'].objects:
            if obj.name.split('_')[0] == 'selector':
                obj.hide_viewport = not obj.hide_viewport
        return {'FINISHED'}

class CreateGrid(bpy.types.Operator):
    """Generate the base grid for the map"""
    bl_idname = "object.create_grid"
    bl_label = "Create Grid"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        generate_grid()
        return {'FINISHED'}


class AddBlock(bpy.types.Operator):
    """Add a block at selected position"""
    bl_idname = "object.add_block"
    bl_label = "Add Block"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        slots = context.selected_objects
        gizmo = context.object
        O.object.select_all(action='DESELECT')
        map = D.collections['Map']
        trash = D.collections['Trash']
        brush = D.collections['Brush'].objects[0]
        for giz in slots:
            for obj in giz.children:
                for coll in obj.users_collection:
                    coll.objects.unlink(obj)
                trash.objects.link(obj)
                obj.parent = None
            context.view_layer.objects.active = brush
            brush.select_set(True)
            O.object.duplicate()
            block = context.object
            for col in block.users_collection:
                col.objects.unlink(block)
            map.objects.link(block)
            block.location = (0,0,0)
            block.parent = giz
            block.hide_select = True
            block.name = block.name.split('.')[0] + "_r0_." + giz.name.split('_')[1]
        empty_collection(D.collections['Trash'])
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

class RotateBlock(bpy.types.Operator):
    """Rotate selected blocks 90 degrees"""
    bl_idname = "object.rotate_block"
    bl_label = "Rotate Block"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        slots = context.selected_objects
        gizmo = context.object
        O.object.select_all(action='DESELECT')
        for giz in slots:
            if len(giz.children) == 1:
                obj = giz.children[0]
                name, rot, flip = obj.name.split('.')[0].split('_')
                if flip == "":
                    obj.hide_select = False
                    context.view_layer.objects.active = obj
                    obj.select_set(True)
                    obj.rotation_euler.rotate_axis('Z', PI/2)
                    O.object.transform_apply(properties=False)
                    obj.hide_select = True
                    rot = (int(rot.split('r')[1]) + 1) % 4
                    obj.name = f"{name}_r{rot}_{flip}.{giz.name.split('_')[1]}"
                    obj.hide_select = True
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

class FlipBlock(bpy.types.Operator):
    """Flip selected blocks about x-axix"""
    bl_idname = "object.flip_block"
    bl_label = "Flip Block"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        invert = {'f': '', '': 'f'}
        slots = context.selected_objects
        gizmo = context.object
        O.object.select_all(action='DESELECT')
        for giz in slots:
            if len(giz.children) == 1:
                obj = giz.children[0]
                obj.hide_select = False
                context.view_layer.objects.active = obj
                obj.select_set(True)
                s = obj.scale.x
                n = obj.name
                obj.scale.x = obj.scale.x * (-1)
                O.object.transform_apply(properties=False)
                obj.hide_select = True
                name, rot, flip = obj.name.split('.')[0].split('_')
                flip = invert[flip]
                obj.name = f"{name}_{rot}_{flip}.{giz.name.split('_')[1]}"
                obj.hide_select = True
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

class ClearMap(bpy.types.Operator):
    """Removes blocks in selection"""
    bl_idname = "object.clear_map"
    bl_label = "Clear Selection"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        slots = context.selected_objects
        gizmo = context.object
        O.object.select_all(action='DESELECT')
        map = D.collections['Map']
        trash = D.collections['Trash']
        for giz in slots:
            for obj in giz.children:
                for coll in obj.users_collection:
                    coll.objects.unlink(obj)
                trash.objects.link(obj)
                obj.parent = None
        empty_collection(D.collections['Trash'])
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

class SelectBlock(bpy.types.Operator):
    """Select building block"""
    bl_idname = "object.select_block"
    bl_label = "Select Block"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        for brush in D.collections['Brush'].objects:
            D.collections['Brush'].objects.unlink(brush)
        D.collections['Brush'].objects.link(context.object)
        return {'FINISHED'}

class AnalyzeMap(bpy.types.Operator):
    """Analyzes map and generates seed file"""
    bl_idname = "object.analyze_map"
    bl_label = "Analyze Map"
    
    @classmethod
    def poll(cls, context):
        return context.active_object is not None
    
    def execute(self, context):
        map_analysis()
        return {'FINISHED'}


#######################################################
#                HELPER FUNCTIONS
#######################################################

def generate_grid():
    settings = D.objects['grid']
    size_x = int(settings.location.x)
    settings.location.x = size_x
    size_y = int(settings.location.y)
    settings.location.y = size_y
    size_z = int(settings.location.z)
    settings.location.z = size_z
    fx = settings.scale.x
    fy = settings.scale.y
    fz = settings.scale.z
    empty_collection(D.collections['Map'])
    empty_collection(D.collections['Grid'])
    empty_collection(D.collections['Settings'], [settings])
    empty_collection(D.collections['Trash'])

    # Create selectors
    C.view_layer.active_layer_collection = \
    C.view_layer.layer_collection.children['Settings']
    for i, axis in enumerate(['x', 'y', 'z']):
        for k, end in enumerate(['l', 'h']):
            sel = 'selector_' + end + '-' + axis
            if not D.objects.get(sel):
                s = D.objects['grid']
                loc = s.location
                scl = s.scale
                O.object.empty_add(type="CIRCLE", radius=max(loc) * max(scl))
                obj = C.object
                obj.name = sel
                adj = [0.5, 0.5, 0.5]
                fac = [-0.3, 1.3]
                adj[i] = fac[k]
                obj.location = (adj[0] * loc.x * scl.x, adj[1] * loc.y * scl.y, adj[2] * loc.z * scl.z)
                if i != 1:
                    ax = ['Z', 'Z', 'X']
                    obj.rotation_euler.rotate_axis(ax[i], PI/2)
                lock = [True, True, True]
                lock[i] = False
                for j in range(3):
                    obj.lock_location[j] = lock[j]
                    obj.lock_rotation[j] = True
                    obj.lock_scale[j] = True
                obj.hide_viewport = True

    # Create grid
    C.view_layer.active_layer_collection = \
    C.view_layer.layer_collection.children['Grid']
    for z in range(size_z):
        for x in range(size_x):
            for y in range(size_y):
                O.object.empty_add(location=((x+0.5)*fx, (y+0.5)*fy, z*fz), radius=0.1)
                gizmo = C.object
                gizmo.name = f"block_{x}-{y}-{z}"
                gizmo.show_in_front = True
                # Create drivers
                drv = gizmo.driver_add("hide_viewport").driver
                expr = []
                for ax in ['x', 'y', 'z']:
                    var = drv.variables.new()
                    var.name = ax
                    var.targets[0].id = gizmo
                    var.targets[0].data_path = f"location.{ax}"
                    for lh in ['l', 'h']:
                        var = drv.variables.new()
                        var.name = f"{lh}{ax}"
                        var.targets[0].id = D.objects[f"selector_{lh}-{ax}"]
                        var.targets[0].data_path = f"location.{ax}"
                    expr.append(f"({ax} < l{ax} or h{ax} < {ax})")
                drv.expression = " or ".join(expr)
                for i in range(3):
                    gizmo.lock_location[i] = True
                    gizmo.lock_rotation[i] = True
                    gizmo.lock_scale[i] = True


def all_blocks():
   for blk in D.collections['Grid'].objects:
        name, pos = blk.name.split('_')
        if name == 'block':
            x, y, z = pos.split('-')
            x = int(x)
            y = int(y)
            z = int(z)
            if len(blk.children) == 1:
                name = blk.children[0].name.split('.')[0]
            else:
                name = "Empty"
            yield (x, y, z), name

def get_block(x, y, z):
    if x < 0 or y < 0 or z < 0:
        return "Edge"
    max_x = D.objects['grid'].location.x
    max_y = D.objects['grid'].location.y
    max_z = D.objects['grid'].location.z
    if x > max_x or y > max_y or z > max_z:
        return "Edge"
    blk_name = f"block_{x}-{y}-{z}"
    blk = D.objects[blk_name]
    if len(blk.children) == 0:
        return "Empty"
    else:
        return blk.children[0].name.split('.')[0]

def empty_collection(coll, ex=[]):
    O.object.select_all(action='DESELECT')
    for obj in coll.objects:
        obj.hide_viewport = False
        obj.hide_select = False
    O.object.select_same_collection(collection=coll.name)
    for obj in ex:
        obj.hide_viewport = True
    O.object.delete()

def menu_func(self, context):
    self.layout.operator(GridToggleXray.bl_idname, text=GridToggleXray.bl_label)
    self.layout.operator(T.bl_idname, text=CreateGrid.bl_label)
    self.layout.operator(AddBlock.bl_idname, text=AddBlock.bl_label)
    self.layout.operator(RotateBlock.bl_idname, text=RotateBlock.bl_label)
    self.layout.operator(FlipBlock.bl_idname, text=FlipBlock.bl_label)
    self.layout.operator(ClearMap.bl_idname, text=ClearMap.bl_label)
    self.layout.operator(SelectBlock.bl_idname, text=SelectBlock.bl_label)
    self.layout.operator(AnalyzeMap.bl_idname, text=AnalyzeMap.bl_label)


# Register and add to the "object" menu (required to also use F3 search "Simple Object Operator" for quick access).
def register():
    bpy.utils.register_class(CreateGrid)
    bpy.utils.register_class(GridToggleXray)
    bpy.utils.register_class(AddBlock)
    bpy.utils.register_class(RotateBlock)
    bpy.utils.register_class(FlipBlock)
    bpy.utils.register_class(ClearMap)
    bpy.utils.register_class(SelectBlock)
    bpy.utils.register_class(AnalyzeMap)
    bpy.types.VIEW3D_MT_object.append(menu_func)

    bpy.utils.register_class(PANEL_PT_builder_brush)
    bpy.utils.register_class(PANEL_PT_builder_paint)
    bpy.utils.register_class(PANEL_PT_builder_grid)
    bpy.utils.register_class(PANEL_PT_builder_analyze)

def unregister():
    bpy.utils.unregister_class(CreateGrid)
    bpy.utils.unregister_class(GridToggleXray)
    bpy.utils.unregister_class(AddBlock)
    bpy.utils.unregister_class(RotateBlock)
    bpy.utils.unregister_class(FlipBlock)
    bpy.utils.unregister_class(ClearMap)
    bpy.utils.unregister_class(SelectBlock)
    bpy.utils.unregister_class(AnalyzeMap)
    bpy.types.VIEW3D_MT_object.remove(menu_func)

    bpy.utils.unregister_class(PANEL_PT_builder_brush)
    bpy.utils.unregister_class(PANEL_PT_builder_paint)
    bpy.utils.unregister_class(PANEL_PT_builder_grid)
    bpy.utils.unregister_class(PANEL_PT_builder_analyze)


#######################################################
#                MAP ANALYSIS ALGORITHM
#######################################################


def map_analysis():
        print("="*5, " Analysis ", "="*5)
        for (x, y, z), block_id in all_blocks():
#        for pos, block_id in all_blocks():
            print(f"{str((x, y, z)):15} {block_id}, {get_block(x-1, y-1, z-1)}")



#######################################################
#                MAIN
#######################################################


if __name__ == "__main__":
    if not D.collections.get('Map'):
        map = D.collections.new("Map")
        C.scene.collection.children.link(map)

    if not D.collections.get('Grid'):
        grid = D.collections.new("Grid")
        C.scene.collection.children.link(grid)

    if not D.collections.get('Settings'):
        grid = D.collections.new("Settings")
        C.scene.collection.children.link(grid)

    if not D.objects.get('grid'):
        C.view_layer.active_layer_collection = \
        C.view_layer.layer_collection.children['Settings']
        O.object.empty_add()
        settings = C.object
        settings.name = "grid"
        settings.location = (5, 5, 3)
        settings.scale = (2, 2, 2)
        settings.hide_viewport = True


    if not D.collections.get('Trash'):
        trash = D.collections.new("Trash")
        C.scene.collection.children.link(trash)

    if not D.collections.get('Brush'):
        grid = D.collections.new("Brush")
        C.scene.collection.children.link(grid)
    
    if not D.objects.get('block_0-0-0'):
        generate_grid()
    register()

