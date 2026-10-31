// The environment's snow laid over a material that has none of its own (ground_weather.gdshaderinc):
// the next pass of an opaque surface - a big world's roofs and walls (WorldMeshPlacement) - drawn on
// the same triangles, the snow where it lies on what faces up or where the wind drove it onto a wall,
// and nothing elsewhere.
shader_type spatial;
render_mode blend_mix, depth_draw_never, cull_back, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libreloaded/environment/ground_weather.gdshaderinc"

// the surface's own normal map (WorldMeshPlacement.NORMAL_TEXTURE), the snow lying on what it turns up
// and driven onto what it turns into the wind; flat without one
uniform sampler2D texture_normal : hint_normal, filter_linear_mipmap, repeat_enable;

varying vec3 world_position;

void vertex() {
	// not INV_VIEW_MATRIX * VERTEX in fragment(): a double precision build keeps the view's origin
	// there its own way, kilometres off far from the world's origin
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
	// as Godot lays a NORMAL_MAP on NORMAL
	vec3 map = texture(texture_normal, UV).rgb * 2.0 - 1.0;
	map.z = sqrt(max(0.0, 1.0 - dot(map.xy, map.xy)));
	vec3 mapped = normalize(TANGENT * map.x + BINORMAL * map.y + NORMAL * map.z);
	vec3 surface_normal = normalize(mat3(INV_VIEW_MATRIX) * mapped);
	float snow = reloaded_weather_snow(surface_normal, world_position);
	if (snow <= 0.001) {
		discard;
	}
	// the weather over a black, glossless surface that holds no water is the snow's colour and
	// roughness times its cover: divided by it, the snow's own, laid on at its cover
	vec3 albedo = vec3(0.0);
	float roughness = 0.0;
	reloaded_weather_surface_puddled(albedo, roughness, surface_normal, world_position, 0.0);
	ALBEDO = albedo / snow;
	ROUGHNESS = roughness / snow;
	NORMAL = mapped;
	ALPHA = snow;
}
