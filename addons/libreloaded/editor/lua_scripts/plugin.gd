@tool
extends EditorPlugin

## Lua scripts in the editor. The script editor opens them as text and colours them as Lua
## (lua_syntax_highlighter.gd); a script context whose files changed on disk - its script, a module it
## required - runs again at once (ScenarioScriptServer.context_reload()), so a node drawing a texture
## shows the saved script without the scene being reloaded.

const LuaSyntaxHighlighter = preload("./lua_syntax_highlighter.gd")
## The files the editor treats as text: Lua is added to them, so the FileSystem dock lists the scripts
## and the script editor opens them
const TEXT_FILE_EXTENSIONS_SETTING:String = "docks/filesystem/textfile_extensions"
const LUA_EXTENSION:String = "lua"
const EXTENSION_SEPARATOR:String = ","

var _highlighter:LuaSyntaxHighlighter = null
## The newest modification time of each context's files, when it was looked at last [unix s]
var _file_times:Dictionary[RID, int] = {}


func _enter_tree() -> void:
    var settings:EditorSettings = EditorInterface.get_editor_settings()
    var extensions:PackedStringArray = String(settings.get_setting(TEXT_FILE_EXTENSIONS_SETTING)).split(
            EXTENSION_SEPARATOR, false)
    if not LUA_EXTENSION in extensions:
        extensions.append(LUA_EXTENSION)
        settings.set_setting(TEXT_FILE_EXTENSIONS_SETTING, EXTENSION_SEPARATOR.join(extensions))
    _highlighter = LuaSyntaxHighlighter.new()
    EditorInterface.get_script_editor().register_syntax_highlighter(_highlighter)
    EditorInterface.get_resource_filesystem().filesystem_changed.connect(_on_filesystem_changed)


func _exit_tree() -> void:
    EditorInterface.get_resource_filesystem().filesystem_changed.disconnect(_on_filesystem_changed)
    EditorInterface.get_script_editor().unregister_syntax_highlighter(_highlighter)
    _highlighter = null


## A context seen for the first time is only noted: it has just run what is on disk
func _on_filesystem_changed() -> void:
    var times:Dictionary[RID, int] = {}
    for context:RID in ScenarioScriptServer.context_get_rids():
        var newest:int = _newest_time(context)
        if _file_times.has(context) and newest > _file_times[context]:
            ScenarioScriptServer.context_reload(context)
            newest = _newest_time(context)
        times[context] = newest
    _file_times = times


func _newest_time(context:RID) -> int:
    var newest:int = 0
    for path:String in ScenarioScriptServer.context_get_files(context):
        newest = maxi(newest, FileAccess.get_modified_time(path))
    return newest
