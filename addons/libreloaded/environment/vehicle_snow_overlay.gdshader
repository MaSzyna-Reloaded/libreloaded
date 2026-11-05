// The snow lying on a vehicle, laid over its own materials: the material overlay of its exterior and
// attachments (VehicleRenderingServer), each vehicle drawn in detail with snow on it a copy of its
// own. On what faces up - its roof, its frame and walkways, a wagon's floor - a dusting, then patches
// that run together, never all of it. In patches that ride with the model; nothing elsewhere.
shader_type spatial;
render_mode blend_mix, depth_draw_never, cull_back, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libreloaded/environment/ground_weather.gdshaderinc"

// how much snow lies on the vehicle (VehicleRenderingServer, 0 - 1)
uniform float vehicle_snow = 0.0;

// where the surface is in its model: the patches ride with the vehicle
varying vec3 model_position;
varying vec3 world_normal;

void vertex() {
	model_position = VERTEX;
	world_normal = normalize(mat3(MODEL_MATRIX) * NORMAL);
}

void fragment() {
	float amount = clamp(vehicle_snow, 0.0, 1.0);
	vec3 normal = normalize(world_normal);
	float up = smoothstep(0.45, 0.9, normal.y);
	// two octaves in the model's own space, turned against each other so the noise's grid does not show
	vec2 p = model_position.xz * 1.8;
	float patches = reloaded_weather_noise(p) * 0.65
		+ reloaded_weather_noise(mat2(vec2(0.8, 0.6), vec2(-0.6, 0.8)) * p * 2.4 + 5.7) * 0.35;
	// a thin dusting from the first of it, the patches growing as the square root - a vehicle
	// snowed on for a while shows it - and some three quarters of what faces up at the most
	float dusting = smoothstep(0.0, 0.3, amount) * mix(0.3, 0.5, patches);
	float cover = sqrt(amount) * 0.65;
	float lying = smoothstep(1.0 - cover * 1.3, 1.0 - cover * 1.3 + 0.3, patches);
	float snow = up * max(dusting, lying);
	if (snow <= 0.001) {
		discard;
	}
	ALBEDO = RELOADED_SNOW_ALBEDO;
	ROUGHNESS = 0.75;
	ALPHA = snow;
}
