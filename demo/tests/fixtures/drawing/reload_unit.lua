-- Counts its runs in the runtime it runs in, and requires a module
local module = require("lib.reload_module")
runs = (runs or 0) + 1
assert(module.loaded)
