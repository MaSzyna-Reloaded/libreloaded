// The weather on a material's surface - the ground's and whatever stands on it: snow lying on what
// faces up, grey slush when it is soaked, the wet darkening and smoothing everything. For the
// spatial shaders of the scenery's materials:
//
//   #include "res://addons/libreloaded/environment/ground_weather.gdshaderinc"
//   varying vec3 world_position;
//   void vertex() {
//       // not INV_VIEW_MATRIX * VERTEX in fragment(): a double precision build keeps the view's
//       // origin there its own way, kilometres off far from the world's origin
//       world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
//   }
//   void fragment() {
//       vec3 albedo = ...;
//       float roughness = ...;
//       vec3 world_normal = mat3(INV_VIEW_MATRIX) * NORMAL;
//       reloaded_weather_surface(albedo, roughness, world_normal, world_position);
//       ALBEDO = albedo;
//       ROUGHNESS = roughness;
//   }
//
// The ground's state is the environment's, in these globals, whichever weather the game draws sets
// them (gnd-weather's WeatherServer with gnd_weather/ground_globals_prefix "reloaded_"). A game
// whose weather does more serves a ground_weather.gdshaderinc of its own under this path (a
// ResourceFormatLoader in front of the engine's), with the functions below.
//
// Where the weather's probes reach (reloaded_weather_*: gnd-weather's WeatherNode weather_probe_*),
// the snow, the wet and their warmth are the ground's there: less under a roof, none where a heat
// source warms it above freezing; without them, the open ground's everywhere.

global uniform float reloaded_wetness;
global uniform float reloaded_air_temperature;
// how deep the snow lies on the average, in metres
global uniform float reloaded_snow_depth;
// how wet the lying snow is (0 - 1): soaked by a rain or a thaw, grey slush
global uniform float reloaded_snow_wetness;

// the weather probes' (gnd-weather's WeatherProbes): the open ground's share of the snow (r), the wet
// (g) and the standing water (b) at each, and its temperature (a, °C) - addressed round in the
// texture - their window's corner and size in the world, and whether there are any
global uniform sampler3D reloaded_weather_ground : filter_linear, repeat_enable;
global uniform vec3 reloaded_weather_probe_origin;
global uniform vec3 reloaded_weather_probe_size;
global uniform float reloaded_weather_probes_enabled;
// which of the weather there is on the surfaces - the wet, the snow lying - and whether any of it
// lies there (gnd-weather's WeatherServer.WET_EXISTS_GLOBAL and the rest, told once a change by
// whoever draws the weather): without one nothing of it is worked out, without any a surface stays
// its own
global uniform bool reloaded_weather_wet_exists;
global uniform bool reloaded_weather_snow_exists;
global uniform bool reloaded_weather_surfaces_active;

// How far world_position is in the probes' window: 1 well in, down to 0 towards its sides, 0 out of
// it or without probes - for blending what the probes tell into the open ground's
float reloaded_weather_probe_blend(vec3 world_position) {
	vec3 in_window = (world_position - reloaded_weather_probe_origin) / max(reloaded_weather_probe_size, vec3(0.001));
	if (reloaded_weather_probes_enabled < 0.5 || any(lessThan(in_window, vec3(0.0))) || any(greaterThan(in_window, vec3(1.0)))) {
		return 0.0;
	}
	vec3 from_side = min(in_window, 1.0 - in_window);
	return smoothstep(0.0, 0.08, min(from_side.x, from_side.z)) * smoothstep(0.0, 0.08, from_side.y);
}

// The weather at world_position from the probes, blended between them and into the open ground's
// towards their window's sides: the open ground's share of its snow, its wet and its standing water,
// and the temperature - 1, 1, 1 and the air's out of them or without them
vec4 reloaded_weather_local(vec3 world_position) {
	vec4 open_ground = vec4(1.0, 1.0, 1.0, reloaded_air_temperature);
	float inside = reloaded_weather_probe_blend(world_position);
	if (inside <= 0.0) {
		return open_ground;
	}
	return mix(open_ground, texture(reloaded_weather_ground, world_position / reloaded_weather_probe_size), inside);
}

// How much of the snow a surface as warm as temperature (°C) - surface_heat warmer than that of its
// own, a material's warm place - keeps of the open ground's: none where it is above freezing while the
// air is not, the edge soft over a degree; the open ground's own thaw is its depth's
float reloaded_weather_snow_kept(float temperature, float surface_heat) {
	float warmth = max(temperature + surface_heat, 0.0) - max(reloaded_air_temperature, 0.0);
	return 1.0 - smoothstep(0.0, 1.0, warmth);
}

// the depth at which the snow covers the ground (WeatherServer.SNOW_COVER_DEPTH)
const float RELOADED_SNOW_COVER_DEPTH = 0.03;

const vec3 RELOADED_SNOW_ALBEDO = vec3(0.86, 0.88, 0.92);
const vec3 RELOADED_SLUSH_ALBEDO = vec3(0.5, 0.53, 0.55);

// How near a pixel is for the wet's reflections to show (1 - 0), by how wide it is in the
// world (length(fwidth(world_position)), fragment() only): some 30 m off at 1080p for a surface facing
// the eye, gone by some 55 m, the sooner for the ground seen aslant. Beyond, the wet is as rough (0.7)
// as the screen space reflections skip: the wet scenery reflected everywhere cost them every
// pixel of the screen
float reloaded_weather_reflections_near(float pixel_size) {
	return 1.0 - smoothstep(0.03, 0.06, pixel_size);
}

float reloaded_weather_hash(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

// value noise, 0 - 1
float reloaded_weather_noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(
		mix(reloaded_weather_hash(i), reloaded_weather_hash(i + vec2(1.0, 0.0)), u.x),
		mix(reloaded_weather_hash(i + vec2(0.0, 1.0)), reloaded_weather_hash(i + vec2(1.0, 1.0)), u.x),
		u.y
	);
}

// How much deeper than the average the snow lies at a point of the ground (0 - 2 or so): in heaps a
// metre or two across
float reloaded_weather_snow_heaping(vec2 p) {
	float heaps = reloaded_weather_noise(p * 0.8) * 0.6 + reloaded_weather_noise(p * 2.1 + 5.3) * 0.4;
	return mix(0.65, 1.35, heaps) * mix(0.75, 1.25, reloaded_weather_noise(p * 0.11 + 11.7));
}

// How much of a surface facing up as much the snow of `cover` (0 - 1) covers: the first snow dusts
// everything evenly, the ground showing through, and lies thicker in patches that grow and run
// together into a full cover - thin, it was a few white blots on bare ground
float reloaded_weather_snow_patches(vec2 world_xz, float cover) {
	// three octaves, turned against each other so the value noise's grid does not show
	vec2 p = world_xz * 0.45;
	float patches = reloaded_weather_noise(p) * 0.55;
	p = mat2(vec2(0.8, 0.6), vec2(-0.6, 0.8)) * p * 2.03 + 17.1;
	patches += reloaded_weather_noise(p) * 0.3;
	p = mat2(vec2(0.8, 0.6), vec2(-0.6, 0.8)) * p * 2.01 + 3.7;
	patches += reloaded_weather_noise(p) * 0.15;
	float dusting = smoothstep(0.0, 0.35, cover) * mix(0.45, 0.65, patches);
	float lying = smoothstep(1.0 - cover * 1.4, 1.0 - cover * 1.4 + 0.35, patches);
	return max(dusting, lying);
}

// reloaded_weather_snow() with the weather there given (reloaded_weather_local()'s)
float reloaded_weather_snow_at(vec3 world_normal, vec3 world_position, vec4 local) {
	float facing_up = smoothstep(0.25, 0.75, normalize(world_normal).y);
	float depth = reloaded_snow_depth * local.r * reloaded_weather_snow_kept(local.a, 0.0);
	float cover = clamp(depth * reloaded_weather_snow_heaping(world_position.xz) / RELOADED_SNOW_COVER_DEPTH, 0.0, 1.0);
	if (facing_up <= 0.0 || cover <= 0.001) {
		return 0.0;
	}
	return facing_up * reloaded_weather_snow_patches(world_position.xz, cover);
}

// How much snow lies on a surface (0 - 1): on what faces up, in patches while the cover is thin, as
// much of it as lies there (reloaded_weather_local())
float reloaded_weather_snow(vec3 world_normal, vec3 world_position) {
	if (!reloaded_weather_snow_exists) {
		return 0.0;
	}
	float facing_up = smoothstep(0.25, 0.75, normalize(world_normal).y);
	if (facing_up <= 0.0) {
		return 0.0;
	}
	return reloaded_weather_snow_at(world_normal, world_position, reloaded_weather_local(world_position));
}

// The deepest a surface is raised by the snow, in metres, however deep it lies (#define it before
// including this to change it): a deeper snow would bury what the game does not bury - rails,
// a platform's edge
#ifndef RELOADED_SNOW_MAX_DEPTH
#define RELOADED_SNOW_MAX_DEPTH 1.0
#endif

// reloaded_weather_snow_depth() with the weather there given (reloaded_weather_local()'s)
float reloaded_weather_snow_depth_at(vec3 world_normal, vec3 world_position, vec4 local) {
	float facing_up = smoothstep(0.5, 0.9, normalize(world_normal).y);
	float depth = reloaded_snow_depth * local.r * reloaded_weather_snow_kept(local.a, 0.0) * reloaded_weather_snow_heaping(world_position.xz);
	return facing_up * min(depth, RELOADED_SNOW_MAX_DEPTH);
}

// How deep the snow lies at a point of a surface facing up, in metres: as deep as it lies on the
// average, heaped up here. For vertex(), to raise the surface by:
//   vec3 world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
//   vec3 world_normal = normalize(mat3(MODEL_MATRIX) * NORMAL);
//   float depth = reloaded_weather_snow_depth(world_normal, world_position);
//   VERTEX += inverse(mat3(MODEL_MATRIX)) * vec3(0.0, depth, 0.0);
// It moves vertices, so it shows on a dense mesh (a terrain) and not on a box of eight corners.
float reloaded_weather_snow_depth(vec3 world_normal, vec3 world_position) {
	if (!reloaded_weather_snow_exists) {
		return 0.0;
	}
	return reloaded_weather_snow_depth_at(world_normal, world_position, reloaded_weather_local(world_position));
}

// The surface's normal turned by the heaps the snow lies in (world space), where there is snow
vec3 reloaded_weather_snow_normal(vec3 world_normal, vec3 world_position) {
	if (!reloaded_weather_snow_exists) {
		return world_normal;
	}
	float e = 0.15;
	// the probes' once: they stand metres apart, the samples a span
	vec4 local = reloaded_weather_local(world_position);
	float here = reloaded_weather_snow_depth_at(world_normal, world_position, local);
	if (here <= 0.0001) {
		return world_normal;
	}
	float dx = reloaded_weather_snow_depth_at(world_normal, world_position + vec3(e, 0.0, 0.0), local) - here;
	float dz = reloaded_weather_snow_depth_at(world_normal, world_position + vec3(0.0, 0.0, e), local) - here;
	return normalize(world_normal - vec3(dx, 0.0, dz) / e);
}

// The weather on a surface: the wet and the snow on it. puddling - how much water the surface would
// hold standing (a paving 1, a soil 0.3, a ballast 0), for a weather that draws it
void reloaded_weather_surface_puddled(inout vec3 surface_albedo, inout float surface_roughness, vec3 world_normal, vec3 world_position, float puddling) {
	if (!reloaded_weather_surfaces_active) {
		return;
	}
	// before any branch on the pixel's own: the derivatives are good only in uniform control flow
	float reflections_near = reloaded_weather_reflections_near(length(fwidth(world_position)));
	vec4 local = reloaded_weather_local(world_position);
	// wet: darker (the water fills the pores) and glossier
	if (reloaded_weather_wet_exists) {
		float wet = clamp(reloaded_wetness * local.g, 0.0, 1.0);
		surface_albedo *= mix(1.0, 0.62, wet);
		surface_roughness = mix(surface_roughness, min(surface_roughness, mix(0.7, 0.18, reflections_near)), wet * 0.85);
	}
	// soaked, it greys to a glistening slush
	float snow = reloaded_weather_snow_exists ? reloaded_weather_snow_at(world_normal, world_position, local) : 0.0;
	float slush = clamp(reloaded_snow_wetness * 0.8, 0.0, 1.0);
	surface_albedo = mix(surface_albedo, mix(RELOADED_SNOW_ALBEDO, RELOADED_SLUSH_ALBEDO, slush), snow);
	surface_roughness = mix(surface_roughness, mix(0.75, 0.3, slush), snow);
}

// Everything the weather does to a surface, each part worked out once and only where it is - for a
// material taking its colour, roughness and normal from it (fragment() only): the wet, the snow
// lying with its heaps in the light as near as a pixel is small enough to show them. world_normal -
// the surface's own (world space) in, the weather's out; returns how far the snow covers the surface,
// for its own bumps (its normal map) to go under it. puddling - as for
// reloaded_weather_surface_puddled(). snow_share - how much of the snow on the open ground lies on
// the surface: 0 none (a road cleared of it), 1 as much, more where it was heaped up (the banks a
// plough leaves); surface_heat - how much warmer than where it stands the surface is of its own (°C,
// a material's warm place: a chimney, a boiler room's wall, an engine's cover). The snow and the
// wet are as much as lies where it is (reloaded_weather_local())
float reloaded_weather_apply_heated(inout vec3 surface_albedo, inout float surface_roughness, inout vec3 world_normal,
		vec3 world_position, float puddling, float snow_share, float surface_heat) {
	// a branch on the globals alone: the control flow stays uniform for the derivatives below
	if (!reloaded_weather_surfaces_active) {
		return 0.0;
	}
	vec3 normal = normalize(world_normal);
	// before any branch on the pixel's own: the derivatives are good only in uniform control flow
	float footprint = length(fwidth(world_position.xz));
	float reflections_near = reloaded_weather_reflections_near(length(fwidth(world_position)));
	vec3 weather_normal = normal;
	vec4 local = reloaded_weather_local(world_position);
	snow_share *= local.r * reloaded_weather_snow_kept(local.a, surface_heat);

	if (reloaded_weather_wet_exists) {
		float wet = clamp(reloaded_wetness * local.g, 0.0, 1.0);
		surface_albedo *= mix(1.0, 0.62, wet);
		surface_roughness = mix(surface_roughness, min(surface_roughness, mix(0.7, 0.18, reflections_near)), wet * 0.85);
	}

	float snow = 0.0;
	float facing_up = smoothstep(0.25, 0.75, normal.y);
	if (reloaded_weather_snow_exists && snow_share > 0.0 && facing_up > 0.0) {
		float depth = reloaded_snow_depth * snow_share;
		float heaping = reloaded_weather_snow_heaping(world_position.xz);
		float cover = clamp(depth * heaping / RELOADED_SNOW_COVER_DEPTH, 0.0, 1.0);
		if (cover > 0.001) {
			snow = facing_up * reloaded_weather_snow_patches(world_position.xz, cover);
		}
		if (snow > 0.001) {
			float slush = clamp(reloaded_snow_wetness * 0.8, 0.0, 1.0);
			surface_albedo = mix(surface_albedo, mix(RELOADED_SNOW_ALBEDO, RELOADED_SLUSH_ALBEDO, slush), snow);
			surface_roughness = mix(surface_roughness, mix(0.75, 0.3, slush), snow);
			// the heaps in the light, near enough to show
			float near = 1.0 - smoothstep(0.1, 0.4, footprint);
			if (near > 0.0) {
				float e = 0.15;
				float heap_depth = smoothstep(0.5, 0.9, normal.y) * min(depth, RELOADED_SNOW_MAX_DEPTH);
				float dx = (reloaded_weather_snow_heaping(world_position.xz + vec2(e, 0.0)) - heaping) * heap_depth;
				float dz = (reloaded_weather_snow_heaping(world_position.xz + vec2(0.0, e)) - heaping) * heap_depth;
				vec3 snow_normal = normalize(normal - vec3(dx, 0.0, dz) * (near / e));
				weather_normal = normalize(mix(weather_normal, snow_normal, snow));
			}
		}
	}
	world_normal = weather_normal;
	return snow;
}

// reloaded_weather_apply_heated() with the surface as warm as where it stands
float reloaded_weather_apply_snow_share(inout vec3 surface_albedo, inout float surface_roughness, inout vec3 world_normal,
		vec3 world_position, float puddling, float snow_share) {
	return reloaded_weather_apply_heated(surface_albedo, surface_roughness, world_normal, world_position, puddling, snow_share, 0.0);
}

// reloaded_weather_apply_snow_share() with as much snow as on the open ground
float reloaded_weather_apply(inout vec3 surface_albedo, inout float surface_roughness, inout vec3 world_normal,
		vec3 world_position, float puddling) {
	return reloaded_weather_apply_snow_share(surface_albedo, surface_roughness, world_normal, world_position, puddling, 1.0);
}

void reloaded_weather_surface(inout vec3 surface_albedo, inout float surface_roughness, vec3 world_normal, vec3 world_position) {
	reloaded_weather_surface_puddled(surface_albedo, surface_roughness, world_normal, world_position, 0.5);
}
