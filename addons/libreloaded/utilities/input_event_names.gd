class_name InputEventNames

## A key or a pad's button or axis named for the player to read: a punctuation key by its sign and a
## numpad key by its sign or digit ("Kp -", not Godot's "Kp Subtract"), with the modifiers before it
## ("Ctrl+Shift+W"); a pad's button or axis by its label on a pad of the layout given, after "Pad"
## ("Pad A", "Pad Cross", "Pad ZL", "Pad LS Left"), so it is not read as a key. Godot maps every pad
## onto one set of buttons by their place (SDL's: JOY_BUTTON_A is the bottom one), so a button is
## one JoyButton on every pad and only its label differs. Every place that shows a key names it here.

## The labels a pad's buttons carry, by its maker - an unknown pad is labelled as an Xbox pad, whose
## labels SDL's buttons are named after
enum PadLayout {
    XBOX,
    PLAYSTATION,
    NINTENDO,
}

## Godot's names of the keys a player reads better as their signs (OS.get_keycode_string())
const KEY_NAMES:Dictionary[Key, String] = {
    KEY_APOSTROPHE: "'",
    KEY_QUOTELEFT: "`",
    KEY_BACKSLASH: "\\",
    KEY_BRACKETLEFT: "[",
    KEY_BRACKETRIGHT: "]",
    KEY_COMMA: ",",
    KEY_EQUAL: "=",
    KEY_MINUS: "-",
    KEY_PERIOD: ".",
    KEY_SEMICOLON: ";",
    KEY_SLASH: "/",
    KEY_KP_ADD: "Kp +",
    KEY_KP_SUBTRACT: "Kp -",
    KEY_KP_MULTIPLY: "Kp *",
    KEY_KP_DIVIDE: "Kp /",
    KEY_KP_PERIOD: "Kp .",
    KEY_PAGEUP: "PgUp",
    KEY_PAGEDOWN: "PgDn",
}


## A pad's buttons (JoyButton) by their labels on an Xbox pad
const PAD_BUTTON_NAMES:Dictionary[JoyButton, String] = {
    JOY_BUTTON_A: "Pad A",
    JOY_BUTTON_B: "Pad B",
    JOY_BUTTON_X: "Pad X",
    JOY_BUTTON_Y: "Pad Y",
    JOY_BUTTON_BACK: "Pad View",
    JOY_BUTTON_GUIDE: "Pad Xbox",
    JOY_BUTTON_START: "Pad Menu",
    JOY_BUTTON_LEFT_STICK: "Pad LS",
    JOY_BUTTON_RIGHT_STICK: "Pad RS",
    JOY_BUTTON_LEFT_SHOULDER: "Pad LB",
    JOY_BUTTON_RIGHT_SHOULDER: "Pad RB",
    JOY_BUTTON_DPAD_UP: "Pad Up",
    JOY_BUTTON_DPAD_DOWN: "Pad Down",
    JOY_BUTTON_DPAD_LEFT: "Pad Left",
    JOY_BUTTON_DPAD_RIGHT: "Pad Right",
    JOY_BUTTON_MISC1: "Pad Share",
    JOY_BUTTON_PADDLE1: "Pad P1",
    JOY_BUTTON_PADDLE2: "Pad P2",
    JOY_BUTTON_PADDLE3: "Pad P3",
    JOY_BUTTON_PADDLE4: "Pad P4",
    JOY_BUTTON_TOUCHPAD: "Pad Touchpad",
}
## ... and the labels another maker's pads carry in place of an Xbox pad's - by the button's place:
## a Nintendo pad has A on the right and B at the bottom
const PAD_LAYOUT_BUTTON_NAMES:Dictionary[PadLayout, Dictionary] = {
    PadLayout.XBOX: {},
    PadLayout.PLAYSTATION: {
        JOY_BUTTON_A: "Pad Cross",
        JOY_BUTTON_B: "Pad Circle",
        JOY_BUTTON_X: "Pad Square",
        JOY_BUTTON_Y: "Pad Triangle",
        JOY_BUTTON_BACK: "Pad Create",
        JOY_BUTTON_GUIDE: "Pad PS",
        JOY_BUTTON_START: "Pad Options",
        JOY_BUTTON_LEFT_STICK: "Pad L3",
        JOY_BUTTON_RIGHT_STICK: "Pad R3",
        JOY_BUTTON_LEFT_SHOULDER: "Pad L1",
        JOY_BUTTON_RIGHT_SHOULDER: "Pad R1",
        JOY_BUTTON_MISC1: "Pad Mute",
    },
    PadLayout.NINTENDO: {
        JOY_BUTTON_A: "Pad B",
        JOY_BUTTON_B: "Pad A",
        JOY_BUTTON_X: "Pad Y",
        JOY_BUTTON_Y: "Pad X",
        JOY_BUTTON_BACK: "Pad -",
        JOY_BUTTON_GUIDE: "Pad Home",
        JOY_BUTTON_START: "Pad +",
        JOY_BUTTON_LEFT_SHOULDER: "Pad L",
        JOY_BUTTON_RIGHT_SHOULDER: "Pad R",
        JOY_BUTTON_MISC1: "Pad Capture",
    },
}
## A pad's axes (JoyAxis) pushed to the negative side - left, up - and to the positive one; a
## trigger has only the positive side
const PAD_AXIS_NEGATIVE_NAMES:Dictionary[JoyAxis, String] = {
    JOY_AXIS_LEFT_X: "Pad LS Left",
    JOY_AXIS_LEFT_Y: "Pad LS Up",
    JOY_AXIS_RIGHT_X: "Pad RS Left",
    JOY_AXIS_RIGHT_Y: "Pad RS Up",
}
const PAD_AXIS_POSITIVE_NAMES:Dictionary[JoyAxis, String] = {
    JOY_AXIS_LEFT_X: "Pad LS Right",
    JOY_AXIS_LEFT_Y: "Pad LS Down",
    JOY_AXIS_RIGHT_X: "Pad RS Right",
    JOY_AXIS_RIGHT_Y: "Pad RS Down",
    JOY_AXIS_TRIGGER_LEFT: "Pad LT",
    JOY_AXIS_TRIGGER_RIGHT: "Pad RT",
}
## The triggers' labels on another maker's pads
const PAD_LAYOUT_TRIGGER_NAMES:Dictionary[PadLayout, Dictionary] = {
    PadLayout.XBOX: {},
    PadLayout.PLAYSTATION: {JOY_AXIS_TRIGGER_LEFT: "Pad L2", JOY_AXIS_TRIGGER_RIGHT: "Pad R2"},
    PadLayout.NINTENDO: {JOY_AXIS_TRIGGER_LEFT: "Pad ZL", JOY_AXIS_TRIGGER_RIGHT: "Pad ZR"},
}
## The makers' USB vendor ids (Input.get_joy_info() "vendor_id")
const PAD_VENDORS:Dictionary[int, PadLayout] = {
    0x054C: PadLayout.PLAYSTATION,
    0x057E: PadLayout.NINTENDO,
}
## Words of a pad's name (Input.get_joy_name(), SDL's mapping database) that tell its maker, for a
## pad whose vendor id the platform does not give
const PAD_NAME_WORDS:Dictionary[String, PadLayout] = {
    "playstation": PadLayout.PLAYSTATION,
    "dualshock": PadLayout.PLAYSTATION,
    "dualsense": PadLayout.PLAYSTATION,
    "ps3": PadLayout.PLAYSTATION,
    "ps4": PadLayout.PLAYSTATION,
    "ps5": PadLayout.PLAYSTATION,
    "nintendo": PadLayout.NINTENDO,
    "switch": PadLayout.NINTENDO,
    "joy-con": PadLayout.NINTENDO,
}
## A button or an axis of no label, by its number
const PAD_BUTTON_FALLBACK:String = "Pad Button %d"
const PAD_AXIS_FALLBACK:String = "Pad Axis %d%s"


## `key` (with its modifier mask, as InputEventKey.get_keycode_with_modifiers() gives it)
static func key_name(key:Key) -> String:
    var code:Key = key & KEY_CODE_MASK
    var modifiers:String = OS.get_keycode_string(key & KEY_MODIFIER_MASK)
    var name:String = KEY_NAMES.get(code, OS.get_keycode_string(code))
    return modifiers + "+" + name if modifiers else name


## The labels of the pad `device` (an InputEvent's device): its maker by its vendor id, or by its
## name; an Xbox pad's for any other, and for a pad not connected (the engine has no data of it)
static func pad_layout(device:int) -> PadLayout:
    if not device in Input.get_connected_joypads():
        return PadLayout.XBOX
    var vendor:int = int(Input.get_joy_info(device).get("vendor_id", 0))
    if PAD_VENDORS.has(vendor):
        return PAD_VENDORS[vendor]
    var pad_name:String = Input.get_joy_name(device).to_lower()
    for word:String in PAD_NAME_WORDS:
        if word in pad_name:
            return PAD_NAME_WORDS[word]
    return PadLayout.XBOX


## `event` - a key by its key code (its physical one when it has none), a pad's button by its
## label on a pad of `layout`, a pad's axis by its label and the side it is pushed to
static func event_name(event:InputEvent, layout:PadLayout = PadLayout.XBOX) -> String:
    if event is InputEventKey:
        var key:InputEventKey = event as InputEventKey
        return key_name(key.get_keycode_with_modifiers() if key.keycode else key.get_physical_keycode_with_modifiers())
    # not as_text(): it names the button on every pad ("Joypad Button 0 (Bottom Action, Sony Cross,
    # ...)"), wider than a window, which cannot be narrower than its widest row
    if event is InputEventJoypadButton:
        var button:JoyButton = (event as InputEventJoypadButton).button_index
        return PAD_LAYOUT_BUTTON_NAMES[layout].get(
                button, PAD_BUTTON_NAMES.get(button, PAD_BUTTON_FALLBACK % button))
    if event is InputEventJoypadMotion:
        var motion:InputEventJoypadMotion = event as InputEventJoypadMotion
        if PAD_LAYOUT_TRIGGER_NAMES[layout].has(motion.axis):
            return PAD_LAYOUT_TRIGGER_NAMES[layout][motion.axis]
        var names:Dictionary[JoyAxis, String] = (
                PAD_AXIS_NEGATIVE_NAMES if motion.axis_value < 0.0 else PAD_AXIS_POSITIVE_NAMES)
        return names.get(motion.axis, PAD_AXIS_FALLBACK % [motion.axis, "-" if motion.axis_value < 0.0 else "+"])
    return event.as_text()
