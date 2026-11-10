@tool
extends EditorSyntaxHighlighter

## Lua in the script editor: its keywords, strings, comments and numbers in the colours of the editor's
## theme. A line is coloured on its own, so a block comment or a long string ([[ ]]) spanning lines is
## coloured on its first line only.

const KEYWORDS:PackedStringArray = [
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in",
    "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while",
]
const COMMENT:String = "--"
const ESCAPE:String = "\\"
const HIGHLIGHTING_SETTINGS:String = "text_editor/theme/highlighting/"
## The highlighting's key of a colour (EditorSyntaxHighlighter._get_line_syntax_highlighting())
const COLOR_KEY:String = "color"

var _text_color:Color
var _keyword_color:Color
var _string_color:Color
var _comment_color:Color
var _number_color:Color


func _get_name() -> String:
    return "Lua"


func _get_supported_languages() -> PackedStringArray:
    return PackedStringArray(["lua"])


func _update_cache() -> void:
    var settings:EditorSettings = EditorInterface.get_editor_settings()
    _text_color = settings.get_setting(HIGHLIGHTING_SETTINGS + "text_color")
    _keyword_color = settings.get_setting(HIGHLIGHTING_SETTINGS + "keyword_color")
    _string_color = settings.get_setting(HIGHLIGHTING_SETTINGS + "string_color")
    _comment_color = settings.get_setting(HIGHLIGHTING_SETTINGS + "comment_color")
    _number_color = settings.get_setting(HIGHLIGHTING_SETTINGS + "number_color")


func _get_line_syntax_highlighting(line:int) -> Dictionary:
    var text:String = get_text_edit().get_line(line)
    var colors:Dictionary = {0: {COLOR_KEY: _text_color}}
    var at:int = 0
    while at < text.length():
        var character:String = text[at]
        if text.substr(at, COMMENT.length()) == COMMENT:
            colors[at] = {COLOR_KEY: _comment_color}
            break
        var end:int = at + 1
        if character == "\"" or character == "'":
            while end < text.length() and not text[end] == character:
                end += 2 if text[end] == ESCAPE else 1
            end += 1
            colors[at] = {COLOR_KEY: _string_color}
        elif character.is_valid_int():
            while end < text.length() and (text[end].is_valid_hex_number() or text[end] in ".xX"):
                end += 1
            colors[at] = {COLOR_KEY: _number_color}
        elif character.is_valid_identifier():
            while end < text.length() and ("a" + text[end]).is_valid_identifier():
                end += 1
            if text.substr(at, end - at) in KEYWORDS:
                colors[at] = {COLOR_KEY: _keyword_color}
            else:
                at = end
                continue
        else:
            at = end
            continue
        colors[end] = {COLOR_KEY: _text_color}
        at = end
    return colors
