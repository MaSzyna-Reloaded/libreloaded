-- A LuaTextureNode drawing through its material: the drawing has its own size
local drawing = ...
local width, height = reloaded.draw.size(drawing)
assert(width == 8 and height == 4, "the drawing has the node's drawing size")
