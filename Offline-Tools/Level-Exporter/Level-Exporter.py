import bpy
from bpy.types import Panel

C = bpy.context
D = bpy.data
O = bpy.ops
PI = 3.141592653589793


#######################################################
#                LEVEL BUILDER UI PANELS
#######################################################


# Base class for the builder panel
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

# Grid settings panel
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

# Panel to choose your block to paint with
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
        if D.collections['Brushes'].hide_viewport:
            row = layout.row()
            row.operator("object.select_brush")
        else:
            row = layout.row()
            row.operator("object.set_brush")

# Panel with actions to edit the map
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
            row.operator("object.flip_block_x")
            row = layout.row()
            row.operator("object.flip_block_y")
        else:
            row = layout.row()
            row.label(text="Select cubes in grid to draw map")
        row = layout.row()
        row.operator("object.clear_map")

# The export/analysis button
class PANEL_PT_builder_analyze(PANEL_PT_builder, Panel):
    bl_idname = "PANEL_PT_builder_analyze"
    bl_label = "Analyze"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.operator("object.analyze_map")

# Selected blocks
class PANEL_PT_builder_selected(PANEL_PT_builder, Panel):
    bl_idname = "PANEL_PT_builder_selected"
    bl_label = "Map Selection"

    def draw(self, context):
        layout = self.layout
        if not D.collections['Grid'].hide_viewport:
            if len(context.active_object.children) > 0:
                row = layout.row()
                row.label(text=f"{context.active_object.children[0].name}")
            for gizmo in context.selected_objects:
                if len(gizmo.children) > 0 and not gizmo == context.active_object:
                    row = layout.row()
                    row.label(text=f"     {gizmo.children[0].name}")


#######################################################
#                OPERATORS
#######################################################

# Operator for toggling visibility of grid tiles
class GridToggleXray(bpy.types.Operator):
    """Selectors to toggle visibility of grid positions"""
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

# Generate the grid
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

# Adding blocks to the map
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
        brush.hide_select = False
        for giz in slots:
            for obj in giz.children:
                for coll in obj.users_collection:
                    coll.objects.unlink(obj)
                trash.objects.link(obj)
                obj.parent = None
            context.view_layer.objects.active = brush
            brush.select_set(True)
            if O.object.duplicate() == {'FINISHED'}:
                block = context.object
                for col in block.users_collection:
                    col.objects.unlink(block)
                map.objects.link(block)
                block.location = (0,0,0)
                block.parent = giz
                block.hide_select = True
                block.name = block.name.split('.')[0] + "_r0_f." + giz.name.split('_')[1]
            else:
                print(f"Couldn't duplicate {brush.name}")
        brush.hide_select = True
        empty_collection(D.collections['Trash'])
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

# Rotating blocks
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
                # if flip == "":
                obj.hide_select = False
                context.view_layer.objects.active = obj
                obj.select_set(True)
                obj.rotation_euler.rotate_axis('Z', PI/2)
                # O.object.transform_apply(properties=False)
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

# Flip blocks over x-axis
class FlipBlockX(bpy.types.Operator):
    """Flip selected blocks over x-axix"""
    bl_idname = "object.flip_block_x"
    bl_label = "Flip Block X"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        invert = {'f': 'fx', 'fx': 'f', 'fy': 'fxy', 'fxy': 'fy'}
        slots = context.selected_objects
        gizmo = context.object
        O.object.select_all(action='DESELECT')
        for giz in slots:
            if len(giz.children) == 1:
                obj = giz.children[0]
                obj.hide_select = False
                context.view_layer.objects.active = obj
                obj.select_set(True)
                obj.scale.x = obj.scale.x * (-1)
                # O.object.transform_apply(properties=False)
                obj.hide_select = True
                name, rot, flip = obj.name.split('.')[0].split('_')
                flip = invert[flip]
                if flip == 'fxy':
                    rot = f"r{(int(rot.split('r')[1]) + 2) % 4}"
                    flip = 'f'
                obj.name = f"{name}_{rot}_{flip}.{giz.name.split('_')[1]}"
                obj.hide_select = True
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

# Flip blocks over x-axis
class FlipBlockY(bpy.types.Operator):
    """Flip selected blocks over y-axix"""
    bl_idname = "object.flip_block_y"
    bl_label = "Flip Block Y"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        invert = {'f': 'fy', 'fy': 'f', 'fx': 'fxy', 'fxy': 'fx'}
        slots = context.selected_objects
        gizmo = context.object
        O.object.select_all(action='DESELECT')
        for giz in slots:
            if len(giz.children) == 1:
                obj = giz.children[0]
                obj.hide_select = False
                context.view_layer.objects.active = obj
                obj.select_set(True)
                obj.scale.y = obj.scale.y * (-1)
                # O.object.transform_apply(properties=False)
                obj.hide_select = True
                name, rot, flip = obj.name.split('.')[0].split('_')
                flip = invert[flip]
                if flip == 'fxy':
                    rot = f"r{(int(rot.split('r')[1]) + 2) % 4}"
                    flip = 'f'
                obj.name = f"{name}_{rot}_{flip}.{giz.name.split('_')[1]}"
                obj.hide_select = True
        O.object.select_all(action='DESELECT')
        for obj in slots:
            obj.select_set(True)
        gizmo.select_set(True)
        context.view_layer.objects.active = gizmo
        return {'FINISHED'}

# Remove blocks from selected grid elements
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

# Open the select brush dialog
class SelectBrush(bpy.types.Operator):
    """Display all brushes"""
    bl_idname = "object.select_brush"
    bl_label = "Display Brushes"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        save_selection(context)
        O.object.select_all(action='DESELECT')
        D.collections['Brushes'].hide_viewport = False
        D.collections['Map'].hide_viewport = True
        D.collections['Grid'].hide_viewport = True
        for obj in D.collections['Brushes'].objects:
            obj.hide_select = False
        context.view_layer.objects.active = D.collections['Brush'].objects[0]
        D.collections['Brush'].objects[0].select_set(True)
        return {'FINISHED'}

# Activate selected map element as current brush
class SetBrush(bpy.types.Operator):
    """Select building block"""
    bl_idname = "object.set_brush"
    bl_label = "Set Brush"

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        for brush in D.collections['Brush'].objects:
            D.collections['Brush'].objects.unlink(brush)
        D.collections['Brush'].objects.link(context.object)
        O.object.select_all(action='DESELECT')
        for obj in D.collections['Brushes'].objects:
            obj.hide_select = True
        context.view_layer.objects.active = None
        D.collections['Brushes'].hide_viewport = True
        D.collections['Map'].hide_viewport = False
        D.collections['Grid'].hide_viewport = False
        restore_selection(context)
        return {'FINISHED'}

# Calls the map_analysis() function
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

def remove_block(x, y, z):
    name = f"block_{x}-{y}-{z}"
    obj = D.objects[name]
    for child in obj.children:
        child.hide_select = False
        for coll in child.users_collection:
            coll.objects.unlink(child)
        D.collections['Trash'].objects.link(child)
    obj.hide_select = False
    for coll in obj.users_collection:
        coll.objects.unlink(obj)
    D.collections['Trash'].objects.link(obj)

def generate_grid():
    # save_selection()
    new_settings = D.objects['grid']
    current = D.objects['current_grid_settings']

    new_x = int(new_settings.location.x)
    new_settings.location.x = new_x
    new_y = int(new_settings.location.y)
    new_settings.location.y = new_y
    new_z = int(new_settings.location.z)
    new_settings.location.z = new_z
    old_x = int(current.location.x)
    old_y = int(current.location.y)
    old_z = int(current.location.z)
    current.location = new_settings.location
    current.scale = new_settings.scale

    viewport_state = D.collections['Grid'].hide_viewport
    D.collections['Grid'].hide_viewport = False

    if new_x < old_x or new_y < old_y or new_z < old_z:
        # remove superflous blocks
        x_rng = range(new_x, old_x)
        y_rng = range(new_y, old_y)
        z_rng = range(new_z, old_z)
        if len(x_rng) == 0:
            x_rng = [old_x - 1]
        else:
            for z in range(old_z):
                for x in x_rng:
                    for y in range(old_y):
                        remove_block(x, y, z)
        if len(y_rng) == 0:
            y_rng = [old_y - 1]
        else:
            for z in range(old_z):
                for x in range(old_x):
                    for y in y_rng:
                        remove_block(x, y, z)
        if len(z_rng) == 0:
            z_rng = [old_z - 1]
        else:
            for z in z_rng:
                for x in range(old_x):
                    for y in range(old_y):
                        remove_block(x, y, z)
        for z in z_rng:
            for x in x_rng:
                for y in y_rng:
                    remove_block(x, y, z)

    if new_x != old_x or new_y != old_y or new_z != old_z:
        empty_collection(D.collections['Trash'])
        # Create selectors
        C.view_layer.active_layer_collection = \
        C.view_layer.layer_collection.children['Settings']
        for i, axis in enumerate(['x', 'y', 'z']):
            for k, end in enumerate(['l', 'h']):
                sel = 'selector_' + end + '-' + axis
                s = D.objects['grid']
                loc = s.location
                scl = s.scale
                obj = D.objects.get(sel)
                if not obj:
                    O.object.empty_add(type="CIRCLE", radius=max(loc) * max(scl))
                    obj = C.object
                    obj.name = sel
                    if i != 1:
                        ax = ['Z', 'Z', 'X']
                        obj.lock_rotation 
                        obj.rotation_euler.rotate_axis(ax[i], PI/2)
                else:
                    obj.hide_viewport = False
                    obj.empty_display_size = max(loc) * max(scl)
                for j in range(3):
                    obj.lock_location[j] = False
                adj = [0.5, 0.5, 0.5]
                fac = [-0.3, 1.3]
                adj[i] = fac[k]
                obj.location = (adj[0] * loc.x * scl.x, adj[1] * loc.y * scl.y, adj[2] * loc.z * scl.z)
                lock = [True, True, True]
                lock[i] = False
                for j in range(3):
                    obj.lock_location[j] = lock[j]
                    obj.lock_rotation[j] = True
                    obj.lock_scale[j] = True
                obj.hide_viewport = True

    if old_x < new_x or old_y < new_y or old_z < new_z:
        # Add blocks to grid
        C.view_layer.active_layer_collection = \
        C.view_layer.layer_collection.children['Grid']
        x_rng = range(old_x, new_x)
        y_rng = range(old_y, new_y)
        z_rng = range(old_z, new_z)
        if len(x_rng) == 0:
            x_rng = [new_x - 1]
        else:
            for z in range(old_z):
                for x in x_rng:
                    for y in range(old_y):
                        add_block(x, y, z)
        if len(y_rng) == 0:
            y_rng = [new_y - 1]
        else:
            for z in range(old_z):
                for x in range(old_x):
                    for y in y_rng:
                        add_block(x, y, z)
        for z in range(old_z):
            for x in x_rng:
                for y in y_rng:
                    add_block(x, y, z)
        if len(z_rng) > 0:
            for z in z_rng:
                for x in range(new_x):
                    for y in range(new_y):
                        add_block(x, y, z)
    
    D.collections['Grid'].hide_viewport = viewport_state
    # restore_selection()

def add_block(x, y, z):
    name = f"block_{x}-{y}-{z}"
    # if not D.objects.get(name):
    if D.objects.get(name):
        print(f"Warning: {(x, y, x)} exists")
    else:
        fx = D.objects['grid'].scale.x
        fy = D.objects['grid'].scale.y
        fz = D.objects['grid'].scale.z
        O.object.empty_add(location=((x+0.5)*fx, (y+0.5)*fy, z*fz), radius=0.1)
        gizmo = C.object
        gizmo.name = name
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


# Generator that loops over entire grid, yielding one element at at time
def all_blocks():
    """Yields position and name of all building blocks in grid"""
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

# Get block info at position (x, y, z)
def get_block(x, y, z):
    """Returns block info at position (x, y, z)"""
    if x < 0 or y < 0 or z < 0:
        return "Edge"
    max_x = int(D.objects['grid'].location.x)
    max_y = int(D.objects['grid'].location.y)
    max_z = int(D.objects['grid'].location.z)
    if x >= max_x or y >= max_y or z >= max_z:
        return "Edge"
    blk_name = f"block_{x}-{y}-{z}"
    blk = D.objects[blk_name]
    if len(blk.children) == 0:
        return "Empty"
    else:
        return blk.children[0].name.split('.')[0]

def empty_collection(coll, exclude=[]):
    # save_selection()
    O.object.select_all(action='DESELECT')
    coll_state = coll.hide_viewport
    coll.hide_viewport = False
    for obj in coll.objects:
        obj.hide_viewport = False
        obj.hide_select = False
    O.object.select_same_collection(collection=coll.name)
    for obj in exclude:
        obj.hide_viewport = True
    O.object.delete()
    coll.hide_viewport = coll_state
    # restore_selection()

def save_selection(context=C):
    # First empty selection
    for obj in D.collections['Object_Selection'].objects:
        D.collections['Object_Selection'].objects.unlink(obj)
    # ... and active object
    for obj in D.collections['Active_Object'].objects:
        D.collections['Active_Object'].objects.unlink(obj)
    # then save selection and active object
    for obj in context.selected_objects:
        D.collections['Object_Selection'].objects.link(obj)
    for obj in D.collections['Active_Object'].objects:
        D.collections['Active_Object'].objects.link(obj)
    

def restore_selection(context=C):
    O.object.select_all(action='DESELECT')
    O.object.select_same_collection(collection='Object_Selection')
    for obj in D.collections['Object_Selection'].objects:
        D.collections['Object_Selection'].objects.unlink(obj)
    for i, obj in enumerate(D.collections['Active_Object'].objects):
        context.view_layer.objects.active = obj             # last object in collection will become active
        D.collections['Active_Object'].objects.unlink(obj)


def menu_func(self, context):
    self.layout.operator(GridToggleXray.bl_idname, text=GridToggleXray.bl_label)
    self.layout.operator(CreateGrid.bl_idname, text=CreateGrid.bl_label)
    self.layout.operator(AddBlock.bl_idname, text=AddBlock.bl_label)
    self.layout.operator(RotateBlock.bl_idname, text=RotateBlock.bl_label)
    self.layout.operator(FlipBlockX.bl_idname, text=FlipBlockX.bl_label)
    self.layout.operator(FlipBlockY.bl_idname, text=FlipBlockY.bl_label)
    self.layout.operator(ClearMap.bl_idname, text=ClearMap.bl_label)
    self.layout.operator(SelectBrush.bl_idname, text=SelectBrush.bl_label)
    self.layout.operator(SetBrush.bl_idname, text=SetBrush.bl_label)
    self.layout.operator(AnalyzeMap.bl_idname, text=AnalyzeMap.bl_label)


# Register and add to the "object" menu (required to also use F3 search "Simple Object Operator" for quick access).
def register():
    bpy.utils.register_class(CreateGrid)
    bpy.utils.register_class(GridToggleXray)
    bpy.utils.register_class(AddBlock)
    bpy.utils.register_class(RotateBlock)
    bpy.utils.register_class(FlipBlockX)
    bpy.utils.register_class(FlipBlockY)
    bpy.utils.register_class(ClearMap)
    bpy.utils.register_class(SelectBrush)
    bpy.utils.register_class(SetBrush)
    bpy.utils.register_class(AnalyzeMap)
    bpy.types.VIEW3D_MT_object.append(menu_func)

    bpy.utils.register_class(PANEL_PT_builder_brush)
    bpy.utils.register_class(PANEL_PT_builder_paint)
    bpy.utils.register_class(PANEL_PT_builder_grid)
    bpy.utils.register_class(PANEL_PT_builder_analyze)
    bpy.utils.register_class(PANEL_PT_builder_selected)

def unregister():
    bpy.utils.unregister_class(CreateGrid)
    bpy.utils.unregister_class(GridToggleXray)
    bpy.utils.unregister_class(AddBlock)
    bpy.utils.unregister_class(RotateBlock)
    bpy.utils.unregister_class(FlipBlockX)
    bpy.utils.unregister_class(FlipBlockY)
    bpy.utils.unregister_class(ClearMap)
    bpy.utils.unregister_class(SelectBrush)
    bpy.utils.unregister_class(SetBrush)
    bpy.utils.unregister_class(AnalyzeMap)
    bpy.types.VIEW3D_MT_object.remove(menu_func)

    bpy.utils.unregister_class(PANEL_PT_builder_brush)
    bpy.utils.unregister_class(PANEL_PT_builder_paint)
    bpy.utils.unregister_class(PANEL_PT_builder_grid)
    bpy.utils.unregister_class(PANEL_PT_builder_analyze)
    bpy.utils.unregister_class(PANEL_PT_builder_selected)


#######################################################
#                MAP ANALYSIS ALGORITHM
#######################################################


def map_analysis():
    blockDict = {}
    for (x, y, z), block_id in all_blocks(): #Go through all blocks in the input level.
        if block_id not in blockDict: #If the current block is not in the dictionary yet we create an entry for it.
        #order: +x  -x  +y  -y  +z  -z
        #IN ENGINE WE USE LEFT HAND! Y AND Z CHANGES PLACE!
            blockDict[block_id] = [1, [get_block(x + 1, y, z)], [get_block(x - 1, y, z)], [get_block(x, y + 1, z)], [get_block(x, y - 1, z)], [get_block(x, y, z + 1)], [get_block(x, y, z - 1)]]

        else: #If the entry already exist we have to check if the neighboring blocks already exists in the id's lists.
            blockDict[block_id][0] += 1
            
            r = get_block(x + 1, y, z) #Get the block next to the current one.
            if blockDict[block_id][1].count(r) == 0: #Check if that block's id already exists in this block's list in that direction.
                blockDict[block_id][1].append(r)

            l = get_block(x - 1, y, z)
            if blockDict[block_id][2].count(l) == 0:
                blockDict[block_id][2].append(l)

            f = get_block(x, y + 1, z)
            if blockDict[block_id][3].count(f) == 0:
                blockDict[block_id][3].append(f)

            b = get_block(x, y - 1, z)
            if blockDict[block_id][4].count(b) == 0:
                blockDict[block_id][4].append(b)

            u = get_block(x, y, z + 1)
            if blockDict[block_id][5].count(u) == 0:
                blockDict[block_id][5].append(u)

            d = get_block(x, y, z - 1)
            if blockDict[block_id][6].count(d) == 0:
                blockDict[block_id][6].append(d)
        
    blendFileName = D.filepath.split('\\')[-1].split('.')[0] #Get the name of the blendfile.
    f = open(blendFileName + 'Output.txt', 'w')
    for uniqueBlockId in blockDict:
        f.write(uniqueBlockId + ' ' + str(blockDict[uniqueBlockId][0]) + '\n')
        for i in range(1, 7):
            s = ""
            for listItem in blockDict[uniqueBlockId][i]:
                s += listItem + ','
            f.write(s)
            f.write('\n')
        f.write('\n')

        



#######################################################
#                MAIN
#######################################################

# Things to do at startup
if __name__ == "__main__":
    if not D.collections.get('Brush'):
        grid = D.collections.new("Brush")
        C.scene.collection.children.link(grid)
    
    if not D.collections.get('Brushes'):
        brushes = D.collections.new("Brushes")
        C.scene.collection.children.link(brushes)

    if not D.collections.get('Map'):
        map = D.collections.new("Map")
        C.scene.collection.children.link(map)

    if not D.collections.get('Grid'):
        grid = D.collections.new("Grid")
        C.scene.collection.children.link(grid)

    if not D.collections.get('Settings'):
        settings = D.collections.new("Settings")
        C.scene.collection.children.link(settings)

    if not D.objects.get('grid'):
        C.view_layer.active_layer_collection = \
        C.view_layer.layer_collection.children['Settings']
        O.object.empty_add()
        settings = C.object
        settings.name = "grid"
        settings.location = (5, 5, 3)
        settings.scale = (5, 5, 5)
        settings.hide_viewport = True
    
    if not D.objects.get('current_grid_settings'):
        C.view_layer.active_layer_collection = \
        C.view_layer.layer_collection.children['Settings']
        O.object.empty_add()
        settings = C.object
        settings.name = "current_grid_settings"
        settings.location = (0, 0, 0)
        settings.scale = D.objects['grid'].scale
        settings.hide_viewport = True

    if not D.collections.get('Trash'):
        trash = D.collections.new("Trash")
        C.scene.collection.children.link(trash)

    if not D.collections.get('Object_Selection'):
        obj_select = D.collections.new("Object_Selection")
        # C.scene.collection.children.link(obj_select)

    if not D.collections.get('Active_Object'):
        active_obj = D.collections.new("Active_Object")
        # C.scene.collection.children.link(active_obj)

    if D.collections.get('Collection'):
        for obj in D.collections['Collection'].objects:
            if obj.name.split('.')[0] not in ['Camera', 'Cube', 'Light']:
                D.collections['Brushes'].objects.link(obj)
                D.collections['Collection'].objects.unlink(obj)

    if not D.objects.get('block_0-0-0'):
        generate_grid()
        D.collections['Object_Selection'].objects.link(D.objects['block_0-0-0'])
        D.collections['Active_Object'].objects.link(D.objects['block_0-0-0'])

    D.collections['Brushes'].hide_viewport = False
    D.collections['Grid'].hide_viewport = True

    if len(D.collections['Brushes'].objects) > 0:
        C.view_layer.objects.active = D.collections['Brushes'].objects[0]
        D.collections['Brushes'].objects[0].select_set(True)

    register()

