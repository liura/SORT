import bpy
import bl_ui
from .. import common
from .. import SORTAddon
from extensions_framework import declarative_property_group

# attach customized properties in lamp
@SORTAddon.addon_register_class
class sort_lamp(declarative_property_group):
    ef_attach_to = ['Lamp']

    controls = []
    visibility = {}
    properties = []

@SORTAddon.addon_register_class
class sort_lamp_hemi(declarative_property_group):
    ef_attach_to = ['sort_lamp']

    controls = []

    properties = [
        {
            'type': 'string',
            'subtype': 'FILE_PATH',
            'attr': 'envmap_file',
            'name': 'HDRI Map',
            'description': 'EXR image to use for lighting (in latitude-longitude format)',
            'default': '',
            'save_in_preset': True
        },
    ]

class SORTLampPanel(bl_ui.properties_data_lamp.DataButtonsPanel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"
    COMPAT_ENGINES = {common.default_bl_name}

    @classmethod
    def poll(cls, context):
        rd = context.scene.render
        return super().poll(context) and rd.engine in cls.COMPAT_ENGINES

class LampPanel(SORTLampPanel, bpy.types.Panel):
    bl_label = 'Lamp Property'

    def draw(self, context):
        lamp = context.lamp
        if lamp is not None:
            layout = self.layout
            
            # lamp type
            layout.prop(lamp, "type", expand=True)

            layout.prop(lamp, "color")
            layout.prop(lamp, "energy")


class LampHemiPanel(SORTLampPanel, bpy.types.Panel):
    bl_label = 'Lamp Hemi Property'

    @classmethod
    def poll(cls, context):
        return super().poll(context) and context.lamp.type == 'HEMI'

    def draw(self, context):
        layout = self.layout
        lamp = context.lamp
        layout.prop(lamp.sort_lamp.sort_lamp_hemi, "envmap_file", text="HDRI file")


class LampAreaPanel(SORTLampPanel, bpy.types.Panel):
    bl_label = 'Lamp Area Property'

    @classmethod
    def poll(cls, context):
        return super().poll(context) and context.lamp.type == 'AREA'

    def draw(self, context):
        layout = self.layout
        lamp = context.lamp

        split = layout.split()
        col = split.column(align=True)
        col.prop(lamp, "shape", text="")
        sub = split.column(align=True)

        if lamp.shape == 'SQUARE':
            sub.prop(lamp, "size")
        elif lamp.shape == 'RECTANGLE':
            sub.prop(lamp, "size", text="Size X")
            sub.prop(lamp, "size_y", text="Size Y")

class LampSpotPanel(SORTLampPanel, bpy.types.Panel):
    bl_label = 'Lamp Spot Property'

    @classmethod
    def poll(cls, context):
        return super().poll(context) and context.lamp.type == 'SPOT'

    def draw(self, context):
        layout = self.layout
        lamp = context.lamp

        layout.prop( lamp , "spot_size" , text="Spot Light Range" )
        layout.prop( lamp , "spot_blend" , text="Spot Light Blend" )