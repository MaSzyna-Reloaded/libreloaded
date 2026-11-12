extends Node3D
class_name FollowView

## External views of a vehicle (PlayerCameraServer.CameraFollowView, cycled with Shift+F4):
## TRAINSET_FRONT over the vehicle followed and BOGIE at its bogie, both looking the way the
## trainset goes, to its head; TRAINSET_REAR over the end of the trainset looking at its rear - the
## trainset's last vehicle, but a vehicle chosen in the trainset followed already, other than the
## player's own; DRIVEBY from a point by the track ahead of the vehicle followed, standing while it
## passes and set ahead of it again, standing to the vehicle as the player set it, once it is
## farther from it than the zoom takes a view - always looking at the vehicle, never from below its
## center, moved only by the arrows, the zoom and the mouse going up or down, straight, standing
## where they set it until the view is chosen anew. The view flies to the selected view like a drone
## and looks at what it shows. The arrows move the view along the screen's axes - left/right across,
## up/down ahead and back the way it looks, PageUp/PageDown up and down - the mouse wheel brings it
## closer or farther, dragging with the right mouse button orbits it around the point it looks at,
## up to straight over or under it, never upside down. Like a camera operator following the player's
## direction, the view keeps the shot so set, going with the vehicle as it drives. While no vehicle
## of the trainset is within the middle FRAME_SHARE of the screen and the mouse turns the view
## towards the trainset, it turns the view further where it stands, across, towards the part of the
## trainset turned to, the more of the way into the shot the more the mouse turns it to the
## trainset; nothing while the mouse turns it away or not at all, nor while all of the trainset is
## behind it. It never moves the view. Each view keeps its own shot while the player stays in one
## trainset; entering another one or leaving the vehicle sets them all back. It is no camera - the
## player's one camera looks through it while it is the view shown (Player), the only time it is
## processed.


## Distance scale per mouse wheel step towards the vehicle
const ZOOM_STEP:float = 0.9
## How fast the up/down arrows change the distance - e-fold per second, with Shift held - and as
## fast the left/right arrows and PageUp/PageDown move the view, the distance a second
const ZOOM_KEY_RATE:float = 1.0
const ZOOM_KEY_RATE_FAST:float = 3.0
## Closest and farthest the zoom takes the view from what it looks at (m)
const MIN_DISTANCE_SETTING:StringName = &"reloaded/camera/external_view_min_distance"
const MIN_DISTANCE_DEFAULT:float = 1.0
const MAX_DISTANCE_SETTING:StringName = &"reloaded/camera/external_view_max_distance"
const MAX_DISTANCE_DEFAULT:float = 200.0

## Where the view looks at first, ahead of the camera it takes over from [m]
const LOOK_AHEAD:float = 10.0
## The highest and lowest the orbit takes the view over or under what it looks at, short of the
## vertical, past which the view would stand upside down [rad]
const ELEVATION_LIMIT:float = deg_to_rad(89.0)
## The views over a vehicle: aside by its widths, up by its heights but at least VIEW_MIN_HEIGHT [m],
## along it by its lengths - back from its center for TRAINSET_FRONT, towards the rest of the
## trainset for TRAINSET_REAR
const VIEW_SIDE:float = 1.5
const VIEW_HEIGHT:float = 1.25
const VIEW_MIN_HEIGHT:float = 5.0
const VIEW_ALONG:float = 0.4
## The bogie view beside the vehicle: aside by its widths, ahead by its lengths, up [m]; looking at
## the bogie ahead by its lengths, at the same height
const BOGIE_VIEW_SIDE:float = 0.65
const BOGIE_VIEW_AHEAD:float = 0.15
const BOGIE_VIEW_HEIGHT:float = 0.9
const BOGIE_LOOK_AHEAD:float = 0.35
## The drive-by point ahead of the vehicle followed and aside of the track, level with its center [m]
const DRIVEBY_AHEAD:float = 50.0
const DRIVEBY_SIDE:float = 10.0
## How far the mouse moves the drive-by camera a pixel [m]
const DRIVEBY_MOUSE_STEP:float = 0.05
## The middle share of the screen, across and up, the camera operator brings the trainset into
const FRAME_SHARE:float = 0.8
## How much of the way into the shot the camera operator goes as the mouse turns the view towards
## the trainset: e-fold per radian turned to it
const OPERATOR_RATE:float = 2.0
## How far ahead of the view a part of the trainset is, at least, to count as in front of it [m]
const FRONT_DEPTH:float = 0.1


## The shot set for one view: the orbit, the shift and the zoom
class Offsets:
    ## Yaw (x) and pitch (y) of the view around what it looks at [rad]
    var orbit:Vector2 = Vector2.ZERO
    ## Shift of what the view looks at from the vehicle, in the frame of the vehicle the view stands
    ## at, so the shot goes with it [m]
    var shift:Vector3 = Vector3.ZERO
    ## Scale of the view's distance to what it looks at
    var zoom:float = 1.0

## The field of view of the camera looking through this view [degrees]
@export var fov:float = 45.0
## How fast the drone follows its target (1/s)
@export var response:float = 2.0
## Mouse orbit sensitivity (degrees per pixel)
@export var sensitivity:float = 0.25

## The vehicle followed (VehicleServer's)
var vehicle:RID
var view:PlayerCameraServer.CameraFollowView = PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT:
    set(x):
        if not view == x:
            view = x
            _dirty = true
## How fast the drone flies [m/s], for a view taking over from it
var velocity:Vector3 = Vector3.ZERO

var _dirty:bool = false
# the vehicle the view is attached to (none for the drive-by point) and its local offset
var _view_vehicle:RID
var _view_offset:Vector3 = Vector3.ZERO
# what the view looks at, local to _view_vehicle
var _look_offset:Vector3 = Vector3.ZERO
# what the view's target is, where it is now
var _look_target:Vector3 = Vector3.ZERO
# where the view looked the frame before, and what is left of the flight from there
var _look_point:Vector3 = Vector3.ZERO
var _look_flight:Vector3 = Vector3.ZERO
# the offsets of each view, by PlayerCameraServer.CameraFollowView, and those shown now, going over
# to the selected view's ones
var _offsets:Array[Offsets] = []
var _shown:Offsets = Offsets.new()
# what is left of the flight from where the view was to where the selected view puts it, around
# what it looks at: heading (x) and elevation (y) [rad], distance (z) [m]
var _flight:Vector3 = Vector3.ZERO
# the view's own distance from its target before the zoom and its own elevation over it before the
# orbit, for the zoom and the orbit to keep within their limits
var _view_distance:float = 1.0
var _view_elevation:float = 0.0
## The zoom's limits, as their settings are now
var _min_distance:float = MIN_DISTANCE_DEFAULT
var _max_distance:float = MAX_DISTANCE_DEFAULT
# the trainset of the player's vehicle the offsets are kept for (none outside a trainset)
var _player_trainset:RID
# whether the mouse has orbited the view since the last frame, and the shot set the frame before
var _orbit_turned:bool = false
var _last_shot:Transform3D = Transform3D()
# the vehicles of the trainset followed, the camera operator keeps a part of in the shot
var _trainset_vehicles:Array[RID] = []
# whether the vehicle followed was chosen in the trainset followed already, not the player's own -
# the rear view shows it instead of the trainset's last vehicle
var _chosen_in_trainset:bool = false
# the ways aside and ahead the arrows held move the drive-by camera, level, fixed as they went down;
# and the frame its
# shift is kept in, of the track at its point - across it, up, back along it - so the camera set
# ahead again stands to the vehicle as the player set it
var _driveby_aside:Vector3 = Vector3.ZERO
var _driveby_ahead:Vector3 = Vector3.ZERO
var _driveby_frame:Basis = Basis()


func _enter_tree() -> void:
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    PlayerServer.player_vehicle_changed.connect(_on_player_vehicle_changed)
    RailVehicleServer.trainset_vehicles_changed.connect(_on_trainset_changed)
    RailVehicleServer.trainset_head_changed.connect(_on_trainset_changed)
    _on_project_settings_changed()
    _reset_offsets()


func _exit_tree() -> void:
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)
    PlayerServer.player_vehicle_changed.disconnect(_on_player_vehicle_changed)
    RailVehicleServer.trainset_vehicles_changed.disconnect(_on_trainset_changed)
    RailVehicleServer.trainset_head_changed.disconnect(_on_trainset_changed)


func _on_project_settings_changed() -> void:
    _min_distance = ProjectSettings.get_setting(MIN_DISTANCE_SETTING)
    _max_distance = ProjectSettings.get_setting(MAX_DISTANCE_SETTING)


## Sets the shot of every view back to its own
func _reset_offsets() -> void:
    _offsets.clear()
    for _index:int in PlayerCameraServer.CAMERA_FOLLOW_VIEW_MAX:
        _offsets.append(Offsets.new())


## Entering a vehicle of another trainset or leaving the vehicle sets the views back; going to
## another vehicle of the same trainset keeps them. The rear view goes with the player's vehicle.
func _on_player_vehicle_changed(p_vehicle:RID, _previous:RID) -> void:
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(p_vehicle)
    if not (trainset.is_valid() and trainset == _player_trainset):
        _reset_offsets()
    _player_trainset = trainset
    _dirty = true


## A vehicle coupled or uncoupled, or another head: the view goes to the trainset as it is now, and
## the player's trainset is the one the player's vehicle is in now
func _on_trainset_changed(p_trainset:RID) -> void:
    _player_trainset = RailVehicleServer.vehicle_get_trainset(PlayerServer.player_get_vehicle())
    if p_trainset == RailVehicleServer.vehicle_get_trainset(vehicle):
        _dirty = true


## Starts the flight from p_from to the selected view of the vehicle; the selected view is kept
## between activations. A vehicle of another trainset than the one followed is shown anew, every
## view's shot set back; a vehicle in no trainset is a trainset of its own.
func activate(p_vehicle:RID, p_from:Transform3D) -> void:
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(p_vehicle)
    var followed:RID = RailVehicleServer.vehicle_get_trainset(vehicle)
    var anew:bool = not ((trainset if trainset.is_valid() else p_vehicle)
            == (followed if followed.is_valid() else vehicle))
    if anew:
        _reset_offsets()
    _chosen_in_trainset = not anew and not p_vehicle == PlayerServer.player_get_vehicle()
    vehicle = p_vehicle
    global_transform = p_from
    _look_point = p_from.origin - p_from.basis.z * LOOK_AHEAD
    _dirty = true


func _input(event:InputEvent) -> void:
    if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_RIGHT:
        Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED if event.pressed else Input.MOUSE_MODE_VISIBLE)
    elif event is InputEventMouseMotion and Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
        var offsets:Offsets = _offsets[view]
        # the drive-by camera always looks at the vehicle: the mouse going down moves it straight up,
        # DRIVEBY_MOUSE_STEP a pixel, and it stands there on; aside the mouse does nothing
        if not _view_vehicle.is_valid():
            offsets.shift.y += event.relative.y * DRIVEBY_MOUSE_STEP
            return
        var turn:Vector2 = Vector2(event.relative.x, event.relative.y) * deg_to_rad(sensitivity)
        offsets.orbit -= turn
        _orbit_turned = true
        offsets.orbit.y = clampf(offsets.orbit.y, _view_elevation - ELEVATION_LIMIT, _view_elevation + ELEVATION_LIMIT)


# the wheel is left to a HUD under the cursor first (the vehicle card scrolls with it)
func _unhandled_input(event:InputEvent) -> void:
    if not event is InputEventMouseButton or not event.pressed:
        return
    if event.button_index == MOUSE_BUTTON_WHEEL_UP:
        _zoom_by(ZOOM_STEP)
    elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
        _zoom_by(1.0 / ZOOM_STEP)


func _process(delta:float) -> void:
    if not vehicle.is_valid():
        return
    var fly:bool = _dirty
    if _dirty:
        _dirty = false
        _process_dirty()
    _move_view(delta)

    var weight:float = 1.0 - exp(-response * delta)
    var view_transform:Transform3D = VehicleServer.vehicle_get_transform(_view_vehicle)
    var look_target:Vector3
    # the drive-by point is cut to, as a camera standing there, never flown to; it looks at the
    # vehicle's center, and stands until the vehicle is far from it
    var cut:bool = fly and not _view_vehicle.is_valid()
    fly = fly and not cut
    # the drive-by camera chosen anew stands where its point is; set ahead again, as the player set it
    if cut:
        _offsets[view].shift = Vector3.ZERO
    if _view_vehicle.is_valid():
        look_target = view_transform * _look_offset
    else:
        var body:Transform3D = VehicleServer.vehicle_get_transform(vehicle)
        look_target = body.origin + body.basis.y.normalized() * 0.5 * VehicleServer.vehicle_get_dimensions(vehicle).y
        # gone farther from the camera than the zoom takes a view, the vehicle is shown from a new
        # point ahead, the keys held or not
        if (_view_offset + _driveby_frame * _offsets[view].shift).distance_to(look_target) > _max_distance:
            _process_dirty()
            cut = true
    # the view goes with what it looks at as it is, so a driving trainset keeps its place in the shot;
    # it goes over to the selected offsets, another view or vehicle is flown to with its own offsets,
    # a drive-by point set again is cut to
    _look_target = look_target
    var offsets:Offsets = _offsets[view]
    if fly or cut:
        _shown.orbit = offsets.orbit
        _shown.shift = offsets.shift
        _shown.zoom = offsets.zoom
    _shown.orbit = _shown.orbit.lerp(offsets.orbit, weight)
    _shown.shift = _shown.shift.lerp(offsets.shift, weight)
    _shown.zoom = lerpf(_shown.zoom, offsets.zoom, weight)

    # the view stands on its arm turned by the orbit around what it looks at - the heading around the
    # vertical, the elevation up to it - and faces along the arm, so it turns only the way the orbit
    # goes; the views over a vehicle go with it, the drive-by point stands
    # the drive-by camera stands where its point and its shift set it, looking at the vehicle, never
    # lower than its center
    if not _view_vehicle.is_valid():
        offsets.shift.y = maxf(offsets.shift.y, _look_target.y - _view_offset.y)
    var view_position:Vector3 = (
            view_transform * _view_offset if _view_vehicle.is_valid() else _view_offset + _driveby_frame * _shown.shift)
    var arm:Vector3 = view_position - _look_target
    _view_distance = arm.length()
    _view_elevation = atan2(arm.y, Vector2(arm.x, arm.z).length())
    var heading:float = atan2(arm.x, arm.z) + _shown.orbit.x
    var elevation:float = clampf(_view_elevation - _shown.orbit.y, -ELEVATION_LIMIT, ELEVATION_LIMIT)
    var distance:float = _view_distance * _shown.zoom
    # the shift is the view vehicle's own; the drive-by point stands in the world
    var vehicle_basis:Basis = view_transform.basis if _view_vehicle.is_valid() else Basis()
    var look_point:Vector3 = _look_target + (vehicle_basis * _shown.shift if _view_vehicle.is_valid() else Vector3.ZERO)

    # the flight starts where the view stands, looking where it looked, and goes around what the view
    # is to look at, so the view turns to it without a jump or a swing
    if fly:
        _look_flight = _look_point - look_point
        var from:Vector3 = global_position - _look_point
        _flight = Vector3(
                angle_difference(heading, atan2(from.x, from.z)),
                atan2(from.y, Vector2(from.x, from.z).length()) - elevation,
                from.length() - distance)
    if cut:
        _look_flight = Vector3.ZERO
        _flight = Vector3.ZERO
    _look_point = look_point + _look_flight
    _look_flight *= 1.0 - weight
    _flight *= 1.0 - weight
    var facing:Basis = Basis(Vector3.UP, heading + _flight.x) * Basis(Vector3.RIGHT, -(elevation + _flight.y))
    var previous_position:Vector3 = global_position
    global_transform = Transform3D(facing, _look_point + facing.z * (distance + _flight.z))
    if delta > 0.0:
        velocity = (global_position - previous_position) / delta
    # the shot set, not the view going over to it, is what the camera operator works on; it follows
    # the trainset's part nearest the shot each frame, and moves only for the mouse
    if _view_vehicle.is_valid():
        var shot_heading:float = atan2(arm.x, arm.z) + offsets.orbit.x
        var shot_elevation:float = clampf(_view_elevation - offsets.orbit.y, -ELEVATION_LIMIT, ELEVATION_LIMIT)
        var screen:Basis = Basis(Vector3.UP, shot_heading) * Basis(Vector3.RIGHT, -shot_elevation)
        var shot_distance:float = _view_distance * offsets.zoom
        var pivot:Vector3 = _look_target + vehicle_basis * offsets.shift
        var shot:Transform3D = Transform3D(screen, pivot + screen.z * shot_distance)
        if _orbit_turned:
            # the operator turns the view where it stands: the point it turns around goes along
            var turn:float = _bring_trainset_in(_last_shot, shot)
            offsets.orbit.x += turn
            var turned:Basis = Basis(Vector3.UP, shot_heading + turn) * Basis(Vector3.RIGHT, -shot_elevation)
            offsets.shift += vehicle_basis.inverse() * (shot.origin - turned.z * shot_distance - pivot)
            shot = Transform3D(turned, shot.origin)
        _last_shot = shot
    _orbit_turned = false


## How far the camera operator turns p_shot where it stands, around the vertical, to bring the
## trainset into its middle FRAME_SHARE [rad]: none while any vehicle of it is within it, nor while
## all of it is behind the view; else the part of it nearest the middle into it, the more of the way
## the more p_shot turned to the trainset from p_before - as from straight aside, with all of it
## behind p_before
func _bring_trainset_in(p_before:Transform3D, p_shot:Transform3D) -> float:
    var part:Vector3 = _nearest_part(p_shot)
    if part == Vector3.ZERO or _frame_off(p_shot, part) <= 1.0:
        return 0.0
    var before:Vector3 = _nearest_part(p_before)
    var before_angle:float = PI * 0.5 if before == Vector3.ZERO else (-p_before.basis.z).angle_to(before)
    var turned_to:float = maxf(before_angle - (-p_shot.basis.z).angle_to(part), 0.0)
    # how far across the part stands, as the angle the view turns to it; a turn to the right is a
    # smaller heading
    var aside:float = atan2(p_shot.basis.x.dot(part), -p_shot.basis.z.dot(part))
    var edge:float = atan(tan(deg_to_rad(fov) * 0.5) * FRAME_SHARE * _aspect())
    return -(aside - clampf(aside, -edge, edge)) * (1.0 - exp(-OPERATOR_RATE * turned_to))


## The part of the trainset nearest the middle of p_shot's screen, from where p_shot stands; none
## (Vector3.ZERO) while all of it is behind the view. A vehicle is the line along its length; of
## what of it is in front of the view, its part nearest the middle is the point nearest the line
## the view looks along.
func _nearest_part(p_shot:Transform3D) -> Vector3:
    var forward:Vector3 = -p_shot.basis.z
    var nearest:Vector3 = Vector3.ZERO
    var nearest_off:float = INF
    for each:RID in _trainset_vehicles:
        var body:Transform3D = VehicleServer.vehicle_get_transform(each)
        var dimensions:Vector3 = VehicleServer.vehicle_get_dimensions(each)
        var center:Vector3 = body.origin + body.basis.y.normalized() * 0.5 * dimensions.y - p_shot.origin
        var along:Vector3 = body.basis.z.normalized() * dimensions.z
        var start:Vector3 = center - 0.5 * along
        var end:Vector3 = center + 0.5 * along
        var start_depth:float = forward.dot(start)
        var end_depth:float = forward.dot(end)
        if start_depth <= FRONT_DEPTH and end_depth <= FRONT_DEPTH:
            continue
        if start_depth < FRONT_DEPTH:
            start = start.lerp(end, (FRONT_DEPTH - start_depth) / (end_depth - start_depth))
        elif end_depth < FRONT_DEPTH:
            end = end.lerp(start, (FRONT_DEPTH - end_depth) / (start_depth - end_depth))
        var front:Vector3 = end - start
        var across:float = front.dot(front) - pow(front.dot(forward), 2.0)
        var share:float = 0.0
        if across > 0.0:
            share = clampf((front.dot(forward) * forward.dot(start) - front.dot(start)) / across, 0.0, 1.0)
        var part:Vector3 = start + front * share
        var off:float = _frame_off(p_shot, part)
        if off < nearest_off:
            nearest_off = off
            nearest = part
    return nearest


## How far off the middle of p_shot's screen p_part stands, from where p_shot stands: 1 at the edge
## of its middle FRAME_SHARE
func _frame_off(p_shot:Transform3D, p_part:Vector3) -> float:
    var tan_height:float = tan(deg_to_rad(fov) * 0.5) * FRAME_SHARE
    var depth:float = -p_shot.basis.z.dot(p_part)
    return maxf(absf(p_shot.basis.x.dot(p_part)) / (depth * tan_height * _aspect()),
            absf(p_shot.basis.y.dot(p_part)) / (depth * tan_height))


## The screen's width by its height
func _aspect() -> float:
    var screen:Vector2 = get_viewport().get_visible_rect().size
    return screen.x / screen.y


## Sets the view up once per selection and per change of the trainset followed; the offsets of the
## view stay as they are
func _process_dirty() -> void:
    if view == PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY:
        var body:Transform3D = VehicleServer.vehicle_get_transform(vehicle)
        # Godot vehicles face -Z
        var ahead:Vector3 = -body.basis.z.normalized() * _towards_head(vehicle)
        _view_vehicle = RID()
        var level:Vector3 = Vector3(ahead.x, 0.0, ahead.z).normalized()
        _driveby_frame = Basis(level.cross(Vector3.UP).normalized(), Vector3.UP, -level)
        _view_offset = (body.origin + ahead * DRIVEBY_AHEAD + ahead.cross(Vector3.UP).normalized() * DRIVEBY_SIDE
                + body.basis.y.normalized() * 0.5 * VehicleServer.vehicle_get_dimensions(vehicle).y)
        return

    _view_vehicle = vehicle
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(vehicle)
    _trainset_vehicles = [vehicle]
    if trainset.is_valid():
        _trainset_vehicles.assign(RailVehicleServer.trainset_get_vehicles(trainset))
    if view == PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR and not _chosen_in_trainset:
        _view_vehicle = _trainset_vehicles[_trainset_vehicles.size() - 1]
    var dimensions:Vector3 = VehicleServer.vehicle_get_dimensions(_view_vehicle)
    var width:float = dimensions.x
    var height:float = dimensions.y
    var length:float = dimensions.z
    # the vehicle's own frame faces -Z; ahead is the way to the trainset's head
    var ahead:float = _towards_head(_view_vehicle)
    var over:float = maxf(VIEW_MIN_HEIGHT, VIEW_HEIGHT * height)
    match view:
        PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT:
            _view_offset = Vector3(-VIEW_SIDE * width * ahead, over, VIEW_ALONG * length * ahead)
            _look_offset = Vector3(0.0, 0.5 * height, 0.0)
        PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR:
            _view_offset = Vector3(VIEW_SIDE * width * ahead, over, -VIEW_ALONG * length * ahead)
            _look_offset = Vector3(0.0, 0.5 * height, 0.5 * length * ahead)
        PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE:
            _view_offset = Vector3(BOGIE_VIEW_SIDE * width * ahead, BOGIE_VIEW_HEIGHT,
                    -BOGIE_VIEW_AHEAD * length * ahead)
            _look_offset = Vector3(0.0, BOGIE_VIEW_HEIGHT, -BOGIE_LOOK_AHEAD * length * ahead)


## +1 when the vehicle faces the head of its trainset, or is in none; -1 when it is reversed in it
func _towards_head(p_vehicle:RID) -> float:
    var reversed:bool = (RailVehicleServer.vehicle_get_trainset(p_vehicle).is_valid()
            and RailVehicleServer.trainset_vehicle_get_direction(p_vehicle) == TrackServer.DIRECTION_REVERSED)
    return -1.0 if reversed else 1.0


## The arrows and PageUp/PageDown move the view along the screen's axes - left/right across it,
## up/down ahead and back the way it looks, PageUp/PageDown up it - as fast as the zoom goes
func _move_view(delta:float) -> void:
    var move:Vector2 = Vector2(
        float(Input.is_physical_key_pressed(KEY_RIGHT)) - float(Input.is_physical_key_pressed(KEY_LEFT)),
        float(Input.is_physical_key_pressed(KEY_PAGEUP)) - float(Input.is_physical_key_pressed(KEY_PAGEDOWN)))
    var zoom:float = float(Input.is_physical_key_pressed(KEY_DOWN)) - float(Input.is_physical_key_pressed(KEY_UP))
    if move.x == 0.0:
        _driveby_aside = Vector3.ZERO
    if zoom == 0.0:
        _driveby_ahead = Vector3.ZERO
    if move.is_zero_approx() and zoom == 0.0:
        return
    var fast:bool = Input.is_key_pressed(KEY_SHIFT)
    # as fast as the zoom: the distance to what the view looks at, at its rate, a second
    var rate:float = ZOOM_KEY_RATE_FAST if fast else ZOOM_KEY_RATE
    var distance:float = (_view_distance * _offsets[view].zoom if _view_vehicle.is_valid()
            else (_view_offset + _driveby_frame * _offsets[view].shift).distance_to(_look_target))
    var step:Vector2 = move.normalized() * distance * rate * delta
    if _view_vehicle.is_valid():
        # along the screen's axes as they are - across, up, and the up arrow ahead the way the view
        # looks - with what it looks at, kept in the view vehicle's frame
        _offsets[view].shift += VehicleServer.vehicle_get_transform(_view_vehicle).basis.inverse() * (
                global_transform.basis.x * step.x + global_transform.basis.y * step.y
                + global_transform.basis.z * zoom * distance * rate * delta)
        return
    else:
        # the drive-by camera turns after the vehicle, so it goes aside and ahead along the screen's
        # level axes as they were when the arrows went down - the up arrow ahead, the down one back -
        # and up straight, in the world
        if move.x != 0.0 and _driveby_aside == Vector3.ZERO:
            _driveby_aside = Vector3(global_transform.basis.x.x, 0.0, global_transform.basis.x.z).normalized()
        if zoom != 0.0 and _driveby_ahead == Vector3.ZERO:
            _driveby_ahead = Vector3(-global_transform.basis.z.x, 0.0, -global_transform.basis.z.z).normalized()
        _offsets[view].shift += _driveby_frame.inverse() * (_driveby_aside * step.x + Vector3.UP * step.y
                - _driveby_ahead * zoom * distance * rate * delta)
        return
    _zoom_by(exp(zoom * (ZOOM_KEY_RATE_FAST if fast else ZOOM_KEY_RATE) * delta))


## Scales the view's distance, kept within the min/max distance settings - closer than the least, the
## view goes on ahead along the way it looks, what it looks at with it. The drive-by camera stands:
## it is moved ahead or back, level, as far as the distance would change, as the arrows move it.
func _zoom_by(p_factor:float) -> void:
    var offsets:Offsets = _offsets[view]
    if not _view_vehicle.is_valid():
        var distance:float = _look_target.distance_to(_view_offset + _driveby_frame * offsets.shift)
        var ahead:Vector3 = Vector3(-global_transform.basis.z.x, 0.0, -global_transform.basis.z.z).normalized()
        offsets.shift += _driveby_frame.inverse() * ahead * distance * (1.0 - p_factor)
        return
    # closer than the zoom goes, the view goes on ahead with what it looks at, along the way it looks
    var short:float = _min_distance - _view_distance * offsets.zoom * p_factor
    offsets.zoom = clampf(offsets.zoom * p_factor, _min_distance / _view_distance, _max_distance / _view_distance)
    if short > 0.0:
        offsets.shift += (VehicleServer.vehicle_get_transform(_view_vehicle).basis.inverse()
                * -global_transform.basis.z * short)
