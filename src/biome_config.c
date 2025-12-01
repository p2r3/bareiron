#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "biome_config.h"
#include "cJSON.h"

// registries.h is generated at build time, so we conditionally include it
#ifdef BLOCKS_COUNT
  #include "registries.h"
#endif

WorldConfig world_config = {0};

// Helper: parse a noise layer from JSON
static int parse_noise_layer(cJSON *json, NoiseLayer *layer) {
  cJSON *item;

  layer->frequency = 0.01f;
  layer->octaves = 1;
  layer->persistence = 0.5f;
  layer->lacunarity = 2.0f;
  layer->amplitude = 1.0f;
  layer->seed_offset = 0;
  layer->warp_enabled = 0;

  if ((item = cJSON_GetObjectItem(json, "frequency")))
    layer->frequency = (float)item->valuedouble;
  if ((item = cJSON_GetObjectItem(json, "octaves")))
    layer->octaves = item->valueint;
  if ((item = cJSON_GetObjectItem(json, "persistence")))
    layer->persistence = (float)item->valuedouble;
  if ((item = cJSON_GetObjectItem(json, "lacunarity")))
    layer->lacunarity = (float)item->valuedouble;
  if ((item = cJSON_GetObjectItem(json, "amplitude")))
    layer->amplitude = (float)item->valuedouble;
  if ((item = cJSON_GetObjectItem(json, "seed_offset")))
    layer->seed_offset = item->valueint;

  // Parse domain warping
  cJSON *warp = cJSON_GetObjectItem(json, "warp");
  if (warp) {
    if ((item = cJSON_GetObjectItem(warp, "enabled")))
      layer->warp_enabled = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(warp, "strength")))
      layer->warp_strength = (float)item->valuedouble;
    if ((item = cJSON_GetObjectItem(warp, "octaves")))
      layer->warp_octaves = item->valueint;
  }

  return 1;
}

// Helper: parse surface layers
static int parse_surface_layers(cJSON *json, BiomeDefinition *biome) {
  if (!cJSON_IsArray(json)) return 0;

  biome->surface_layer_count = 0;
  cJSON *layer;
  cJSON_ArrayForEach(layer, json) {
    if (biome->surface_layer_count >= MAX_SURFACE_LAYERS) break;

    SurfaceLayer *sl = &biome->surface_layers[biome->surface_layer_count];

    cJSON *block = cJSON_GetObjectItem(layer, "block");
    cJSON *depth = cJSON_GetObjectItem(layer, "depth");

    if (block && cJSON_IsString(block)) {
      sl->block_id = get_block_id_by_name(block->valuestring);
      sl->depth = depth ? depth->valueint : 1;
      biome->surface_layer_count++;
    }
  }

  return 1;
}

// Helper: parse features
static int parse_features(cJSON *json, BiomeDefinition *biome) {
  if (!cJSON_IsArray(json)) return 0;

  biome->feature_count = 0;
  cJSON *feature_json;
  cJSON_ArrayForEach(feature_json, json) {
    if (biome->feature_count >= MAX_FEATURES_PER_BIOME) break;

    BiomeFeature *feature = &biome->features[biome->feature_count];
    memset(feature, 0, sizeof(BiomeFeature));

    cJSON *type = cJSON_GetObjectItem(feature_json, "type");
    if (!type || !cJSON_IsString(type)) continue;

    if (strcmp(type->valuestring, "tree") == 0) {
      feature->type = FEATURE_TREE;
    } else if (strcmp(type->valuestring, "scatter") == 0) {
      feature->type = FEATURE_SCATTER;
    } else if (strcmp(type->valuestring, "ore") == 0) {
      feature->type = FEATURE_ORE;
    } else {
      continue;
    }

    cJSON *item;
    if ((item = cJSON_GetObjectItem(feature_json, "chance")))
      feature->chance = (float)item->valuedouble;
    if ((item = cJSON_GetObjectItem(feature_json, "block")) && cJSON_IsString(item))
      feature->block_id = get_block_id_by_name(item->valuestring);

    // Tree properties
    if ((item = cJSON_GetObjectItem(feature_json, "leaves")) && cJSON_IsString(item))
      feature->leaves_id = get_block_id_by_name(item->valuestring);
    if ((item = cJSON_GetObjectItem(feature_json, "min_height")))
      feature->min_height = item->valueint;
    if ((item = cJSON_GetObjectItem(feature_json, "max_height")))
      feature->max_height = item->valueint;

    // Ore properties
    if ((item = cJSON_GetObjectItem(feature_json, "min_y")))
      feature->min_y = item->valueint;
    if ((item = cJSON_GetObjectItem(feature_json, "max_y")))
      feature->max_y = item->valueint;
    if ((item = cJSON_GetObjectItem(feature_json, "vein_size")))
      feature->vein_size = item->valueint;

    biome->feature_count++;
  }

  return 1;
}

int load_world_config(const char *filename) {
  // Read file
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Failed to open config file: %s\n", filename);
    return 0;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = malloc(fsize + 1);
  fread(content, 1, fsize, f);
  fclose(f);
  content[fsize] = 0;

  // Parse JSON
  cJSON *root = cJSON_Parse(content);
  free(content);

  if (!root) {
    fprintf(stderr, "Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
    return 0;
  }

  // Parse world settings
  cJSON *world = cJSON_GetObjectItem(root, "world");
  if (world) {
    cJSON *seed = cJSON_GetObjectItem(world, "seed");
    cJSON *threads = cJSON_GetObjectItem(world, "thread_count");

    if (seed) world_config.seed = seed->valueint;
    if (threads) world_config.thread_count = threads->valueint;
  }

  // Parse noise layers
  cJSON *noise_layers = cJSON_GetObjectItem(root, "noise_layers");
  if (noise_layers) {
    world_config.noise_layer_count = 0;

    cJSON *layer;
    cJSON_ArrayForEach(layer, noise_layers) {
      if (world_config.noise_layer_count >= MAX_NOISE_LAYERS) break;

      cJSON *name = cJSON_GetObjectItem(layer, "name");
      if (!name || !cJSON_IsString(name)) continue;

      int idx = world_config.noise_layer_count;
      strncpy(world_config.noise_layers[idx].name, name->valuestring, 31);
      parse_noise_layer(layer, &world_config.noise_layers[idx].layer);

      // Initialize noise context with seed
      int64_t layer_seed = world_config.seed + world_config.noise_layers[idx].layer.seed_offset;
      noise_init(&world_config.noise_layers[idx].context, layer_seed);

      // Track key layers
      if (strcmp(name->valuestring, "temperature") == 0)
        world_config.temperature_layer_idx = idx;
      else if (strcmp(name->valuestring, "humidity") == 0)
        world_config.humidity_layer_idx = idx;
      else if (strcmp(name->valuestring, "elevation") == 0)
        world_config.elevation_layer_idx = idx;

      world_config.noise_layer_count++;
    }
  }

  // Parse biomes
  cJSON *biomes = cJSON_GetObjectItem(root, "biomes");
  if (biomes && cJSON_IsArray(biomes)) {
    world_config.biome_count = 0;

    cJSON *biome_json;
    cJSON_ArrayForEach(biome_json, biomes) {
      if (world_config.biome_count >= MAX_BIOMES) break;

      BiomeDefinition *biome = &world_config.biomes[world_config.biome_count];
      memset(biome, 0, sizeof(BiomeDefinition));

      biome->id = world_config.biome_count;

      cJSON *item;
      if ((item = cJSON_GetObjectItem(biome_json, "name")) && cJSON_IsString(item))
        strncpy(biome->name, item->valuestring, MAX_BIOME_NAME - 1);

      // Climate conditions
      cJSON *conditions = cJSON_GetObjectItem(biome_json, "conditions");
      if (conditions) {
        cJSON *temp = cJSON_GetObjectItem(conditions, "temperature");
        cJSON *hum = cJSON_GetObjectItem(conditions, "humidity");
        cJSON *elev = cJSON_GetObjectItem(conditions, "elevation");

        if (temp && cJSON_IsArray(temp) && cJSON_GetArraySize(temp) == 2) {
          biome->temp_min = (float)cJSON_GetArrayItem(temp, 0)->valuedouble;
          biome->temp_max = (float)cJSON_GetArrayItem(temp, 1)->valuedouble;
        }
        if (hum && cJSON_IsArray(hum) && cJSON_GetArraySize(hum) == 2) {
          biome->humidity_min = (float)cJSON_GetArrayItem(hum, 0)->valuedouble;
          biome->humidity_max = (float)cJSON_GetArrayItem(hum, 1)->valuedouble;
        }
        if (elev && cJSON_IsArray(elev) && cJSON_GetArraySize(elev) == 2) {
          biome->elevation_min = (float)cJSON_GetArrayItem(elev, 0)->valuedouble;
          biome->elevation_max = (float)cJSON_GetArrayItem(elev, 1)->valuedouble;
        }
      }

      // Terrain
      cJSON *terrain = cJSON_GetObjectItem(biome_json, "terrain");
      if (terrain) {
        if ((item = cJSON_GetObjectItem(terrain, "base_height")))
          biome->base_height = item->valueint;
        if ((item = cJSON_GetObjectItem(terrain, "height_scale")))
          biome->height_scale = (float)item->valuedouble;
        if ((item = cJSON_GetObjectItem(terrain, "height_noise")) && cJSON_IsString(item))
          strncpy(biome->height_noise, item->valuestring, 31);
      }

      // Surface layers
      cJSON *surface = cJSON_GetObjectItem(biome_json, "surface_layers");
      if (surface) parse_surface_layers(surface, biome);

      // Features
      cJSON *features = cJSON_GetObjectItem(biome_json, "features");
      if (features) parse_features(features, biome);

      // Caves
      cJSON *caves = cJSON_GetObjectItem(biome_json, "caves");
      if (caves) {
        if ((item = cJSON_GetObjectItem(caves, "enabled")))
          biome->caves_enabled = cJSON_IsTrue(item);
        if ((item = cJSON_GetObjectItem(caves, "threshold")))
          biome->cave_threshold = (float)item->valuedouble;
      }

      world_config.biome_count++;
    }
  }

  cJSON_Delete(root);

  printf("Loaded world config: %d biomes, %d noise layers\n",
         world_config.biome_count, world_config.noise_layer_count);

  return 1;
}

void free_world_config(void) {
  memset(&world_config, 0, sizeof(WorldConfig));
}

float get_noise_value(const char *name, float x, float z) {
  for (int i = 0; i < world_config.noise_layer_count; i++) {
    if (strcmp(world_config.noise_layers[i].name, name) == 0) {
      return noise_fractal(
        &world_config.noise_layers[i].context,
        &world_config.noise_layers[i].layer,
        x, z
      );
    }
  }
  return 0.0f;
}

BiomeDefinition* get_biome_at(float x, float z) {
  if (world_config.biome_count == 0) return NULL;

  // Get noise values for biome selection
  float temp = noise_normalize(get_noise_value("temperature", x, z));
  float humidity = noise_normalize(get_noise_value("humidity", x, z));
  float elevation = noise_normalize(get_noise_value("elevation", x, z));

  // Find best matching biome
  BiomeDefinition *best_biome = &world_config.biomes[0];
  float best_score = -1000.0f;

  for (int i = 0; i < world_config.biome_count; i++) {
    BiomeDefinition *biome = &world_config.biomes[i];

    // Check if climate values are within biome ranges
    int temp_match = (temp >= biome->temp_min && temp <= biome->temp_max);
    int hum_match = (humidity >= biome->humidity_min && humidity <= biome->humidity_max);
    int elev_match = (elevation >= biome->elevation_min && elevation <= biome->elevation_max);

    if (temp_match && hum_match && elev_match) {
      // Calculate score based on distance from biome center
      float temp_center = (biome->temp_min + biome->temp_max) * 0.5f;
      float hum_center = (biome->humidity_min + biome->humidity_max) * 0.5f;
      float elev_center = (biome->elevation_min + biome->elevation_max) * 0.5f;

      float score = -((temp - temp_center) * (temp - temp_center) +
                      (humidity - hum_center) * (humidity - hum_center) +
                      (elevation - elev_center) * (elevation - elev_center));

      if (score > best_score) {
        best_score = score;
        best_biome = biome;
      }
    }
  }

  return best_biome;
}

// Helper to convert block name to ID
// This will use the generated registries
uint16_t get_block_id_by_name(const char *name) {
  // This is a placeholder - the actual implementation will use
  // the generated block registry from build_registries.js

  // For now, we'll use a simple mapping for common blocks
  // This will be replaced with proper registry lookup

  #ifdef BLOCKS
    // When registries are generated, we'll have BLOCKS array
    for (int i = 0; i < BLOCKS_COUNT; i++) {
      if (strcmp(BLOCKS[i].name, name) == 0) {
        return i;
      }
    }
  #endif

  // Fallback to hardcoded IDs for testing
  // These match the current globals.h definitions
  if (strcmp(name, "air") == 0) return 0;
  if (strcmp(name, "stone") == 0) return 1;
  if (strcmp(name, "grass_block") == 0) return 9;
  if (strcmp(name, "dirt") == 0) return 10;
  if (strcmp(name, "sand") == 0) return 66;
  if (strcmp(name, "oak_log") == 0) return 107;
  if (strcmp(name, "oak_leaves") == 0) return 153;

  fprintf(stderr, "Warning: Unknown block '%s', defaulting to air\n", name);
  return 0; // Default to air
}
