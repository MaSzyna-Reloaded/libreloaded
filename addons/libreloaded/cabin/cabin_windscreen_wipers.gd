extends Node
class_name CabinWindscreenWipers

## Feeds the wipers of the occupied vehicle to the "rain_windscreen" materials, as the original
## renderer does for the owner of the camera (opengl33renderer.cpp:755-789): per wiper its
## position (0..1 sweeping out, 1..2 on the way back) and the time since it last left the parked
## position and the far end. The original passes the two as points in time; the shader clock is
## not available here, so they are passed as the time elapsed.
##
## And when its blade last passed each stretch of its sweep (reloaded_wiper_pass_time, PASS_BINS
## stretches a row, a row a wiper, against reloaded_wiper_clock): the rain comes back on each spot of
## the glass from when the blade wiped it - a wiper moved by hand, back and forth and standing still
## anywhere, wipes no strokes the two times above can tell.

const WIPERS:int = 4
const NEVER:float = 1000.0
## The share of the sweep within which a wiper is parked, and past which it is at the far end
const PARKED_SWEEP:float = 0.025
const FAR_END_SWEEP:float = 0.975
## Stretches of a wiper's sweep its blade's passes are kept for, and when one never passed was
## passed - long enough before for the rain to have come back on it in full however slow
const PASS_BINS:int = 64
const PASSED_NEVER:float = -1.0e6

## Set by the factory before the node enters the tree.
## Which vehicle these wipers belong to; the state comes through CabinSystem.
var vehicle_rid:RID

var _since_out:Vector4 = Vector4(NEVER, NEVER, NEVER, NEVER)
var _since_return:Vector4 = Vector4(NEVER, NEVER, NEVER, NEVER)
## Where each wiper stood the last frame: parked, at the far end - the times are taken from the
## moment it leaves the one and reaches the other, not for as long as it stands there (a wiper parked
## for good kept its area wiped clean, the rain never coming back on it)
var _parked:Array[bool] = [true, true, true, true]
var _at_far_end:Array[bool] = [false, false, false, false]
## The windscreen's time - the simulation's, as the wipers it follows
var _clock:SimulationClock = SimulationClock.new()
## Its time so far, and when each blade last passed each stretch of its sweep (a row a wiper) - a
## stretch never passed long before - and where each blade was the last frame (-1: nowhere yet)
var _now:float = 0.0
var _pass_time:Image = Image.create_empty(PASS_BINS, WIPERS, false, Image.FORMAT_RF)
var _pass_texture:ImageTexture
var _last_sweep:Vector4 = -Vector4.ONE


func _ready() -> void:
    _pass_time.fill(Color(PASSED_NEVER, 0.0, 0.0))
    _pass_texture = ImageTexture.create_from_image(_pass_time)
    RenderingServer.global_shader_parameter_set("reloaded_wiper_pass_time", _pass_texture)


func _process(frame_delta:float) -> void:
    var delta:float = _clock.advance(frame_delta)
    _now += delta
    var passed:bool = false
    var positions:PackedFloat64Array = CabinSystem.vehicle_state(vehicle_rid).get("wiper_positions", PackedFloat64Array())
    var cab:int = CabinSystem.vehicle_state(vehicle_rid).get("cabin", 0)
    var wiper_pos:Vector4 = Vector4.ZERO
    for i:int in WIPERS:
        if cab == 0 or i >= positions.size():
            _since_out[i] = NEVER
            _since_return[i] = NEVER
            _last_sweep[i] = -1.0
            continue
        # the wipers are numbered from the side of the active cab
        var position:float = positions[i if cab > 0 else positions.size() - 1 - i]
        var sweep:float = position if position <= 1.0 else position - 1.0
        # The original hands the shader the plain position while the arms move eased
        # (smoothInterpolate(), DynObj.cpp:726), so the wiped edge runs up to a tenth of the sweep
        # away from the blade. Eased here as well, to keep the two together.
        var eased:float = sweep * sweep * (3.0 - 2.0 * sweep)
        wiper_pos[i] = eased if position <= 1.0 else eased + 1.0
        var parked:bool = sweep < PARKED_SWEEP
        var at_far_end:bool = sweep > FAR_END_SWEEP
        _since_out[i] = 0.0 if _parked[i] and not parked else _since_out[i] + delta
        _since_return[i] = 0.0 if at_far_end and not _at_far_end[i] else _since_return[i] + delta
        _parked[i] = parked
        _at_far_end[i] = at_far_end
        # the stretches the blade passed since the last frame, which way soever
        var from:float = eased if _last_sweep[i] < 0.0 else _last_sweep[i]
        if not from == eased:
            var first:int = clampi(int(minf(from, eased) * PASS_BINS), 0, PASS_BINS - 1)
            var last:int = clampi(int(maxf(from, eased) * PASS_BINS), 0, PASS_BINS - 1)
            for bin:int in range(first, last + 1):
                _pass_time.set_pixel(bin, i, Color(_now, 0.0, 0.0))
            passed = true
        _last_sweep[i] = eased
    if passed:
        _pass_texture.update(_pass_time)
    RenderingServer.global_shader_parameter_set("reloaded_wiper_clock", _now)
    _apply(wiper_pos)


func _exit_tree() -> void:
    _since_out = Vector4(NEVER, NEVER, NEVER, NEVER)
    _since_return = Vector4(NEVER, NEVER, NEVER, NEVER)
    _parked = [true, true, true, true]
    _at_far_end = [false, false, false, false]
    _last_sweep = -Vector4.ONE
    _pass_time.fill(Color(PASSED_NEVER, 0.0, 0.0))
    _pass_texture.update(_pass_time)
    _apply(Vector4.ZERO)


func _apply(wiper_pos:Vector4) -> void:
    RenderingServer.global_shader_parameter_set("reloaded_wiper_pos", wiper_pos)
    RenderingServer.global_shader_parameter_set("reloaded_wiper_since_out", _since_out)
    RenderingServer.global_shader_parameter_set("reloaded_wiper_since_return", _since_return)
