@tool
extends Node3D
class_name TrainSet3D

## A trainset assembled by hand - a node over RailVehicleServer's trainset (get_rid()), as a trainset
## of a loaded scenery is one built by its scenery provider without a node. Its RailVehicle3D
## children are the trainset's vehicles, in order: the server stands them on the trainset's track
## one after another from its front, each as long as it is, turned its way (start_direction) and
## trainset_gap behind the one before, and couples each with the next by its
## trainset_coupling_mode as "endtrainset" does. Reordered or changed, they stand anew; a reordered
## vehicle takes its gap and its coupling mode with it - they are the vehicle's, as a `.scn` writes the
## offset and the couplingdata with each `dynamic`, and as TrainsetData.get_arranged_members() moves a
## declared vehicle's gap with it. A vehicle of a trainset has no start track of its own.

## Where the trainset's front stands - the track and the distance along it [m]
@export var start_track_name:String = "":
    set(x):
        if not x == start_track_name:
            start_track_name = x
            _dirty = true
            set_process(true)
@export var start_track_offset:float = 0.0:
    set(x):
        if not x == start_track_offset:
            start_track_offset = x
            _dirty = true
            set_process(true)
## The trainset's timetable (a file in the scenery's directory, `none` for none) and the velocity
## it starts with - its driver's first orders (deserialize_endtrainset(),
## simulationstateserializer.cpp:839-848)
@export var timetable:String = ""
@export var velocity:float = 0.0

var _trainset:RID = RID()
## The track the trainset was last handed - another one registered under its name moves it there
var _track:RID = RID()
## The trainset is handed over once, whatever changed in the frame; the node processes only then
var _dirty:bool = false
## The trainset got its initial placement from one of its RailVehicle3D children.
var _start_track_taken_from_child:bool = false


## The trainset's handle in RailVehicleServer
func get_rid() -> RID:
    return _trainset


## The children come and go at any time and each takes its vehicle's handle only once built, so the
## trainset follows them from here rather than from a scene. They enter the tree after it and leave
## before it, so each is followed from its own entry to its own exit.
func _enter_tree() -> void:
    _trainset = RailVehicleServer.trainset_create()
    RailVehicleServer.trainset_set_name(_trainset, name)
    TrackServer.tracks_changed.connect(_on_tracks_changed)
    child_order_changed.connect(_on_child_order_changed)
    child_entered_tree.connect(_on_child_entered_tree)
    child_exiting_tree.connect(_on_child_exiting_tree)
    _dirty = true
    set_process(true)


func _exit_tree() -> void:
    TrackServer.tracks_changed.disconnect(_on_tracks_changed)
    child_order_changed.disconnect(_on_child_order_changed)
    child_entered_tree.disconnect(_on_child_entered_tree)
    child_exiting_tree.disconnect(_on_child_exiting_tree)
    RailVehicleServer.trainset_free(_trainset)
    _trainset = RID()


func _on_child_entered_tree(node:Node) -> void:
    var vehicle:RailVehicle3D = node as RailVehicle3D
    if not vehicle:
        return
    vehicle.vehicle_changed.connect(_on_child_order_changed)
    vehicle.trainset_placement_changed.connect(_on_child_order_changed)
    if not start_track_name and vehicle.start_track_name:
        start_track_name = vehicle.start_track_name
        start_track_offset = vehicle.start_track_offset
        _start_track_taken_from_child = true
    if _start_track_taken_from_child:
        vehicle.start_track_name = ""
        vehicle.start_track_offset = 0.0


func _on_child_exiting_tree(node:Node) -> void:
    var vehicle:RailVehicle3D = node as RailVehicle3D
    if vehicle:
        vehicle.vehicle_changed.disconnect(_on_child_order_changed)
        vehicle.trainset_placement_changed.disconnect(_on_child_order_changed)


func _on_child_order_changed() -> void:
    _dirty = true
    set_process(true)


func _on_tracks_changed() -> void:
    if not TrackServer.track_get_rid_by_name(start_track_name) == _track:
        _dirty = true
        set_process(true)


func _process(_delta:float) -> void:
    set_process(false)
    if not _dirty:
        return
    _dirty = false
    _track = TrackServer.track_get_rid_by_name(start_track_name)
    RailVehicleServer.trainset_set_track(_trainset, _track, start_track_offset)
    RailVehicleServer.trainset_clear(_trainset)
    for child:Node in get_children():
        var vehicle:RailVehicle3D = child as RailVehicle3D
        if not vehicle:
            continue
        # a vehicle not built yet stands the trainset anew once it is (vehicle_changed)
        if not vehicle.get_rid().is_valid():
            return
        RailVehicleServer.trainset_add_vehicle(
                _trainset, vehicle.get_rid(), vehicle.start_direction, vehicle.trainset_gap,
                vehicle.trainset_coupling_mode)
    RailVehicleServer.trainset_place(_trainset)
