#ifndef H_NOISE
#define H_NOISE

#include <stdint.h>

// OpenSimplex2 noise context
typedef struct {
  int64_t seed;
  // Precomputed permutation table for fast lookups
  uint8_t perm[256];
  uint8_t perm_mod12[256];
} NoiseContext;

// Noise layer configuration
typedef struct {
  float frequency;      // Base frequency (smaller = larger features)
  int octaves;          // Number of noise layers to combine
  float persistence;    // Amplitude multiplier per octave (0.5 = halves each time)
  float lacunarity;     // Frequency multiplier per octave (2.0 = doubles each time)
  float amplitude;      // Overall amplitude/scale of output
  int64_t seed_offset;  // Offset added to seed for this layer

  // Domain warping (optional)
  int warp_enabled;
  float warp_strength;
  int warp_octaves;
} NoiseLayer;

// Initialize noise context with seed
void noise_init(NoiseContext *ctx, int64_t seed);

// Core 2D OpenSimplex2 noise (returns value in [-1, 1])
float noise_2d(NoiseContext *ctx, float x, float y);

// Fractal/octave noise - combines multiple octaves of noise
float noise_fractal(NoiseContext *ctx, NoiseLayer *layer, float x, float y);

// Domain-warped noise - applies distortion for more organic shapes
float noise_warped(NoiseContext *ctx, NoiseLayer *layer, NoiseLayer *warp_layer, float x, float y);

// Utility: normalize noise from [-1,1] to [0,1]
static inline float noise_normalize(float value) {
  return (value + 1.0f) * 0.5f;
}

// Utility: normalize and scale to integer range [0, max]
static inline int noise_to_int(float value, int max) {
  return (int)((noise_normalize(value) * max));
}

#endif
