extends ReloadedGutTest

## DrawingServer's drawings, fonts and images, and the scripts' reloaded.draw. The pixels the GPU
## draws are not read here - a headless run has no renderer; what is checked is what the server
## and the scripts keep and measure.

const FIXTURES:String = "res://tests/fixtures/drawing"
const FONT:String = FIXTURES + "/DejaVuSansMono.ttf"
const SPRITE:String = FIXTURES + "/sprite.png"
const MASK:String = FIXTURES + "/mask.png"
const DRAWING_SIZE:Vector2i = Vector2i(16, 8)
const OUTPUT_SIZE:Vector2i = Vector2i(32, 16)
const SMALL_TEXT_SIZE:int = 16
const BIG_TEXT_SIZE:int = 32

var _context:RID


func before_each() -> void:
    _context = ScenarioScriptServer.context_create(FIXTURES)
    watch_signals(ScenarioScriptServer)


func after_each() -> void:
    ScenarioScriptServer.context_free(_context)


## The last script error, for an assert's message
func _error() -> String:
    var parameters:Variant = get_signal_parameters(ScenarioScriptServer, "script_error")
    return str(parameters[1]) if parameters is Array else ""


func test_a_drawing_has_its_size_and_a_texture_until_freed() -> void:
    var drawing:RID = DrawingServer.drawing_create(DRAWING_SIZE, false)

    assert_eq(DrawingServer.drawing_get_size(drawing), DRAWING_SIZE)
    assert_true(DrawingServer.owns(drawing))
    DrawingServer.free_rid(drawing)
    assert_false(DrawingServer.owns(drawing), "freed, it is gone")


## Which texture the drawing ends in needs a renderer, which a headless run has not
func test_a_material_leaves_the_drawing_its_size() -> void:
    var drawing:RID = DrawingServer.drawing_create(DRAWING_SIZE, true)
    var material:ShaderMaterial = ShaderMaterial.new()

    DrawingServer.drawing_set_material(drawing, material.get_rid(), OUTPUT_SIZE)
    assert_eq(DrawingServer.drawing_get_size(drawing), DRAWING_SIZE)
    DrawingServer.drawing_set_material(drawing, RID(), Vector2i())
    assert_eq(DrawingServer.drawing_get_size(drawing), DRAWING_SIZE)
    DrawingServer.free_rid(drawing)


## The picture turned over is a renderer's to show; headless, the drawing takes the flip with and
## without a material and keeps its size
func test_a_drawing_is_turned_over_through_a_material_too() -> void:
    var drawing:RID = DrawingServer.drawing_create(DRAWING_SIZE, false)
    var material:ShaderMaterial = ShaderMaterial.new()

    DrawingServer.drawing_set_flip_y(drawing, true)
    DrawingServer.drawing_set_material(drawing, material.get_rid(), OUTPUT_SIZE)
    DrawingServer.drawing_set_flip_y(drawing, false)
    assert_eq(DrawingServer.drawing_get_size(drawing), DRAWING_SIZE)
    assert_engine_error_count(0)
    DrawingServer.free_rid(drawing)


func test_a_font_measures_its_text() -> void:
    var font:RID = DrawingServer.font_load(FONT)

    assert_true(font.is_valid())
    var one:Vector2 = DrawingServer.font_get_text_size(font, "A", SMALL_TEXT_SIZE)
    var two:Vector2 = DrawingServer.font_get_text_size(font, "AA", SMALL_TEXT_SIZE)
    assert_gt(one.x, 0.0)
    assert_gt(two.x, one.x, "more letters, wider")
    assert_gt(DrawingServer.font_get_text_size(font, "A", BIG_TEXT_SIZE).y, one.y, "a bigger size is taller")
    DrawingServer.free_rid(font)


func test_an_image_keeps_its_pixels() -> void:
    var image:RID = DrawingServer.image_load(SPRITE)

    assert_eq(DrawingServer.image_get_size(image), Vector2i(2, 2))
    assert_eq(DrawingServer.image_get_pixel(image, Vector2i(0, 0)), Color(1, 0, 0, 1))
    assert_eq(DrawingServer.image_get_pixel(image, Vector2i(0, 1)), Color(0, 0, 1, 1))
    assert_eq(DrawingServer.image_get_pixel(image, Vector2i(1, 1)).a, 0.0, "its alpha kept")
    DrawingServer.free_rid(image)


func test_a_mask_without_alpha_is_as_opaque_as_it_is_bright() -> void:
    var mask:RID = DrawingServer.image_load_mask(MASK)

    assert_eq(DrawingServer.image_get_pixel(mask, Vector2i(0, 0)), Color(1, 1, 1, 0), "black: transparent")
    assert_eq(DrawingServer.image_get_pixel(mask, Vector2i(1, 0)), Color(1, 1, 1, 1), "white: opaque")
    DrawingServer.free_rid(mask)


func test_a_script_draws_and_measures() -> void:
    var source:String = (
        "local d = reloaded.draw.create(16, 8)\n"
        + "local w, h = reloaded.draw.size(d)\n"
        + "assert(w == 16 and h == 8)\n"
        + "local font = reloaded.draw.load_font('DejaVuSansMono.ttf')\n"
        + "local sharp = reloaded.draw.load_font('DejaVuSansMono.ttf', 'none')\n"
        + "assert(reloaded.draw.text_size(sharp, 'A', 16) > 0)\n"
        + "local one = reloaded.draw.text_size(font, 'A', 16)\n"
        + "local two = reloaded.draw.text_size(font, 'AA', 16)\n"
        + "assert(two > one)\n"
        + "local sprite = reloaded.draw.load_image('sprite.png')\n"
        + "assert(select(1, reloaded.draw.image_size(sprite)) == 2)\n"
        + "local red = reloaded.draw.image_pixel(sprite, 0, 0)\n"
        + "assert(red[1] == 255 and red[2] == 0 and red[3] == 0 and red[4] == 255)\n"
        + "local mask = reloaded.draw.load_mask('mask.png')\n"
        + "reloaded.draw.clear(d, {0, 0, 0})\n"
        + "reloaded.draw.rect(d, 0, 0, 4, 4, {255, 255, 0})\n"
        + "reloaded.draw.rect_outline(d, 0, 0, 16, 8, {255, 255, 255}, 1)\n"
        + "reloaded.draw.line(d, {0, 0, 15, 7, 0, 7}, {0, 255, 0}, 2)\n"
        + "reloaded.draw.ellipse(d, 2, 2, 6, 4, {0, 0, 255, 128})\n"
        + "reloaded.draw.ellipse_outline(d, 2, 2, 6, 4, {0, 0, 255})\n"
        + "reloaded.draw.polygon(d, {0, 0, 4, 0, 2, 4}, {255, 0, 255})\n"
        + "reloaded.draw.polygon_outline(d, {0, 0, 4, 0, 2, 4}, {255, 0, 255})\n"
        + "reloaded.draw.pieslice(d, 8, 0, 8, 8, -90, 45, {255, 128, 0})\n"
        + "reloaded.draw.pixel(d, 15, 7, {255, 255, 255})\n"
        + "reloaded.draw.text(d, 0, 0, 'A', font, 8, {255, 255, 255})\n"
        + "reloaded.draw.paste(d, sprite, 4, 4)\n"
        + "reloaded.draw.paste(d, sprite, 6, 4, {w = 4, h = 4, filter = 'linear', flip_x = true})\n"
        + "reloaded.draw.paste(d, sprite, 10, 4, {source = {0, 0, 1, 1}})\n"
        + "reloaded.draw.paste(d, mask, 12, 4, {tint = {255, 0, 0}})\n"
        + "reloaded.draw.paste_rotated(d, sprite, 8, 4, 90)\n"
        + "local other = reloaded.draw.create(4, 4, true)\n"
        + "reloaded.draw.paste(d, other, 0, 0)\n"
        + "assert(reloaded.draw.free(other))\n"
        + "assert(not reloaded.draw.free(other), 'freed already')\n"
    )

    assert_true(ScenarioScriptServer.context_apply_source(_context, &"draw", source), _error())


func test_a_script_reads_no_data_outside_its_directory() -> void:
    watch_signals(ScenarioScriptServer)

    assert_false(ScenarioScriptServer.context_apply_source(
            _context, &"outside", "reloaded.draw.load_font('../DejaVuSansMono.ttf')"))
    assert_signal_emitted(ScenarioScriptServer, "script_error")


func test_the_data_directory_is_set_for_the_context() -> void:
    ScenarioScriptServer.context_set_data_dir(_context, FIXTURES.get_base_dir())

    assert_true(ScenarioScriptServer.context_apply_source(
            _context, &"data", "reloaded.draw.load_font('drawing/DejaVuSansMono.ttf')"))


## What a unit made goes when it is applied again; another unit's stays
func test_what_a_unit_made_is_freed_with_it() -> void:
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"made", "made = reloaded.draw.create(4, 4)"))
    assert_true(ScenarioScriptServer.context_apply_source(_context, &"kept", "kept = reloaded.draw.create(4, 4)"))

    assert_true(ScenarioScriptServer.context_apply_source(
            _context, &"made", "assert(not reloaded.draw.free(made), 'the previous run made it')"))
    assert_true(ScenarioScriptServer.context_apply_source(
            _context, &"check", "assert(reloaded.draw.free(kept), 'another unit made it')"))
