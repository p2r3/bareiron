const fs = require("node:fs/promises");
const path = require("node:path");

// Overrides for block-to-item conversion
const blockToItemOverrides = {
  "grass_block": "dirt",
  "snowy_grass_block": "dirt",
  "stone": "cobblestone",
  "diamond_ore": "diamond",
  "gold_ore": "raw_gold",
  "redstone_ore": "redstone",
  "iron_ore": "raw_iron",
  "coal_ore": "coal",
  "copper_ore": "raw_copper",
  "snow": "snowball",
  "dead_bush": "stick"
};

// Blacklisted block name strings
const blockBlacklist = [
  "spruce_",
  "birch_",
  "jungle_",
  "acacia_",
  "dark_oak_",
  "mangrove_",
  "cherry_",
  "pale_oak_",
  "crimson_",
  "warped_",
  "bamboo_",
  "deepslate",
  "infested_",
  "stained_",
  "_terracotta",
  "_head"
];

// Whitelisted blocks, i.e. guaranteed to be included
const blockWhitelist = [
  "air",
  "water",
  "water_1",
  "water_2",
  "water_3",
  "water_4",
  "water_5",
  "water_6",
  "water_7",
  "lava",
  "lava_2",
  "lava_4",
  "lava_6",
  "snowy_grass_block",
  "mud",
  "moss_carpet",
  "oak_slab",
  "stone_slab",
  "cobblestone_slab",
  "composter",
  "coal_block",
  "copper_ore",
  "copper_block"
];

// Currently, only 4 biome types are supported, excluding "beach"
const biomes = [
  "plains",
  "mangrove_swamp",
  "desert",
  "snowy_plains",
  "beach"
];

// Extract item and block data from registry dump
async function extractItemsAndBlocks () {

  // Block network IDs are defined in their own JSON file
  // The item JSON file doesn't define IDs, we get those from the registries
  const blockSource = JSON.parse(await fs.readFile(`${__dirname}/notchian/generated/reports/blocks.json`, "utf8"));

  // Get registry data for extracting item IDs
  const registriesJSON = JSON.parse(await fs.readFile(`${__dirname}/notchian/generated/reports/registries.json`, "utf8"));
  const itemSource = registriesJSON["minecraft:item"].entries;
  // Retrieve the registry list for blocks too, used later in tags
  const blockRegistrySource = registriesJSON["minecraft:block"].entries;

  // Sort blocks by their network ID
  // Since we're only storing 256 blocks, this prioritizes the "common" ones first
  const sortedBlocks = Object.entries(blockSource);
  sortedBlocks.sort((a, b) => {
    const aState = a[1].states.find(c => c.default);
    if (!aState) return 1;
    const bState = b[1].states.find(c => c.default);
    if (!bState) return -1;
    return aState.id - bState.id;
  });

  // Create name-id pair objects for easier parsing
  const blocks = {}, items = {};

  for (const entry of sortedBlocks) {
    const defaultState = entry[1].states.find(c => c.default);
    if (!defaultState) continue;
    // Check if a part of this block's name is in the blacklist
    let found = false;
    for (const str of blockBlacklist) {
      if (entry[0].includes(str)) {
        found = true;
        break;
      }
    }
    if (found) continue;
    // Register the block ID
    blocks[entry[0].replace("minecraft:", "")] = defaultState.id;
    // Include "snowy" variants of blocks as well
    if ("properties" in defaultState && "snowy" in defaultState.properties) {
      const snowyState = entry[1].states.find(c => c.properties.snowy);
      blocks["snowy_" + entry[0].replace("minecraft:", "")] = snowyState.id;
    }
    // Include levels for fluids
    if ("fluid" in entry[1].definition) {
      for (let i = 1; i <= 7; i ++) {
        blocks[entry[0].replace("minecraft:", "") + "_" + i] = defaultState.id + i;
      }
    }
  }

  for (const item in itemSource) {
    items[item.replace("minecraft:", "")] = itemSource[item].protocol_id;
  }

  /**
   * Create a block network ID palette.
   * The goal is to pack as many meaningful blocks into 256 IDs as
   * possible. For this, we only include blocks that have corresponding
   * items, outside of some exceptions.
   */
  const palette = {};

  // While we're at it, map block IDs to item IDs
  const mapping = [], mappingWithOverrides = [];

  // Handle explicitly whitelisted blocks first
  for (const block of blockWhitelist) {
    palette[block] = blocks[block];
    mapping.push(items[block] || 0);
    mappingWithOverrides.push(items[blockToItemOverrides[block]] || items[block] || 0);
    if (mapping.length === 256) break;
  }

  // Continue adding blocks with matching items
  for (const block in blocks) {
    if (!(block in items)) continue;
    if (blockWhitelist.includes(block)) continue;
    palette[block] = blocks[block];
    mapping.push(items[block] || 0);
    mappingWithOverrides.push(items[blockToItemOverrides[block]] || items[block] || 0);
    if (mapping.length === 256) break;
  }

  // Build list of block IDs, but from the registries
  // Tags refer to these IDs, not the actual blocks
  const blockRegistry = {};
  for (const block in blockRegistrySource) {
    blockRegistry[block.replace("minecraft:", "")] = blockRegistrySource[block].protocol_id;
  }

  return { blocks, items, palette, mapping, mappingWithOverrides, blockRegistry };

}

// Write an integer as a VarInt
function writeVarInt (value) {
  const bytes = [];
  while (true) {
    if ((value & ~0x7F) === 0) {
      bytes.push(value);
      return Buffer.from(bytes);
    }
    bytes.push((value & 0x7F) | 0x80);
    value >>>= 7;
  }
}

// Scan directory recursively to find all JSON files
async function scanDirectory (basePath, currentPath = "") {
  const entries = {};
  const items = await fs.readdir(path.join(basePath, currentPath), { withFileTypes: true });

  for (const item of items) {
    const relativePath = path.join(currentPath, item.name);
    if (item.isDirectory()) {
      const subEntries = await scanDirectory(basePath, relativePath);
      Object.assign(entries, subEntries);
    } else if (item.name.endsWith(".json")) {
      const dirPath = path.dirname(relativePath);
      const registryName = dirPath === "." ? "" : dirPath;
      const entryName = path.basename(item.name, ".json");

      if (!entries[registryName]) {
        entries[registryName] = [];
      }
      entries[registryName].push(entryName);
    }
  }

  return entries;
}

// Serialize a single registry
function serializeRegistry (name, entries) {
  const parts = [];

  // Packet ID for Registry Data
  parts.push(Buffer.from([0x07]));

  // Registry name
  const nameBuf = Buffer.from(name, "utf8");
  parts.push(writeVarInt(nameBuf.length));
  parts.push(nameBuf);

  // Entry count
  parts.push(writeVarInt(entries.length));

  // Serialize entries
  for (const entryName of entries) {
    const entryBuf = Buffer.from(entryName, "utf8");
    parts.push(writeVarInt(entryBuf.length));
    parts.push(entryBuf);
    parts.push(Buffer.from([0x00]));
  }

  // Combine all parts
  const fullData = Buffer.concat(parts);

  // Prepend packet length
  const lengthBuf = writeVarInt(fullData.length);

  return Buffer.concat([lengthBuf, fullData]);
}

// Serialize a tag update
function serializeTags (tags) {
  const parts = [];

  // Packet ID for Update Tags
  parts.push(Buffer.from([0x0D]));

  // Tag type count
  parts.push(writeVarInt(Object.keys(tags).length));

  // Tag registry entry
  for (const type in tags) {

    // Tag registry identifier
    const identifier = Buffer.from(type, "utf8");
    parts.push(writeVarInt(identifier.length));
    parts.push(identifier);

    // Tag count
    parts.push(writeVarInt(Object.keys(tags[type]).length));

    // Write tag data
    for (const tag in tags[type]) {
      // Tag identifier
      const identifier = Buffer.from(tag, "utf8");
      parts.push(writeVarInt(identifier.length));
      parts.push(identifier);
      // Array of IDs
      parts.push(writeVarInt(Object.keys(tags[type][tag]).length));
      for (const id of tags[type][tag]) {
        parts.push(writeVarInt(id));
      }
    }

  }

  // Combine all parts
  const fullData = Buffer.concat(parts);

  // Prepend packet length
  const lengthBuf = writeVarInt(fullData.length);

  return Buffer.concat([lengthBuf, fullData]);
}

function toVarIntBuffer (array) {
  const parts = [];
  for (const num of array) {
    parts.push(writeVarInt(num));
  }
  return Buffer.concat(parts);
}

// Convert to C-style hex byte array string
function toCArray (buffer) {
  const hexBytes = [...buffer].map(b => `0x${b.toString(16).padStart(2, "0")}`);
  const lines = [];
  for (let i = 0; i < hexBytes.length; i += 12) {
    lines.push("  " + hexBytes.slice(i, i + 12).join(", "));
  }
  return lines.join(",\n");
}

const requiredRegistries = [
  "dimension_type",
  "cat_variant",
  "chicken_variant",
  "cow_variant",
  "frog_variant",
  "painting_variant",
  "pig_variant",
  "wolf_sound_variant",
  "wolf_variant",
  "damage_type"
];

async function convert () {

  const inputPath = __dirname + "/notchian/generated/data/minecraft";
  const outputPath = __dirname + "/src/registries.c";
  const headerPath = __dirname + "/include/registries.h";

  const registries = await scanDirectory(inputPath);
  const registryBuffers = [];

  for (const registry of requiredRegistries) {
    if (!(registry in registries)) {
      console.error(`Missing required registry "${registry}"!`);
      return;
    }
    if (registry.endsWith("variant")) {
      // The mob "variants" only require one valid variant to be accepted
      // Send "temperate" if available, otherwise shortest string to save memory
      if (registries[registry].includes("temperate")) {
        registryBuffers.push(serializeRegistry(registry, ["temperate"]));
      } else {
        const shortest = registries[registry].sort((a, b) => a.length - b.length)[0];
        registryBuffers.push(serializeRegistry(registry, [shortest]));
      }
    } else {
      registryBuffers.push(serializeRegistry(registry, registries[registry]));
    }
  }
  // Send biomes separately - only "plains" is actually required
  registryBuffers.push(serializeRegistry("worldgen/biome", biomes));
  const fullRegistryBuffer = Buffer.concat(registryBuffers);

  const itemsAndBlocks = await extractItemsAndBlocks();

  const tagBuffer = serializeTags({
    "fluid": {
      // Water and lava, both flowing and still states
      "water": [ 1, 2 ],
      "lava": [ 3, 4 ]
    },
    "block": {
      "mineable/pickaxe": [
        itemsAndBlocks.blockRegistry["stone"],
        itemsAndBlocks.blockRegistry["stone_slab"],
        itemsAndBlocks.blockRegistry["cobblestone"],
        itemsAndBlocks.blockRegistry["cobblestone_slab"],
        itemsAndBlocks.blockRegistry["sandstone"],
        itemsAndBlocks.blockRegistry["sandstone_slab"],
        itemsAndBlocks.blockRegistry["ice"],
        itemsAndBlocks.blockRegistry["diamond_ore"],
        itemsAndBlocks.blockRegistry["gold_ore"],
        itemsAndBlocks.blockRegistry["redstone_ore"],
        itemsAndBlocks.blockRegistry["iron_ore"],
        itemsAndBlocks.blockRegistry["coal_ore"],
        itemsAndBlocks.blockRegistry["copper_ore"],
        itemsAndBlocks.blockRegistry["furnace"],
        itemsAndBlocks.blockRegistry["iron_block"],
        itemsAndBlocks.blockRegistry["gold_block"],
        itemsAndBlocks.blockRegistry["diamond_block"],
        itemsAndBlocks.blockRegistry["redstone_block"],
        itemsAndBlocks.blockRegistry["coal_block"],
        itemsAndBlocks.blockRegistry["copper_block"]
      ],
      "mineable/axe": [
        itemsAndBlocks.blockRegistry["oak_log"],
        itemsAndBlocks.blockRegistry["oak_planks"],
        itemsAndBlocks.blockRegistry["oak_wood"],
        itemsAndBlocks.blockRegistry["oak_slab"],
        itemsAndBlocks.blockRegistry["crafting_table"],
        itemsAndBlocks.blockRegistry["chest"]
      ],
      "mineable/shovel": [
        itemsAndBlocks.blockRegistry["grass_block"],
        itemsAndBlocks.blockRegistry["dirt"],
        itemsAndBlocks.blockRegistry["sand"],
        itemsAndBlocks.blockRegistry["snow"],
        itemsAndBlocks.blockRegistry["snow_block"],
        itemsAndBlocks.blockRegistry["mud"]
      ],
      "leaves": [
        itemsAndBlocks.blockRegistry["oak_leaves"]
      ]
    },
    "item": {
      "planks": [
        itemsAndBlocks.items["oak_planks"]
      ]
    }
  });

  const networkBlockPalette = toVarIntBuffer(Object.values(itemsAndBlocks.palette));

  const sourceCode = `\
#include <stdint.h>
#include "registries.h"

// Binary contents of required "Registry Data" packets
uint8_t registries_bin[] = {
${toCArray(fullRegistryBuffer)}
};
// Binary contents of "Update Tags" packets
uint8_t tags_bin[] = {
${toCArray(tagBuffer)}
};

// Block palette
uint16_t block_palette[] = { ${Object.values(itemsAndBlocks.palette).join(", ")} };
// Block palette as VarInt buffer
uint8_t network_block_palette[] = {
${toCArray(networkBlockPalette)}
};

// Block-to-item mapping
uint16_t B_to_I[] = { ${itemsAndBlocks.mappingWithOverrides.join(", ")} };
// Item-to-block mapping
uint8_t I_to_B (uint16_t item) {
  switch (item) {
    ${itemsAndBlocks.mapping.map((c, i) => c ? `case ${c}: return ${i};\n    ` : "").join("")}
    default: break;
  }
  return 0;
}
`;

  const headerCode = `\
#ifndef H_REGISTRIES
#define H_REGISTRIES

#include <stdint.h>

// Binary packet data (${fullRegistryBuffer.length + tagBuffer.length} bytes total)
extern uint8_t registries_bin[${fullRegistryBuffer.length}];
extern uint8_t tags_bin[${tagBuffer.length}];

extern uint16_t block_palette[256]; // Block palette
extern uint8_t network_block_palette[${networkBlockPalette.length}]; // Block palette as VarInt buffer
extern uint16_t B_to_I[256]; // Block-to-item mapping
uint8_t I_to_B (uint16_t item); // Item-to-block mapping

// Block identifiers
${Object.keys(itemsAndBlocks.palette).map((c, i) => `#define B_${c} ${i}`).join("\n")}

// Item identifiers
${Object.entries(itemsAndBlocks.items).map(c => `#define I_${c[0]} ${c[1]}`).join("\n")}

// Biome identifiers
${biomes.map((c, i) => `#define W_${c} ${i}`).join("\n")}

// Damage type identifiers
${registries["damage_type"].map((c, i) => `#define D_${c} ${i}`).join("\n")}

#endif
`;

  await fs.writeFile(outputPath, sourceCode);
  await fs.writeFile(headerPath, headerCode);
  console.log("Done. Wrote to `registries.c` and `registries.h`");

}

convert().catch(console.error);
