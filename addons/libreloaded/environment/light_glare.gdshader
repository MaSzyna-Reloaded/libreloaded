// The glare of a model's lit lamp seen against the dark - how an eye or a lens sees a bright point:
// a soft core, a faint ring tinted at its rim and thin rays of diffraction, all of the lamp's colour,
// drawn on a quad that faces the camera, added on top. Nothing of a texture: every part is a function
// of the distance from the lamp's centre and of the angle round it.
// - the lamp's level (glare_level, its channel's) scales it, a dimmed lamp a smaller star
// - whole within the lamp's beam (cos_outer; -1 for a point), shrinking and fading out from its edge
//   to none seen square to the lamp's axis, none behind it
// - by day it fades away (reloaded_light_level, the environment's)
// - the rays turn as the camera goes round the lamp, not on their own
// - from afar it keeps min_screen_size of the screen's height
// - hidden by what stands between it and the camera: the scene's depth sampled at its centre and on a
//   ring round it, the glare fading with the share of samples that see it
shader_type spatial;
render_mode unshaded, blend_add, depth_draw_never, depth_test_disabled, cull_disabled, shadows_disabled, fog_disabled;

global uniform float reloaded_light_level;

uniform sampler2D depth_texture : hint_depth_texture, filter_nearest;

instance uniform vec3 light_color : source_color = vec3(1.0);
instance uniform float glare_level = 0.0;
instance uniform float cos_outer = -1.0;

// the quad's size [m] near the camera, and the least of the screen's height it keeps from afar
uniform float glare_size = 1.6;
uniform float min_screen_size = 0.08;
// the light level from which the glare is gone, and the one up to which it is whole
uniform float day_light_level = 0.6;
uniform float night_light_level = 0.15;
uniform float intensity = 1.0;
// the rays: how many, how thin, how far they reach of the quad's half
uniform int ray_count = 6;
uniform float ray_width = 120.0;
uniform float ray_length = 0.7;
// how much the rays turn as the camera goes round the lamp [turns a radian]
uniform float ray_turn = 0.35;
// the depth samples' ring [of the screen], and how far behind the lamp's centre a surface still
// leaves it seen [m] - the lamp's own lens and housing
uniform float occlusion_ring = 0.004;
uniform float occlusion_tolerance = 0.25;
const int OCCLUSION_SAMPLES = 8;
const float TAU_ = 6.28318530718;

varying float glare_strength;
varying float ray_angle;
varying vec3 centre_view;

void vertex() {
	vec3 centre_world = MODEL_MATRIX[3].xyz;
	centre_view = (VIEW_MATRIX * vec4(centre_world, 1.0)).xyz;
	// towards the beam: the angle between the lamp's axis and the camera
	vec3 axis = normalize(-MODEL_MATRIX[2].xyz);
	vec3 to_camera = normalize(CAMERA_POSITION_WORLD - centre_world);
	float facing = dot(axis, to_camera);
	float beam = cos_outer <= -1.0 ? 1.0 : smoothstep(0.0, cos_outer, facing);
	float night = 1.0 - smoothstep(night_light_level, day_light_level, reloaded_light_level);
	glare_strength = clamp(glare_level, 0.0, 2.0) * beam * night * intensity;
	// the rays turn with the camera's way round the lamp
	vec3 side = normalize(MODEL_MATRIX[0].xyz);
	ray_angle = atan(dot(to_camera, side), dot(to_camera, axis)) * ray_turn;

	// as tall as the least share of the screen from afar: its height projected, of the screen's 2;
	// smaller as it is dimmer off the beam, the whole star - not its core alone
	vec4 bottom = PROJECTION_MATRIX * vec4(centre_view, 1.0);
	vec4 top = PROJECTION_MATRIX * vec4(centre_view + vec3(0.0, glare_size, 0.0), 1.0);
	float screen_size = abs(top.y / top.w - bottom.y / bottom.w) * 0.5;
	float size = glare_size * max(1.0, min_screen_size / max(screen_size, 0.0001)) * beam;
	// a quad facing the camera, at the lamp's centre
	VERTEX = vec3(VERTEX.xy * size, 0.0);
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(INV_VIEW_MATRIX[0], INV_VIEW_MATRIX[1], INV_VIEW_MATRIX[2], MODEL_MATRIX[3]);
}

float seen(vec2 uv_screen, vec2 ring_size, mat4 inv_projection) {
	float visible = 0.0;
	for (int index = 0; index < OCCLUSION_SAMPLES; index++) {
		float angle = TAU_ * float(index) / float(OCCLUSION_SAMPLES);
		vec2 at = uv_screen + vec2(cos(angle), sin(angle)) * ring_size * float(index > 0);
		float depth = texture(depth_texture, at).r;
#if CURRENT_RENDERER == RENDERER_COMPATIBILITY
		vec4 view = inv_projection * vec4(at * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
#else
		vec4 view = inv_projection * vec4(at * 2.0 - 1.0, depth, 1.0);
#endif
		visible += view.z / view.w <= centre_view.z + occlusion_tolerance ? 1.0 : 0.0;
	}
	return visible / float(OCCLUSION_SAMPLES);
}

void fragment() {
	if (glare_strength <= 0.0001) {
		discard;
	}
	vec2 position = UV * 2.0 - 1.0;
	float radius = length(position);
	// a soft core, and a wider veil about it
	float core = exp(-radius * radius * 50.0) * 0.7 + exp(-radius * 6.0) * 0.12;
	// a faint ring, its rim tinted warmer outside and colder inside
	float ring = exp(-pow((radius - 0.55) * 14.0, 2.0)) * 0.025;
	vec3 ring_tint = mix(vec3(0.7, 0.85, 1.0), vec3(1.0, 0.85, 0.7), smoothstep(0.5, 0.6, radius));
	// thin rays, each of its own length
	float rays = 0.0;
	for (int index = 0; index < ray_count; index++) {
		float angle = ray_angle + TAU_ * float(index) / float(ray_count);
		vec2 direction = vec2(cos(angle), sin(angle));
		float across = abs(position.x * direction.y - position.y * direction.x);
		float along = abs(dot(position, direction));
		float length_of = ray_length * (0.6 + 0.4 * fract(sin(float(index) * 12.9898) * 43758.5453));
		rays += exp(-across * ray_width * (0.4 + along)) * (1.0 - smoothstep(0.0, length_of, along));
	}
	rays *= 0.25;
	// the projected centre on the screen, to see whether anything hides it
	vec4 centre_clip = PROJECTION_MATRIX * vec4(centre_view, 1.0);
	vec2 centre_screen = centre_clip.xy / centre_clip.w * 0.5 + 0.5;
	float visible = seen(centre_screen, vec2(occlusion_ring * VIEWPORT_SIZE.y / VIEWPORT_SIZE.x, occlusion_ring), INV_PROJECTION_MATRIX);
	vec3 colour = light_color * (core + rays) + light_color * ring_tint * ring;
	ALBEDO = colour * glare_strength * visible * (1.0 - smoothstep(0.85, 1.0, radius));
}
