// Film grain over the tonemapped picture - FidelityFX Lens' (ffx_lens.h, FfxLensApplyFilmGrain):
// a noise of the pixel's coordinates and the frame shifted onto a simplex grid, its distance from
// the grid's point shaped into a grain, added most to the mid-tones and least to black and white.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

shader_type canvas_item;
render_mode unshaded;

// How strong the grain is (FfxLensApplyFilmGrain's grainAmount)
uniform float amount : hint_range(0.0, 1.0) = 0.12;
// The grain's size in pixels (grainScale)
uniform float grain_scale : hint_range(0.01, 20.0) = 1.0;

uniform sampler2D screen_texture : hint_screen_texture, filter_nearest;

// The grain changes as often as a film's frames
const float FILM_FRAMES_PER_SECOND = 24.0;
// The grain's shape: higher - finer, harder grains
const float GRAIN_SHAPE = 3.0;
// pcg3d16's multiplier and increment
const uint PCG_MULTIPLIER = 12829u;
const uint PCG_INCREMENT = 47989u;
// The 16 bits pcg3d16 keeps, as a fraction
const float PCG_RANGE = 65536.0;

uvec3 pcg3d16(uvec3 v) {
	v = v * PCG_MULTIPLIER + PCG_INCREMENT;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v >>= 16u;
	return v;
}

// The position's distance from the origin of its cell of the triangle grid
vec2 simplex(vec2 p) {
	const float F2 = (sqrt(3.0) - 1.0) / 2.0;
	const float G2 = (3.0 - sqrt(3.0)) / 6.0;
	float u = (p.x + p.y) * F2;
	vec2 pi = round(p + u);
	float v = (pi.x + pi.y) * G2;
	return p - (pi - v);
}

void fragment() {
	vec3 color = texture(screen_texture, SCREEN_UV).rgb;
	vec2 coord = floor(FRAGCOORD.xy);
	uint seed = uint(TIME * FILM_FRAMES_PER_SECOND);
	vec2 random_fine = vec2(pcg3d16(uvec3(uvec2(coord / (grain_scale / 8.0)), seed)).xy) / PCG_RANGE - 0.5;
	vec2 simplex_p = simplex(coord / grain_scale + random_fine);
	float grain = 1.0 - 2.0 * exp2(-length(simplex_p) * GRAIN_SHAPE);
	// with an HDR output a colour may pass 1.0: no grain there rather than an inverted one
	color += grain * max(min(color, vec3(1.0) - color), vec3(0.0)) * amount;
	COLOR = vec4(color, 1.0);
}
