extends ReloadedGutTest

## TractionRenderingServer draws by the backend chosen; the core's own one draws plain wires

var _traction_rid: RID


func before_each() -> void:
    TractionRenderingServer.set_rendering_backend(TractionRenderingServer.PLAIN_BACKEND)
    _traction_rid = TractionRenderingServer.create_traction()
    TractionRenderingServer.set_traction_scenario(_traction_rid, get_tree().root.world_3d.scenario)


func after_each() -> void:
    TractionRenderingServer.free_traction(_traction_rid)
    TractionRenderingServer.set_rendering_backend(ProjectSettings.get_setting(
        TractionRenderingServer.RENDERING_BACKEND_SETTING, TractionRenderingServer.PLAIN_BACKEND))


func _span(wires: TractionRenderingServer.Wires) -> TractionData:
    var span: TractionData = TractionData.new()
    span.contact_p1 = Vector3(0.0, 5.6, 0.0)
    span.contact_p2 = Vector3(60.0, 5.6, 0.0)
    span.support_p1 = Vector3(0.0, 7.0, 0.0)
    span.support_p2 = Vector3(60.0, 7.0, 0.0)
    span.wires = wires
    return span


func test_the_plain_backend_draws_the_contact_and_the_support_wire() -> void:
    TractionRenderingServer.set_traction_description(_traction_rid, _span(TractionRenderingServer.Wires.CONTACT_SUPPORT))

    var aabb: AABB = _built_mesh().get_aabb()
    assert_almost_eq(aabb.size.x, 60.0, 0.01, "along the span")
    assert_almost_eq(aabb.size.y, 1.4, 0.05, "from the contact wire up to the support wire")


func test_a_contact_wire_alone_has_no_support_wire() -> void:
    TractionRenderingServer.set_traction_description(_traction_rid, _span(TractionRenderingServer.Wires.CONTACT))

    assert_lt(_built_mesh().get_aabb().size.y, 0.1, "only the contact wire")


func _built_mesh() -> ArrayMesh:
    return (TractionRenderingServer.traction_get_state(_traction_rid) as TractionRenderingBackend.PlainTractionState).mesh
