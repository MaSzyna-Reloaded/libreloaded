extends GutTest

## CabinSystem lists and makes the controls of the cab implementation registered under a name, and
## nothing for a name no implementation is registered under

const IMPLEMENTATION:StringName = &"test_cabin_implementation"
const UNKNOWN:StringName = &"no_such_implementation"
const CONTROL:StringName = &"horn_bt"
const COMMAND:String = "horn_set"


class TestCabinImplementation extends CabinImplementation:
    func _component_list() -> Array[CabinComponentInfo]:
        var info := CabinComponentInfo.new()
        info.name = CONTROL
        info.title = "Horn"
        info.widget_class = &"CabinButton"
        info.commands = PackedStringArray([COMMAND])
        return [info]

    func _component_create(name:StringName) -> Node3D:
        if name != CONTROL:
            return null
        var control := Node3D.new()
        control.name = name
        return control


func before_each() -> void:
    CabinSystem.implementation_register(IMPLEMENTATION, TestCabinImplementation.new())


func after_each() -> void:
    CabinSystem.implementation_unregister(IMPLEMENTATION)


func test_the_registered_implementation_lists_its_controls() -> void:
    var listed:Array[CabinComponentInfo] = CabinSystem.component_list(IMPLEMENTATION)
    assert_eq(listed.size(), 1)
    assert_eq(listed[0].name, CONTROL)
    assert_eq(listed[0].widget_class, &"CabinButton")
    assert_eq(listed[0].commands, PackedStringArray([COMMAND]))


func test_the_registered_implementation_makes_a_control_it_lists() -> void:
    var control:Node3D = CabinSystem.component_create(IMPLEMENTATION, CONTROL)
    assert_not_null(control)
    assert_eq(control.name, String(CONTROL))
    control.free()
    assert_null(CabinSystem.component_create(IMPLEMENTATION, &"no_such_control"))


func test_a_name_without_an_implementation_lists_and_makes_nothing() -> void:
    assert_eq(CabinSystem.component_list(UNKNOWN).size(), 0)
    assert_null(CabinSystem.component_create(UNKNOWN, CONTROL))


func test_a_cab_names_the_implementation_that_builds_it() -> void:
    var cabin := Cabin3D.new()
    assert_eq(cabin.implementation, &"")
    cabin.implementation = IMPLEMENTATION
    assert_eq(cabin.implementation, IMPLEMENTATION)
    cabin.free()
