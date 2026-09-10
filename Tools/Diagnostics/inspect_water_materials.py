import unreal


ASSETS = [
    "/Game/Ressources/Materials/WorldScapeMaterials/Ocean/MI_Planetary_Ocean",
    "/Game/APS/APS_ALPHA/WSC/WSC_MI_Planetary_Ocean",
    "/Game/APS/APS_ALPHA/WSC/PlanetSurface/Materials/MI_APS_WS_Water",
]


def log_parameters(asset_path: str) -> None:
    asset = unreal.load_asset(asset_path)
    if not asset:
        unreal.log_warning(f"[APS.WaterInspect] missing={asset_path}")
        return

    parent = asset.get_editor_property("parent")
    unreal.log(
        f"[APS.WaterInspect] asset={asset.get_path_name()} "
        f"parent={parent.get_path_name() if parent else 'none'}"
    )
    material = asset
    for kind, names_fn, value_fn in (
        (
            "scalar",
            unreal.MaterialEditingLibrary.get_scalar_parameter_names,
            unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value,
        ),
        (
            "vector",
            unreal.MaterialEditingLibrary.get_vector_parameter_names,
            unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value,
        ),
    ):
        try:
            names = names_fn(material)
        except Exception as exc:
            unreal.log_warning(f"[APS.WaterInspect] names kind={kind} error={exc}")
            continue
        for name in names:
            try:
                value = value_fn(asset, name)
                unreal.log(f"[APS.WaterInspect] {kind} {name}={value}")
            except Exception as exc:
                unreal.log_warning(
                    f"[APS.WaterInspect] value kind={kind} name={name} error={exc}"
                )


for path in ASSETS:
    log_parameters(path)
