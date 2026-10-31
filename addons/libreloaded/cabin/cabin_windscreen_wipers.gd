extends Node
class_name CabinWindscreenWipers

## Feeds the wipers of the occupied vehicle to the "rain_windscreen" materials, as the original
## renderer does for the owner of the camera (opengl33renderer.cpp:755-789): per wiper its
## position (0..1 sweeping out, 1..2 on the way back) and the time since it last left the parked
## position and the far end. The original passes the two as points in time; the shader clock is
## not available here, so they are passed as the time elapsed.

const WIPERS:int = 4
const NEVER:float = 1000.0
## The share of the sweep within which a wiper is parked, and past which it is at the far end
const PARKED_SWEEP:float = 0.025
const FAR_END_SWEEP:float = 0.975

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


func _process(frame_delta:float) -> void:
    var delta:float = _clock.advance(frame_delta)
    var positions:PackedFloat64Array = CabinSystem.vehicle_state(vehicle_rid).get("wiper_positions", PackedFloat64Array())
    var cab:int = CabinSystem.vehicle_state(vehicle_rid).get("cabin", 0)
    var wiper_pos:Vector4 = Vector4.ZERO
    for i:int in WIPERS:
        if cab == 0 or i >= positions.size():
            _since_out[i] = NEVER
            _since_return[i] = NEVER
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
    _apply(wiper_pos)


func _exit_tree() -> void:
    _since_out = Vector4(NEVER, NEVER, NEVER, NEVER)
    _since_return = Vector4(NEVER, NEVER, NEVER, NEVER)
    _parked = [true, true, true, true]
    _at_far_end = [false, false, false, false]
    _apply(Vector4.ZERO)


func _apply(wiper_pos:Vector4) -> void:
    RenderingServer.global_shader_parameter_set("reloaded_wiper_pos", wiper_pos)
    RenderingServer.global_shader_parameter_set("reloaded_wiper_since_out", _since_out)
    RenderingServer.global_shader_parameter_set("reloaded_wiper_since_return", _since_return)
