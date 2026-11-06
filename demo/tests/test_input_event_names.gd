extends ReloadedGutTest

## InputEventNames names a pad's button and axis by its label after "Pad", on a pad of the layout
## given - by the button's place, as Godot maps every pad - never by Godot's as_text(), wider than a
## window, and never as a bare letter a key could be read as.


func test_a_pad_button_is_named_by_its_label() -> void:
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_A)), "Pad A")
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_LEFT_SHOULDER)), "Pad LB")
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_DPAD_UP)), "Pad Up")


func test_a_pad_button_of_no_label_is_named_by_its_number() -> void:
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_SDL_MAX)),
            InputEventNames.PAD_BUTTON_FALLBACK % JOY_BUTTON_SDL_MAX)


func test_a_pad_axis_is_named_by_its_label_and_side() -> void:
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_LEFT_X, -1.0)), "Pad LS Left")
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_LEFT_X, 1.0)), "Pad LS Right")
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_RIGHT_Y, -1.0)), "Pad RS Up")
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_TRIGGER_RIGHT, 1.0)), "Pad RT")


func test_a_pad_button_is_named_by_its_label_on_the_layout_s_pad() -> void:
    var layout:InputEventNames.PadLayout = InputEventNames.PadLayout.PLAYSTATION
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_A), layout), "Pad Cross")
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_Y), layout), "Pad Triangle")
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_LEFT_SHOULDER), layout), "Pad L1")
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_TRIGGER_RIGHT, 1.0), layout), "Pad R2")
    # the same on every pad
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_DPAD_UP), layout), "Pad Up")
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_LEFT_X, -1.0), layout), "Pad LS Left")


func test_a_nintendo_pad_s_buttons_are_named_by_their_place() -> void:
    var layout:InputEventNames.PadLayout = InputEventNames.PadLayout.NINTENDO
    # the bottom button is B, the right one A
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_A), layout), "Pad B")
    assert_eq(InputEventNames.event_name(_button(JOY_BUTTON_B), layout), "Pad A")
    assert_eq(InputEventNames.event_name(_motion(JOY_AXIS_TRIGGER_LEFT, 1.0), layout), "Pad ZL")


func test_a_pad_of_no_maker_known_is_labelled_as_an_xbox_pad() -> void:
    # no pad is connected in a headless run
    assert_eq(InputEventNames.pad_layout(0), InputEventNames.PadLayout.XBOX)


func test_a_key_is_still_named_as_a_key() -> void:
    var key:InputEventKey = InputEventKey.new()
    key.keycode = KEY_A
    key.ctrl_pressed = true
    assert_eq(InputEventNames.event_name(key), "Ctrl+A")


func _button(index:JoyButton) -> InputEventJoypadButton:
    var event:InputEventJoypadButton = InputEventJoypadButton.new()
    event.button_index = index
    return event


func _motion(axis:JoyAxis, value:float) -> InputEventJoypadMotion:
    var event:InputEventJoypadMotion = InputEventJoypadMotion.new()
    event.axis = axis
    event.axis_value = value
    return event
