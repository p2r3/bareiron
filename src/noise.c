#include <math.h>
#include <string.h>
#include "noise.h"

// OpenSimplex2 constants
#define STRETCH_2D (-0.211324865405187f)    // (1/sqrt(2+1)-1)/2
#define SQUISH_2D (0.366025403784439f)      // (sqrt(2+1)-1)/2
#define NORM_2D (1.0f / 47.0f)

// Gradients for 2D noise
static const int8_t gradients_2d[] = {
   5,  2,    2,  5,
  -5,  2,   -2,  5,
   5, -2,    2, -5,
  -5, -2,   -2, -5,
};

// Helper: fast floor for positive and negative values
static inline int fast_floor(float x) {
  int xi = (int)x;
  return x < xi ? xi - 1 : xi;
}

// Contribution function for OpenSimplex2
static float contrib(NoiseContext *ctx, int32_t sb, int32_t tb, float dx, float dy) {
  float attn = 2.0f - dx * dx - dy * dy;
  if (attn <= 0.0f) return 0.0f;

  int perm_index = (ctx->perm[sb & 0xFF] + tb) & 0xFF;
  int grad_index = ctx->perm_mod12[perm_index] << 1;

  float value = gradients_2d[grad_index] * dx + gradients_2d[grad_index + 1] * dy;

  attn *= attn;
  return attn * attn * value;
}

void noise_init(NoiseContext *ctx, int64_t seed) {
  ctx->seed = seed;

  // Initialize permutation table with sequential values
  for (int i = 0; i < 256; i++) {
    ctx->perm[i] = i;
  }

  // Shuffle using seed (simple but effective)
  uint64_t s = (uint64_t)seed;
  for (int i = 255; i > 0; i--) {
    // LCG-style PRNG
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    int j = (int)((s >> 32) % (i + 1));

    uint8_t temp = ctx->perm[i];
    ctx->perm[i] = ctx->perm[j];
    ctx->perm[j] = temp;
  }

  // Precompute modulo 12 for gradient selection
  for (int i = 0; i < 256; i++) {
    ctx->perm_mod12[i] = ctx->perm[i] % 12;
  }
}

float noise_2d(NoiseContext *ctx, float x, float y) {
  // Skew the input space to determine which simplex cell we're in
  float stretch_offset = (x + y) * STRETCH_2D;
  float xs = x + stretch_offset;
  float ys = y + stretch_offset;

  // Floor to get grid coordinates
  int xsb = fast_floor(xs);
  int ysb = fast_floor(ys);

  // Unskew to get position in simplex cell
  float squish_offset = (xsb + ysb) * SQUISH_2D;
  float dx0 = x - (xsb - squish_offset);
  float dy0 = y - (ysb - squish_offset);

  // Sum contributions from corners
  float value = 0.0f;

  // First corner (always included)
  value += contrib(ctx, xsb, ysb, dx0, dy0);

  // Second corner (determined by which simplex we're in)
  float dx1 = dx0 - 1.0f - SQUISH_2D;
  float dy1 = dy0 - 0.0f - SQUISH_2D;
  value += contrib(ctx, xsb + 1, ysb, dx1, dy1);

  float dx2 = dx0 - 0.0f - SQUISH_2D;
  float dy2 = dy0 - 1.0f - SQUISH_2D;
  value += contrib(ctx, xsb, ysb + 1, dx2, dy2);

  // Third corner
  float dx3 = dx0 - 1.0f - 2.0f * SQUISH_2D;
  float dy3 = dy0 - 1.0f - 2.0f * SQUISH_2D;
  value += contrib(ctx, xsb + 1, ysb + 1, dx3, dy3);

  return value * NORM_2D;
}

float noise_fractal(NoiseContext *ctx, NoiseLayer *layer, float x, float y) {
  float result = 0.0f;
  float amplitude = layer->amplitude;
  float frequency = layer->frequency;
  float max_value = 0.0f;  // For normalization

  // Layer multiple octaves
  for (int i = 0; i < layer->octaves; i++) {
    // Sample noise at current frequency
    result += noise_2d(ctx, x * frequency, y * frequency) * amplitude;
    max_value += amplitude;

    // Update for next octave
    amplitude *= layer->persistence;
    frequency *= layer->lacunarity;
  }

  // Normalize to maintain [-1, 1] range
  if (max_value > 0.0f) {
    result /= max_value;
  }

  return result;
}

float noise_warped(NoiseContext *ctx, NoiseLayer *layer, NoiseLayer *warp_layer, float x, float y) {
  if (!layer->warp_enabled || !warp_layer) {
    return noise_fractal(ctx, layer, x, y);
  }

  // Generate warp offsets using the warp layer
  float warp_x = noise_fractal(ctx, warp_layer, x + 1000.0f, y) * layer->warp_strength;
  float warp_y = noise_fractal(ctx, warp_layer, x, y + 1000.0f) * layer->warp_strength;

  // Sample main noise at warped coordinates
  return noise_fractal(ctx, layer, x + warp_x, y + warp_y);
}
