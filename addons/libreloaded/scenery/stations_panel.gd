extends VBoxContainer
class_name StationsPanel

## The stations of the sceneries loaded (SceneryServer.station_list(), a .scn's or a world's): a
## list of them by name and code, and Show - the free camera looking at the station picked from
## SHOW_DISTANCE south of it, SHOW_HEIGHT up (PlayerCameraServer.camera_place()). Shown in the HUD,
## filled again as the list changes.

## Where the camera looks at a station from: this far south of it, this high over it [m]
const SHOW_DISTANCE:float = 250.0
const SHOW_HEIGHT:float = 150.0

var _stations:Array[StationData] = []


func _ready() -> void:
    SceneryServer.station_list_changed.connect(_fill)
    _fill()


func _fill() -> void:
    _stations.assign(SceneryServer.station_list())
    _stations.sort_custom(func(a:StationData, b:StationData) -> bool:
            return a.station_name.naturalnocasecmp_to(b.station_name) < 0)
    %StationList.clear()
    for station:StationData in _stations:
        %StationList.add_item("%s (%s)" % [station.station_name, station.ref] if station.ref else station.station_name)
    %Show.disabled = _stations.is_empty()


## The free camera looking at the station picked in the list
func show_selected() -> void:
    var picked:PackedInt32Array = %StationList.get_selected_items()
    if not picked:
        return
    var target:Vector3 = _stations[picked[0]].position
    var eye:Vector3 = target + Vector3(0.0, SHOW_HEIGHT, SHOW_DISTANCE)
    PlayerCameraServer.camera_place(Transform3D(Basis.looking_at(target - eye), eye))
