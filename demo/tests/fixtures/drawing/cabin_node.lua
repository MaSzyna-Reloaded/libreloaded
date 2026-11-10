-- A LuaCabinTextureNode's script: its drawing, the cab's vehicle and cabin
local drawing, vehicle, cabin = ...
assert(drawing ~= nil and vehicle ~= nil and cabin ~= nil, "the cab's vehicle and cabin follow the drawing")
reloaded.cabin.control(cabin, "screen")
