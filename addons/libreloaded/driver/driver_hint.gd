class_name DriverHint
extends RefCounted

## One step a driver would take now (DriverState.hints), for the player driving its vehicle: what
## it says, the cab control and gesture that do it, and whether the vehicle already shows it done.

## The step, translated, its parameter filled in
var text:String = ""
## The cab control that does it and how it is moved (CabinLogic.get_action()); an empty control for
## a step no control does
var control:StringName = &""
var gesture:CabinLogic.Gesture = CabinLogic.Gesture.PRESS
## The vehicle shows it done
var done:bool = false
