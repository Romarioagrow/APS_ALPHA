"""Bind tested leaf opacity via the owned Forest collection; never rebuild a mesh."""
import unreal

root = '/Game/APS/APS_ALPHA/WSC/PlanetSurface/Diagnostics/'
collection = unreal.load_asset(root+'FoliagePrototype20260929V1/FC_APS_Proto_Forest')
mesh = unreal.load_asset(root+'FoliagePrototype20260929V1/SM_APS_Proto_TreeBudget')
leaf = unreal.load_asset(root+'FoliageLeaf20260930/MI_APS_PrototypeLeaf')
if not collection or not isinstance(mesh, unreal.StaticMesh) or not isinstance(leaf, unreal.MaterialInstanceConstant):
    raise RuntimeError('Missing owned prototype assets')
entries = collection.get_editor_property('foliage_list')
if len(entries) != 1 or entries[0].get_editor_property('static_mesh') != mesh:
    raise RuntimeError('Forest collection layout changed')
entry = entries[0]
if list(entry.get_editor_property('override_material')):
    raise RuntimeError('Forest already has an override; refusing replacement')
if mesh.get_num_lods() != 3 or len(mesh.get_editor_property('static_materials')) != 3:
    raise RuntimeError('Unexpected source mesh layout')
original_materials = [mesh.get_material(i).get_path_name() for i in range(3)]
if original_materials[1] != '/WorldScape/Ressources/Mesh/Tree/MI_Grass_Leaf.MI_Grass_Leaf':
    raise RuntimeError('Source mesh leaf drift')
entry.set_editor_property('override_material', [mesh.get_material(0), leaf])
if [mesh.get_material(i).get_path_name() for i in range(3)] != original_materials:
    raise RuntimeError('Mesh materials unexpectedly changed; no save')
if [m.get_path_name() for m in entry.get_editor_property('override_material')] != [original_materials[0], leaf.get_path_name()]:
    raise RuntimeError('Collection override did not stick; no save')
if not unreal.EditorAssetLibrary.save_loaded_asset(collection, only_if_is_dirty=True):
    raise RuntimeError('Owned collection save failed')
unreal.log('APS_PROTOTYPE_LEAF_INSTALLED collection='+collection.get_path_name()
           +' leaf='+leaf.get_path_name()+' meshWrites=0 overrideSlots=2 globals=unchanged')
