#ifndef H_BIOME_CONFIG
#define H_BIOME_CONFIG

#include <stdint.h>
#include "noise.h"

#define MAX_BIOMES 64
#define MAX_NOISE_LAYERS 16
#define MAX_SURFACE_LAYERS 8
#define MAX_FEATURES_PER_BIOME 16
#define MAX_BIOME_NAME 32

// Feature types
typedef enum {
  FEATURE_NONE = 0,
  FEATURE_TREE,
  FEATURE_SCATTER,    // Random block placement (flowers, grass, etc.)
  FEATURE_ORE,        // Ore veins
  FEATURE_STRUCTURE   // More complex structures
} FeatureType;

// Surface layer configuration
typedef struct {
  uint16_t block_id;  // Block type for this layer
  int depth;          // Depth of layer (-1 = fill to bottom)
} SurfaceLayer;

// Feature configuration
typedef struct {
  FeatureType type;
  char name[32];

  // Common properties
  float chance;       // Probability of placement (0.0 - 1.0)
  uint16_t block_id;  // Primary block type

  // Tree-specific
  uint16_t leaves_id;
  int min_height;
  int max_height;

  // Scatter-specific
  int replace_air_only;

  // Ore-specific
  int min_y;
  int max_y;
  int vein_size;
} BiomeFeature;

// Biome definition
typedef struct {
  char name[MAX_BIOME_NAME];
  int id;

  // Climate conditions for biome selection
  float temp_min, temp_max;       // Temperature range [0,1]
  float humidity_min, humidity_max; // Humidity range [0,1]
  float elevation_min, elevation_max; // Elevation range [0,1]

  // Terrain generation
  int base_height;                // Base Y level for terrain
  float height_scale;             // Multiplier for height variation
  char height_noise[32];          // Name of noise layer to use

  // Surface composition
  int surface_layer_count;
  SurfaceLayer surface_layers[MAX_SURFACE_LAYERS];

  // Features (trees, ores, etc.)
  int feature_count;
  BiomeFeature features[MAX_FEATURES_PER_BIOME];

  // Caves
  int caves_enabled;
  float cave_threshold;
} BiomeDefinition;

// Global world configuration
typedef struct {
  int64_t seed;
  int thread_count;               // -1 = auto-detect

  // Noise layers (named for reference in biomes)
  int noise_layer_count;
  struct {
    char name[32];
    NoiseLayer layer;
    NoiseContext context;
  } noise_layers[MAX_NOISE_LAYERS];

  // Biomes
  int biome_count;
  BiomeDefinition biomes[MAX_BIOMES];

  // References to key noise layers (indices)
  int temperature_layer_idx;
  int humidity_layer_idx;
  int elevation_layer_idx;
} WorldConfig;

// Global world config instance
extern WorldConfig world_config;

// Load configuration from JSON file
int load_world_config(const char *filename);

// Free any allocated resources
void free_world_config(void);

// Get biome at world coordinates based on noise values
BiomeDefinition* get_biome_at(float x, float z);

// Get noise value by name
float get_noise_value(const char *name, float x, float z);

// Helper: get block ID by name from registries
uint16_t get_block_id_by_name(const char *name);

#endif
