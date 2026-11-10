-- What a GDScript subclass adds follows the drawing
local drawing, number, text = ...
assert(drawing ~= nil)
assert(number == 42 and text == "added", "the subclass's arguments follow the drawing")
