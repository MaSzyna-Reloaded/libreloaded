// The environment's weather on a track's and a road's surfaces (StandardTrackRenderingBackend,
// track_look.tres): the wet and the snow of ground_weather.gdshaderinc, lying as it lies on what the
// surface is - the open ground, a road cleared of it, a track's ballast.
shader_type spatial;
render_mode blend_mix, depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libreloaded/environment/ground_weather.gdshaderinc"

// the values of `surface`: the snow lies on it as on the open ground (a shoulder, a formation,
// concrete)
const int SURFACE_GROUND = 0;
// a road's carriageway: cleared of a thin snow but for the banks along its edges, snowed over by a
// deep one or a blizzard, its wheel paths worn to slush
const int SURFACE_ROAD = 1;
// a track's ballast: the snow sinks into it and the trains blow it off, it shows from a deeper one
const int SURFACE_BALLAST = 2;

uniform int surface : hint_enum("Ground", "Road", "Ballast") = 0;
uniform vec4 albedo : source_color = vec4(1.0);
uniform float roughness : hint_range(0.0, 1.0) = 1.0;
// how much of the rain would stand on it (ground_weather.gdshaderinc's puddling)
uniform float puddling : hint_range(0.0, 1.0) = 0.3;

// the depth (m) a road is cleared of the snow below and snowed over from
const vec2 ROAD_SNOWED_DEPTH = vec2(0.05, 0.15);
// the banks of the snow ploughed off the road along its edges: how wide (m), how far that wanders
// along it (0 - 1 of it, every so many metres) and how much more snow lies in them than on the open
const float ROAD_BANK_WIDTH = 0.5;
const float ROAD_BANK_WANDER = 0.4;
const float ROAD_BANK_WAVELENGTH = 7.0;
const float ROAD_BANK_SNOW = 4.0;
// the wheel paths of a lane, from the road's nearer edge (m): a car's wheels 1.5 m apart in the
// middle of a 3.5 m lane; how wide each is (m), how far it wanders across (m, every so many
// metres), how much of the snow is left in it and how much of it the traffic churns to slush
const vec2 ROAD_WHEEL_PATHS = vec2(1.0, 2.5);
const float ROAD_WHEEL_PATH_WIDTH = 0.5;
const float ROAD_WHEEL_PATH_WANDER = 0.3;
const float ROAD_WHEEL_PATH_WAVELENGTH = 25.0;
const float ROAD_WHEEL_PATH_SNOW = 0.1;
const float ROAD_WHEEL_PATH_CHURN = 0.5;
// the slush thrown to the road's edges (m from them), in patches a few metres long, while it is
// wet or thawing (the air between these temperatures, as the model's ice) - in the snow's depth
// (m) that lies on the open ground
const float ROAD_SLUSH_REACH = 1.2;
const float ROAD_SLUSH_PATCH_SIZE = 3.0;
const vec2 ROAD_THAW_TEMPERATURE = vec2(-1.0, 0.5);
const float ROAD_SLUSH_DEPTH = 0.01;
const float ROAD_SLUSH_ROUGHNESS = 0.3;
// the depth (m) the snow shows on ballast from and covers it at; the stones it lies between (m
// across), the least of it on one
const vec2 BALLAST_SNOW_DEPTH = vec2(0.03, 0.10);
const float BALLAST_STONE_SIZE = 0.08;
const float BALLAST_STONE_SNOW = 0.5;

varying vec3 world_position;
// a road's carriageway is one edge of the section (StandardTrackRenderingBackend::_add_road()), its
// UV.x 0 on the left and the road's width (m) on the right: 0 to 1 across it
varying float across;

void vertex() {
	// not INV_VIEW_MATRIX * VERTEX in fragment(): a double precision build keeps the view's origin
	// there its own way, kilometres off far from the world's origin
	world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	across = UV.x > 0.0 ? 1.0 : 0.0;
}

void fragment() {
	ALBEDO = albedo.rgb;
	ROUGHNESS = roughness;
	// nothing of the weather worked out while none lies on the surfaces
	if (reloaded_weather_surfaces_active) {
		vec3 surface_albedo = albedo.rgb;
		float surface_roughness = roughness;
		vec3 world_normal = mat3(INV_VIEW_MATRIX) * NORMAL;
		float snow_share = 1.0;
		float slush = 0.0;
		// the road's and the ballast's snow only while it lies
		if (reloaded_weather_snow_exists) {
			if (surface == SURFACE_ROAD) {
				// how far from the nearer edge, and which (0 left, 1 right); the width is UV.x / across
				float from_edge = across < 0.5 ? UV.x : UV.x * (1.0 - across) / across;
				float side = step(0.5, across);
				float along = UV.y;
				float snowed = smoothstep(ROAD_SNOWED_DEPTH.x, ROAD_SNOWED_DEPTH.y, reloaded_snow_depth);
				float bank_width = ROAD_BANK_WIDTH
						* (1.0 + (reloaded_weather_noise(vec2(along / ROAD_BANK_WAVELENGTH, side * 7.0)) * 2.0 - 1.0) * ROAD_BANK_WANDER);
				float bank = 1.0 - smoothstep(bank_width * 0.5, bank_width, from_edge);
				float wander = (reloaded_weather_noise(vec2(along / ROAD_WHEEL_PATH_WAVELENGTH, side * 5.0 + 3.0)) * 2.0 - 1.0)
						* ROAD_WHEEL_PATH_WANDER;
				vec2 off_path = abs(from_edge - wander - ROAD_WHEEL_PATHS);
				float wheel_path = 1.0 - smoothstep(ROAD_WHEEL_PATH_WIDTH * 0.25, ROAD_WHEEL_PATH_WIDTH * 0.5, min(off_path.x, off_path.y));
				snow_share = max(snowed * mix(1.0, ROAD_WHEEL_PATH_SNOW, wheel_path), bank * ROAD_BANK_SNOW);
				// grey slush by the edges while it is wet or thawing, and in the wheel paths of a snowed
				// road, the more the more it thaws
				float thaw = max(max(clamp(reloaded_wetness, 0.0, 1.0), clamp(reloaded_snow_wetness, 0.0, 1.0)),
						smoothstep(ROAD_THAW_TEMPERATURE.x, ROAD_THAW_TEMPERATURE.y, reloaded_air_temperature));
				float by_edge = 1.0 - smoothstep(ROAD_SLUSH_REACH * 0.5, ROAD_SLUSH_REACH, from_edge);
				float patches = smoothstep(0.3, 0.7, reloaded_weather_noise(world_position.xz / ROAD_SLUSH_PATCH_SIZE));
				float lying = smoothstep(0.0, ROAD_SLUSH_DEPTH, reloaded_snow_depth);
				slush = lying * max(by_edge * thaw, wheel_path * snowed * mix(ROAD_WHEEL_PATH_CHURN, 1.0, thaw)) * mix(0.5, 1.0, patches);
			} else if (surface == SURFACE_BALLAST) {
				float stones = reloaded_weather_noise(world_position.xz / BALLAST_STONE_SIZE);
				snow_share = smoothstep(BALLAST_SNOW_DEPTH.x, BALLAST_SNOW_DEPTH.y, reloaded_snow_depth)
						* mix(BALLAST_STONE_SNOW, 1.0, stones);
			}
		}
		reloaded_weather_apply_snow_share(surface_albedo, surface_roughness, world_normal, world_position, puddling, snow_share);
		ALBEDO = mix(surface_albedo, RELOADED_SLUSH_ALBEDO, slush);
		ROUGHNESS = mix(surface_roughness, ROAD_SLUSH_ROUGHNESS, slush);
		NORMAL = normalize(mat3(VIEW_MATRIX) * world_normal);
	}
}
