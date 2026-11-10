-- A LuaTextureNode's script: its drawing, and nothing after it
local drawing, extra = ...
local width, height = reloaded.draw.size(drawing)
assert(width == 16 and height == 8, "the drawing has the texture's size")
assert(extra == nil, "nothing follows the drawing")
reloaded.draw.clear(drawing, {255, 0, 0})
