#include <string.h>
#include <stdio.h>

#include "globals.h"
#include "registries.h"
#include "tools.h"
#include "crafting.h"

void getCraftingOutput (PlayerData *player, uint8_t *count, uint16_t *item) {

  uint8_t i, filled = 0, first = 10, identical = true;
  for (i = 0; i < 9; i ++) {
    if (player->craft_items[i]) {
      filled ++;
      if (first == 10) first = i;
      else if (player->craft_items[i] != player->craft_items[first]) {
        identical = false;
      }
    }
  }

  if (filled == 0) {
    *item = 0;
    *count = 0;
    return;
  }

  uint16_t first_item = player->craft_items[first];
  uint8_t first_col = first % 3, first_row = first / 3;

  switch (filled) {

    case 0:
      *item = 0;
      *count = 0;
      return;

    case 1:
      switch (first_item) {
        case I_oak_log: *item = I_oak_planks; *count = 4; return;
        case I_oak_planks: *item = I_oak_button; *count = 1; return;
        case I_iron_block: *item = I_iron_ingot; *count = 9; return;
        case I_gold_block: *item = I_gold_ingot; *count = 9; return;
        case I_diamond_block: *item = I_diamond; *count = 9; return;
        case I_redstone_block: *item = I_redstone; *count = 9; return;
        case I_coal_block: *item = I_coal; *count = 9; return;
        case I_copper_block: *item = I_copper_ingot; *count = 9; return;

        default: break;
      }
      break;

    case 2:
      switch (first_item) {
        case I_oak_planks:
          if (first_col != 2 && player->craft_items[first + 1] == I_oak_planks) {
            *item = I_oak_pressure_plate;
            *count = 1;
            return;
          } else if (first_row != 2 && player->craft_items[first + 3] == I_oak_planks) {
            *item = I_stick;
            *count = 4;
            return;
          }
          break;
        case I_charcoal:
        case I_coal:
          if (first_row != 2 && player->craft_items[first + 3] == I_stick) {
            *item = I_torch;
            *count = 4;
            return;
          }
          break;
        case I_iron_ingot:
          if (
            (
              first_row != 2 && first_col != 2 &&
              player->craft_items[first + 4] == I_iron_ingot
            ) || (
              first_row != 2 && first_col != 0 &&
              player->craft_items[first + 2] == I_iron_ingot
            )
          ) {
            *item = I_shears;
            *count = 1;
            return;
          }
          break;

        default: break;
      }
      break;

    case 3:
      switch (first_item) {
        case I_oak_planks:
        case I_cobblestone:
        case I_stone:
        case I_snow_block:
          // Slab recipes
          if (
            first_col == 0 &&
            player->craft_items[first + 1] == first_item &&
            player->craft_items[first + 2] == first_item
          ) {
            if (first_item == I_oak_planks) *item = I_oak_slab;
            else if (first_item == I_cobblestone) *item = I_cobblestone_slab;
            else if (first_item == I_stone) *item = I_stone_slab;
            else if (first_item == I_snow_block) *item = I_snow;
            *count = 6;
            return;
          }
        case I_iron_ingot:
        case I_gold_ingot:
        case I_diamond:
        case I_netherite_ingot:
          // Shovel recipes
          if (
            first_row == 0 &&
            player->craft_items[first + 3] == I_stick &&
            player->craft_items[first + 6] == I_stick
          ) {
            if (first_item == I_oak_planks) *item = I_wooden_shovel;
            else if (first_item == I_cobblestone) *item = I_stone_shovel;
            else if (first_item == I_iron_ingot) *item = I_iron_shovel;
            else if (first_item == I_gold_ingot) *item = I_golden_shovel;
            else if (first_item == I_diamond) *item = I_diamond_shovel;
            else if (first_item == I_netherite_ingot) *item = I_netherite_shovel;
            *count = 1;
            return;
          }
          // Sword recipes
          if (
            first_row == 0 &&
            player->craft_items[first + 3] == first_item &&
            player->craft_items[first + 6] == I_stick
          ) {
            if (first_item == I_oak_planks) *item = I_wooden_sword;
            else if (first_item == I_cobblestone) *item = I_stone_sword;
            else if (first_item == I_iron_ingot) *item = I_iron_sword;
            else if (first_item == I_gold_ingot) *item = I_golden_sword;
            else if (first_item == I_diamond) *item = I_diamond_sword;
            else if (first_item == I_netherite_ingot) *item = I_netherite_sword;
            *count = 1;
            return;
          }
          break;

        default: break;
      }
      break;

    case 4:
      switch (first_item) {
        case I_oak_planks:
        case I_oak_log:
        case I_snowball:
          // Uniform 2x2 shaped recipes
          if (
            first_col != 2 && first_row != 2 &&
            player->craft_items[first + 1] == first_item &&
            player->craft_items[first + 3] == first_item &&
            player->craft_items[first + 4] == first_item
          ) {
            if (first_item == I_oak_planks) { *item = I_crafting_table; *count = 1; }
            else if (first_item == I_oak_log) { *item = I_oak_wood; *count = 3; }
            else if (first_item == I_snowball) { *item = I_snow_block; *count = 3; }
            return;
          }
          break;
        // Boot recipes
        case I_leather:
        case I_iron_ingot:
        case I_gold_ingot:
        case I_diamond:
        case I_netherite_ingot:
          if (
            first_col == 0 && first_row < 2 &&
            player->craft_items[first + 2] == first_item &&
            player->craft_items[first + 3] == first_item &&
            player->craft_items[first + 5] == first_item
          ) {
            if (first_item == I_leather) *item = I_leather_boots;
            else if (first_item == I_iron_ingot) *item = I_iron_boots;
            else if (first_item == I_gold_ingot) *item = I_golden_boots;
            else if (first_item == I_diamond) *item = I_diamond_boots;
            else if (first_item == I_netherite_ingot) *item = I_netherite_boots;
            *count = 1;
            return;
          }
          break;

        default: break;
      }
      break;

    case 5:
    switch (first_item) {
        case I_oak_planks:
        case I_cobblestone:
        case I_iron_ingot:
        case I_gold_ingot:
        case I_diamond:
        case I_netherite_ingot:
          // Pickaxe recipes
          if (
            first == 0 &&
            player->craft_items[first + 1] == first_item &&
            player->craft_items[first + 2] == first_item &&
            player->craft_items[first + 4] == I_stick &&
            player->craft_items[first + 7] == I_stick
          ) {
            if (first_item == I_oak_planks) *item = I_wooden_pickaxe;
            else if (first_item == I_cobblestone) *item = I_stone_pickaxe;
            else if (first_item == I_iron_ingot) *item = I_iron_pickaxe;
            else if (first_item == I_gold_ingot) *item = I_golden_pickaxe;
            else if (first_item == I_diamond) *item = I_diamond_pickaxe;
            else if (first_item == I_netherite_ingot) *item = I_netherite_pickaxe;
            *count = 1;
            return;
          }
          // Axe recipes
          if (
            first < 2 &&
            player->craft_items[first + 1] == first_item &&
            ((
              player->craft_items[first + 3] == first_item &&
              player->craft_items[first + 4] == I_stick &&
              player->craft_items[first + 7] == I_stick
            ) || (
              player->craft_items[first + 4] == first_item &&
              player->craft_items[first + 3] == I_stick &&
              player->craft_items[first + 6] == I_stick
            ))
          ) {
            if (first_item == I_oak_planks) *item = I_wooden_axe;
            else if (first_item == I_cobblestone) *item = I_stone_axe;
            else if (first_item == I_iron_ingot) *item = I_iron_axe;
            else if (first_item == I_gold_ingot) *item = I_golden_axe;
            else if (first_item == I_diamond) *item = I_diamond_axe;
            else if (first_item == I_netherite_ingot) *item = I_netherite_axe;
            *count = 1;
            return;
          }
          if (
            first_item == I_oak_planks ||
            first_item == I_cobblestone
          ) break;
        case I_leather:
          // Helmet recipes
          if (
            first_col == 0 && first_row < 2 &&
            player->craft_items[first + 1] == first_item &&
            player->craft_items[first + 2] == first_item &&
            player->craft_items[first + 3] == first_item &&
            player->craft_items[first + 5] == first_item
          ) {
            if (first_item == I_leather) *item = I_leather_helmet;
            else if (first_item == I_iron_ingot) *item = I_iron_helmet;
            else if (first_item == I_gold_ingot) *item = I_golden_helmet;
            else if (first_item == I_diamond) *item = I_diamond_helmet;
            else if (first_item == I_netherite_ingot) *item = I_netherite_helmet;
            *count = 1;
            return;
          }
          break;

        default: break;
      }
      break;

    case 7:
      // Legging recipes
      if (identical && player->craft_items[4] == 0 && player->craft_items[7] == 0) {
        switch (first_item) {
          case I_leather: *item = I_leather_leggings; *count = 1; return;
          case I_iron_ingot: *item = I_iron_leggings; *count = 1; return;
          case I_gold_ingot: *item = I_golden_leggings; *count = 1; return;
          case I_diamond: *item = I_diamond_leggings; *count = 1; return;
          case I_netherite_ingot: *item = I_netherite_leggings; *count = 1; return;
          default: break;
        }
      }
      switch (first_item) {
        case I_oak_slab:
          if (
            identical &&
            player->craft_items[1] == 0 &&
            player->craft_items[4] == 0
          ) {
            *item = I_composter;
            *count = 1;
            return;
          }
          break;

        default: break;
      }
      break;

    case 8:
      if (identical) {
        if (player->craft_items[4] == 0) {
          // Center slot clear
          switch (first_item) {
            case I_cobblestone: *item = I_furnace; *count = 1; return;
            #ifdef ALLOW_CHESTS
            case I_oak_planks: *item = I_chest; *count = 1; return;
            #endif
            default: break;
          }
        } else if (player->craft_items[1] == 0) {
          // Top-middle slot clear (chestplate recipes)
          switch (first_item) {
            case I_leather: *item = I_leather_chestplate; *count = 1; return;
            case I_iron_ingot: *item = I_iron_chestplate; *count = 1; return;
            case I_gold_ingot: *item = I_golden_chestplate; *count = 1; return;
            case I_diamond: *item = I_diamond_chestplate; *count = 1; return;
            case I_netherite_ingot: *item = I_netherite_chestplate; *count = 1; return;
            default: break;
          }
        }
      }
      break;

    case 9:
      // Uniform 3x3 shaped recipes
      if (identical) switch (first_item) {
        case I_iron_ingot: *item = I_iron_block; *count = 1; return;
        case I_gold_ingot: *item = I_gold_block; *count = 1; return;
        case I_diamond: *item = I_diamond_block; *count = 1; return;
        case I_redstone: *item = I_redstone_block; *count = 1; return;
        case I_coal: *item = I_coal_block; *count = 1; return;
        case I_copper_ingot: *item = I_copper_block; *count = 1; return;
        default: break;
      }
      break;

    default: break;

  }

  *count = 0;
  *item = 0;

}

#define registerSmeltingRecipe(a, b) \
  if (*material == a && (*output_item == b || *output_item == 0)) *output_item = b

void getSmeltingOutput (PlayerData *player) {

  uint8_t *material_count = &player->craft_count[0];
  uint8_t *fuel_count = &player->craft_count[1];

  // Don't process if we're missing material or fuel
  if (*material_count == 0 || *fuel_count == 0) return;

  uint16_t *material = &player->craft_items[0];
  uint16_t *fuel = &player->craft_items[1];

  // Don't process if we're missing material or fuel
  if (*material == 0 || *fuel == 0) return;

  // Furnace output is 3rd crafting table slot
  uint8_t *output_count = &player->craft_count[2];
  uint16_t *output_item = &player->craft_items[2];

  // Determine fuel efficiency based on the type of item
  // Since we can't represent fractions, some items use a random component
  // to represent the fractional part. In some cases (e.g. sticks), this
  // can lead to a fuel_value of 0, which means that the fuel gets consumed
  // without processing any materials.
  uint8_t fuel_value = 0;
  if (*fuel == I_coal) fuel_value = 8;
  else if (*fuel == I_charcoal) fuel_value = 8;
  else if (*fuel == I_coal_block) fuel_value = 80;
  else if (*fuel == I_oak_planks) fuel_value = 1 + (fast_rand() & 1);
  else if (*fuel == I_oak_log) fuel_value = 1 + (fast_rand() & 1);
  else if (*fuel == I_crafting_table) fuel_value = 1 + (fast_rand() & 1);
  else if (*fuel == I_stick) fuel_value = (fast_rand() & 1);
  else if (*fuel == I_oak_sapling) fuel_value = (fast_rand() & 1);
  else if (*fuel == I_wooden_axe) fuel_value = 1;
  else if (*fuel == I_wooden_pickaxe) fuel_value = 1;
  else if (*fuel == I_wooden_shovel) fuel_value = 1;
  else if (*fuel == I_wooden_sword) fuel_value = 1;
  else if (*fuel == I_wooden_hoe) fuel_value = 1;
  else return;

  uint8_t exchange = *material_count > fuel_value ? fuel_value : *material_count;

  registerSmeltingRecipe(I_cobblestone, I_stone);
  else registerSmeltingRecipe(I_oak_log, I_charcoal);
  else registerSmeltingRecipe(I_oak_wood, I_charcoal);
  else registerSmeltingRecipe(I_raw_iron, I_iron_ingot);
  else registerSmeltingRecipe(I_raw_gold, I_gold_ingot);
  else registerSmeltingRecipe(I_sand, I_glass);
  else registerSmeltingRecipe(I_chicken, I_cooked_chicken);
  else registerSmeltingRecipe(I_beef, I_cooked_beef);
  else registerSmeltingRecipe(I_porkchop, I_cooked_porkchop);
  else registerSmeltingRecipe(I_mutton, I_cooked_mutton);
  else return;

  *output_count += exchange;
  *material_count -= exchange;

  *fuel_count -= 1;
  if (*fuel_count == 0) *fuel = 0;

  if (*material_count <= 0) {
    *material_count = 0;
    *material = 0;
  } else return getSmeltingOutput(player);

  return;

}
