extends ReloadedGutTest

## FollowView, the external views of a vehicle, around a car (a trainset of its own): the left/right
## arrows and PageUp/PageDown move the view in the screen plane without turning it, the up/down
## arrows zoom it as the mouse wheel does, the mouse orbits it only the way the mouse goes and up to
## the vertical, never upside down; like a camera operator following the player's direction the
## view leaves a shot the arrows took past its frame be without the mouse and while the mouse turns
## it away from the vehicle or it faces away, keeps a shot moved aside through a small orbit, and
## brings the vehicle into the shot, across the screen only, as the mouse turns it to the vehicle; a change of
## view turns it without turning back, and leaving the vehicle sets the views back.

## The car's size [m]
const VEHICLE_LENGTH:float = 4.0
const VEHICLE_WIDTH:float = 1.8
const VEHICLE_HEIGHT:float = 1.5
## Frames for the view to settle where its offsets put it
const SETTLE_FRAMES:int = 60
## How fast the view follows in the tests (1/s), so it settles within SETTLE_FRAMES
const RESPONSE:float = 50.0
## How fast the view follows in the game (1/s), FollowView's own, for a flight to be watched
const GAME_RESPONSE:float = 2.0
## Frames a flight at GAME_RESPONSE is watched
const FLIGHT_FRAMES:int = 180
const ZOOM_STEPS:int = 10
## Mouse motion of one orbit step [px] and the steps of a half turn at the view's sensitivity
const ORBIT_STEP_PX:float = 20.0
const ORBIT_STEPS:int = 36
## Frames a key is held - moving the view less than the shot lets it
const KEY_FRAMES:int = 15
## Seconds the up arrow is held to come as close as the zoom goes, and then again
const CLOSE_IN_SECONDS:float = 3.0
## Mouse steps of a small orbit, the vehicle staying in the shot
const SMALL_ORBIT_STEPS:int = 3
## How far off the middle of the screen the vehicle stands once moved aside, at least, as the share
## of half the screen
const OFF_MIDDLE:float = 0.1
## Seconds the right arrow is held to take the vehicle past the shot, and watched after it
const PAST_SHOT_SECONDS:float = 1.5
const LEFT_BE_SECONDS:float = 1.0
## Seconds the right arrow is held with Shift to take the view far aside of the vehicle
const FAR_ASIDE_SECONDS:float = 2.0
## Turns to the vehicle and back, and the share of its distance the view may come nearer it by them
const TURNS_TO_AND_BACK:int = 5
const NEARER_SHARE:float = 0.1
## The most the view may turn from one frame to the next while the mouse orbits it [rad]
const FRAME_TURN_LIMIT:float = 0.5
## How far down a view looking straight down looks: the sine of its pitch, below this
const STRAIGHT_DOWN:float = -0.99
## Mouse steps back up, after which a view at the vertical looks up again
const BACK_STEPS:int = 3
## Distances and angles equal to this [m, rad]
const TOLERANCE:float = 0.05

var _vehicle:RID
var _controller:RID
var _cabin:RID
var _view:FollowView


func before_each() -> void:
    var description:RoadVehicleController = RoadVehicleController.new()
    description.dimensions_length = VEHICLE_LENGTH
    description.dimensions_width = VEHICLE_WIDTH
    description.dimensions_height = VEHICLE_HEIGHT
    _vehicle = VehicleServer.vehicle_create()
    _controller = VehicleServer.controller_create()
    VehicleServer.controller_configure(_controller, description)
    VehicleServer.vehicle_bind_controller(_vehicle, _controller)
    _cabin = VehicleServer.cabin_create()
    VehicleServer.vehicle_cabin_attach(_vehicle, _cabin)
    _view = FollowView.new()
    _view.response = RESPONSE
    add_child(_view)


func after_each() -> void:
    PlayerServer.player_leave_vehicle()
    _release_mouse()
    _view.free()
    VehicleServer.vehicle_free(_vehicle)
    VehicleServer.controller_free(_controller)
    VehicleServer.cabin_free(_cabin)


func _follow(p_view:PlayerCameraServer.CameraFollowView) -> void:
    _view.view = p_view
    _view.activate(_vehicle, _view.global_transform)
    await wait_idle_frames(SETTLE_FRAMES)


# where the vehicle is on the screen: -1 to 1 across and up, its edges
func _on_screen() -> Vector2:
    var to_vehicle:Vector3 = _center() - _view.global_position
    var depth:float = _forward().dot(to_vehicle)
    var screen:Vector2 = _view.get_viewport().get_visible_rect().size
    var half_height:float = depth * tan(deg_to_rad(_view.fov) * 0.5)
    var half_width:float = half_height * screen.x / screen.y
    return Vector2(_view.global_transform.basis.x.dot(to_vehicle) / half_width,
            _view.global_transform.basis.y.dot(to_vehicle) / half_height)


func _center() -> Vector3:
    return VehicleServer.vehicle_get_transform(_vehicle).origin + Vector3.UP * 0.5 * VEHICLE_HEIGHT


func _distance() -> float:
    return _view.global_position.distance_to(_center())


func _forward() -> Vector3:
    return -_view.global_transform.basis.z


func _wheel(p_button:MouseButton, p_steps:int) -> void:
    for _step:int in p_steps:
        var event:InputEventMouseButton = InputEventMouseButton.new()
        event.button_index = p_button
        event.pressed = true
        Input.parse_input_event(event)
        await wait_idle_frames(1)


func _grab_mouse() -> bool:
    var event:InputEventMouseButton = InputEventMouseButton.new()
    event.button_index = MOUSE_BUTTON_RIGHT
    event.pressed = true
    Input.parse_input_event(event)
    await wait_idle_frames(1)
    return Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED


func _release_mouse() -> void:
    var event:InputEventMouseButton = InputEventMouseButton.new()
    event.button_index = MOUSE_BUTTON_RIGHT
    event.pressed = false
    Input.parse_input_event(event)


# the directions the view looks in as the mouse goes by p_motion for p_steps, one per frame
func _orbit(p_motion:Vector2, p_steps:int) -> Array[Vector3]:
    var forwards:Array[Vector3] = [_forward()]
    for _step:int in p_steps:
        var event:InputEventMouseMotion = InputEventMouseMotion.new()
        event.relative = p_motion
        Input.parse_input_event(event)
        await wait_idle_frames(1)
        forwards.append(_forward())
    return forwards


func _hold_key(p_key:Key, p_frames:int) -> void:
    var event:InputEventKey = InputEventKey.new()
    event.physical_keycode = p_key
    event.pressed = true
    Input.parse_input_event(event)
    await wait_idle_frames(p_frames)
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await wait_idle_frames(SETTLE_FRAMES)


# the view moved by p_key in the screen plane along p_axis, looking the same way
func _assert_moves_in_the_screen_plane(p_key:Key, p_axis:Vector3) -> void:
    var position:Vector3 = _view.global_position
    var forward:Vector3 = _forward()

    await _hold_key(p_key, KEY_FRAMES)

    var moved:Vector3 = _view.global_position - position
    assert_gt(moved.length(), TOLERANCE, "it moved")
    assert_almost_eq(moved.angle_to(p_axis), 0.0, TOLERANCE, "along the screen's axis")
    assert_almost_eq(_forward().angle_to(forward), 0.0, TOLERANCE, "looking the same way")


func test_the_right_arrow_moves_the_view_right_in_the_screen_plane() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _assert_moves_in_the_screen_plane(KEY_RIGHT, _view.global_transform.basis.x)


func test_the_left_arrow_moves_the_view_left_in_the_screen_plane() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _assert_moves_in_the_screen_plane(KEY_LEFT, -_view.global_transform.basis.x)


func test_page_up_moves_the_view_up_in_the_screen_plane() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _assert_moves_in_the_screen_plane(KEY_PAGEUP, _view.global_transform.basis.y)


func test_moved_aside_the_up_and_down_arrows_move_the_view_ahead_and_back_the_way_it_looks() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _hold_key(KEY_RIGHT, KEY_FRAMES)
    await _assert_moves_in_the_screen_plane(KEY_UP, _forward())
    await _assert_moves_in_the_screen_plane(KEY_DOWN, -_forward())


func test_the_up_and_down_arrows_bring_the_view_closer_and_farther_as_the_wheel_does() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var distance:float = _distance()
    var forward:Vector3 = _forward()

    await _hold_key(KEY_UP, KEY_FRAMES)
    var closer:float = _distance()
    await _hold_key(KEY_DOWN, 2 * KEY_FRAMES)
    var farther:float = _distance()
    await _wheel(MOUSE_BUTTON_WHEEL_UP, ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_lt(closer, distance, "the up arrow brings it closer")
    assert_gt(farther, distance, "the down arrow takes it farther")
    assert_lt(_distance(), farther, "as the wheel does")
    assert_almost_eq(_forward().angle_to(forward), 0.0, TOLERANCE, "looking the same way")


func test_a_view_moved_aside_comes_on_ahead_as_long_as_the_up_arrow_is_held() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _hold_key(KEY_RIGHT, KEY_FRAMES)
    var forward:Vector3 = _forward()
    var up:InputEventKey = InputEventKey.new()
    up.physical_keycode = KEY_UP
    up.pressed = true
    Input.parse_input_event(up)
    await get_tree().create_timer(CLOSE_IN_SECONDS).timeout
    var closed_in:Vector3 = _view.global_position

    await get_tree().create_timer(CLOSE_IN_SECONDS).timeout

    var went:float = (_view.global_position - closed_in).dot(forward)
    up = up.duplicate()
    up.pressed = false
    Input.parse_input_event(up)
    assert_gt(went, TOLERANCE, "past the closest the zoom goes, it still comes on ahead")


func test_the_orbit_turns_the_view_only_the_way_the_mouse_goes() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return

    var forwards:Array[Vector3] = await _orbit(Vector2(ORBIT_STEP_PX, 0.0), ORBIT_STEPS)

    for index:int in range(1, forwards.size()):
        var turn:float = angle_difference(atan2(forwards[index - 1].x, forwards[index - 1].z),
                atan2(forwards[index].x, forwards[index].z))
        assert_true(turn <= TOLERANCE, "step %d turns the view one way" % index)


func test_the_orbit_takes_the_view_up_to_the_vertical_never_upside_down() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return

    var forwards:Array[Vector3] = await _orbit(Vector2(0.0, ORBIT_STEP_PX), ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    for index:int in range(1, forwards.size()):
        assert_lt(forwards[index - 1].angle_to(forwards[index]), FRAME_TURN_LIMIT, "step %d turns smoothly" % index)
    assert_lt(_forward().y, STRAIGHT_DOWN, "it looks straight down")
    assert_gt(_view.global_transform.basis.y.y, 0.0, "upright")
    var down:Vector3 = _forward()
    await _orbit(Vector2(0.0, -ORBIT_STEP_PX), BACK_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_almost_eq(_forward().angle_to(down), deg_to_rad(BACK_STEPS * ORBIT_STEP_PX * _view.sensitivity), TOLERANCE,
            "the mouse back up turns it back at once, as far as it goes")


func test_without_the_mouse_the_operator_leaves_a_shot_past_its_frame_be() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var event:InputEventKey = InputEventKey.new()
    event.physical_keycode = KEY_RIGHT
    event.pressed = true
    Input.parse_input_event(event)
    await get_tree().create_timer(PAST_SHOT_SECONDS).timeout
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await wait_idle_frames(SETTLE_FRAMES)
    var past:Vector2 = _on_screen()
    assert_lt(past.x, -FollowView.FRAME_SHARE, "the arrow took the vehicle past the frame")

    await get_tree().create_timer(LEFT_BE_SECONDS).timeout

    assert_almost_eq(_on_screen().distance_to(past), 0.0, TOLERANCE, "nothing brings it back")


# the view moved far aside to the right with the arrows, the vehicle past the shot on the left
func _far_aside() -> void:
    for key:Key in [KEY_SHIFT, KEY_RIGHT]:
        var press:InputEventKey = InputEventKey.new()
        press.physical_keycode = key
        press.keycode = key
        press.pressed = true
        Input.parse_input_event(press)
    await get_tree().create_timer(FAR_ASIDE_SECONDS).timeout
    for key:Key in [KEY_RIGHT, KEY_SHIFT]:
        var release:InputEventKey = InputEventKey.new()
        release.physical_keycode = key
        release.keycode = key
        release.pressed = false
        Input.parse_input_event(release)
    await wait_idle_frames(SETTLE_FRAMES)


# the point the view turns around: as far ahead of it as the vehicle was from the view unmoved
func _pivot(p_distance:float) -> Vector3:
    return _view.global_position + _forward() * p_distance


func test_facing_away_from_the_vehicle_the_operator_does_nothing() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _far_aside()
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return
    # turned until the vehicle is behind the view
    for _step:int in 2 * ORBIT_STEPS:
        if _forward().dot(_center() - _view.global_position) < 0.0:
            break
        await _orbit(Vector2(ORBIT_STEP_PX, 0.0), 1)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_lt(_forward().dot(_center() - _view.global_position), 0.0, "the vehicle is behind the view")
    var position:Vector3 = _view.global_position

    await _orbit(Vector2(ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
    await _orbit(Vector2(-ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_view.global_position.distance_to(position), 0.0, TOLERANCE, "the shot is as it was")


func test_a_small_orbit_keeps_a_shot_moved_aside() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _hold_key(KEY_RIGHT, KEY_FRAMES)
    var aside:float = _on_screen().x
    assert_lt(aside, -OFF_MIDDLE, "the vehicle stands left of the middle")
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return

    await _orbit(Vector2(ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_lt(_on_screen().x, -OFF_MIDDLE, "it stays aside, nothing set back")
    assert_lt(absf(_on_screen().x), FollowView.FRAME_SHARE + TOLERANCE, "within the shot")


func test_turning_away_from_the_vehicle_the_operator_does_nothing() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var distance:float = _distance()
    await _far_aside()
    assert_lt(_on_screen().x, -FollowView.FRAME_SHARE, "the vehicle is past the shot on the left")
    var pivot:Vector3 = _pivot(distance)
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return

    # the mouse going right turns the view to the right, away from the vehicle
    await _orbit(Vector2(ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_pivot(distance).distance_to(pivot), 0.0, TOLERANCE, "the view turns around the same point")


func test_turning_to_the_vehicle_the_operator_brings_it_into_the_shot_across_the_screen() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var distance:float = _distance()
    await _far_aside()
    var pivot:Vector3 = _pivot(distance)
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return

    # the mouse going left turns the view to the left, to the vehicle
    await _orbit(Vector2(-ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    var moved:Vector3 = _pivot(distance) - pivot
    assert_lt(_pivot(distance).distance_to(_center()), pivot.distance_to(_center()), "towards the vehicle")
    assert_almost_eq(moved.y, 0.0, TOLERANCE, "not up nor down")


func test_turning_to_the_vehicle_and_back_the_view_never_comes_nearer_it() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _wheel(MOUSE_BUTTON_WHEEL_DOWN, ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    await _far_aside()
    var distance:float = _distance()
    if not await _grab_mouse():
        pending("the display server cannot capture the mouse")
        return

    for _turn:int in TURNS_TO_AND_BACK:
        await _orbit(Vector2(-ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
        await _orbit(Vector2(ORBIT_STEP_PX, 0.0), SMALL_ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_gt(_distance(), distance * (1.0 - NEARER_SHARE))


func test_a_shot_moved_aside_stays_with_its_view() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE)
    var bogie:Transform3D = _view.global_transform
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _hold_key(KEY_RIGHT, KEY_FRAMES)

    _view.view = PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_view.global_position.distance_to(bogie.origin), 0.0, TOLERANCE,
            "the bogie view stands as its own")
    assert_almost_eq(_forward().angle_to(-bogie.basis.z), 0.0, TOLERANCE, "and looks as its own")


func test_cycling_the_views_turns_the_view_without_turning_back() -> void:
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    _view.response = GAME_RESPONSE

    for next:PlayerCameraServer.CameraFollowView in [PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE,
            PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY,
            PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT]:
        _view.view = next
        # how far the heading and the pitch go in all, and how far they get
        var travel:Vector2 = Vector2.ZERO
        var turn:Vector2 = Vector2.ZERO
        var previous:Vector3 = _forward()
        for _frame:int in FLIGHT_FRAMES:
            await wait_idle_frames(1)
            var current:Vector3 = _forward()
            var step:Vector2 = Vector2(angle_difference(atan2(previous.x, previous.z), atan2(current.x, current.z)),
                    asin(current.y) - asin(previous.y))
            travel += step.abs()
            turn += step
            previous = current
        var name:String = str(next)
        assert_almost_eq(travel.x, absf(turn.x), TOLERANCE, "to %s the heading turns one way" % name)
        assert_almost_eq(travel.y, absf(turn.y), TOLERANCE, "to %s the pitch turns one way" % name)


func test_out_of_the_cab_the_vehicle_view_comes_first() -> void:
    PlayerServer.player_enter_vehicle(_vehicle)
    PlayerCameraServer.camera_set_follow_view(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    PlayerCameraServer.camera_cycle_follow_view()

    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FOLLOW)
    assert_eq(PlayerCameraServer.camera_get_follow_view(), PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    PlayerCameraServer.camera_set_target(RID())


func test_leaving_the_vehicle_resets_the_offsets() -> void:
    PlayerServer.player_enter_vehicle(_vehicle)
    await _follow(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var distance:float = _distance()
    await _wheel(MOUSE_BUTTON_WHEEL_DOWN, ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_gt(_distance(), distance + 1.0, "the wheel takes the view farther")

    PlayerServer.player_leave_vehicle()
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_distance(), distance, TOLERANCE)
