// src/recipes.zig
const std = @import("std");
const c = @import("c_api.zig").c;

// Defines the structure for a single crafting recipe.
pub const Recipe = struct {
    /// A compact representation of the recipe shape using a slice.
    /// '0' represents an empty slot within the recipe's bounding box.
    shape: []const u16,
    width: u8,
    height: u8,

    /// If non-zero, this recipe is shapeless and requires this many of `shape[0]`.
    /// If zero, the recipe is shaped.
    shapeless_count: u8 = 0,

    output_item: u16,
    output_count: u8,
};

// A compile-time known array of all crafting recipes in the game.
// This is a single, large, static array literal to avoid comptime complexity.
pub const recipes: []const Recipe = &.{
    // === Shapeless: One-to-Many ===
    // Width and height are set to 0 as they are unused for shapeless recipes.
    .{ .shape = &.{c.I_oak_log}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_oak_planks, .output_count = 4 },
    .{ .shape = &.{c.I_oak_planks}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_oak_button, .output_count = 1 },
    .{ .shape = &.{c.I_iron_block}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_iron_ingot, .output_count = 9 },
    .{ .shape = &.{c.I_gold_block}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_gold_ingot, .output_count = 9 },
    .{ .shape = &.{c.I_diamond_block}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_diamond, .output_count = 9 },
    .{ .shape = &.{c.I_redstone_block}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_redstone, .output_count = 9 },
    .{ .shape = &.{c.I_coal_block}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_coal, .output_count = 9 },
    .{ .shape = &.{c.I_copper_block}, .width = 0, .height = 0, .shapeless_count = 1, .output_item = c.I_copper_ingot, .output_count = 9 },

    // === Shapeless: Many-to-One ===
    .{ .shape = &.{c.I_iron_ingot}, .width = 0, .height = 0, .shapeless_count = 9, .output_item = c.I_iron_block, .output_count = 1 },
    .{ .shape = &.{c.I_gold_ingot}, .width = 0, .height = 0, .shapeless_count = 9, .output_item = c.I_gold_block, .output_count = 1 },
    .{ .shape = &.{c.I_diamond}, .width = 0, .height = 0, .shapeless_count = 9, .output_item = c.I_diamond_block, .output_count = 1 },
    .{ .shape = &.{c.I_redstone}, .width = 0, .height = 0, .shapeless_count = 9, .output_item = c.I_redstone_block, .output_count = 1 },
    .{ .shape = &.{c.I_coal}, .width = 0, .height = 0, .shapeless_count = 9, .output_item = c.I_coal_block, .output_count = 1 },
    .{ .shape = &.{c.I_copper_ingot}, .width = 0, .height = 0, .shapeless_count = 9, .output_item = c.I_copper_block, .output_count = 1 },

    // === Shaped: Miscellaneous ===
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks }, .width = 1, .height = 2, .output_item = c.I_stick, .output_count = 4 },
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks }, .width = 2, .height = 1, .output_item = c.I_oak_pressure_plate, .output_count = 1 },
    .{ .shape = &.{ c.I_coal, c.I_stick }, .width = 1, .height = 2, .output_item = c.I_torch, .output_count = 4 },
    .{ .shape = &.{ c.I_charcoal, c.I_stick }, .width = 1, .height = 2, .output_item = c.I_torch, .output_count = 4 },
    .{ .shape = &.{ c.I_iron_ingot, 0, 0, c.I_iron_ingot }, .width = 2, .height = 2, .output_item = c.I_shears, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_iron_ingot, c.I_iron_ingot, 0 }, .width = 2, .height = 2, .output_item = c.I_shears, .output_count = 1 },
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks, c.I_oak_planks }, .width = 3, .height = 1, .output_item = c.I_oak_slab, .output_count = 6 },
    .{ .shape = &.{ c.I_cobblestone, c.I_cobblestone, c.I_cobblestone }, .width = 3, .height = 1, .output_item = c.I_cobblestone_slab, .output_count = 6 },
    .{ .shape = &.{ c.I_stone, c.I_stone, c.I_stone }, .width = 3, .height = 1, .output_item = c.I_stone_slab, .output_count = 6 },
    .{ .shape = &.{ c.I_snow_block, c.I_snow_block, c.I_snow_block }, .width = 3, .height = 1, .output_item = c.I_snow, .output_count = 6 },
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks, c.I_oak_planks, c.I_oak_planks }, .width = 2, .height = 2, .output_item = c.I_crafting_table, .output_count = 1 },
    .{ .shape = &.{ c.I_oak_log, c.I_oak_log, c.I_oak_log, c.I_oak_log }, .width = 2, .height = 2, .output_item = c.I_oak_wood, .output_count = 3 },
    .{ .shape = &.{ c.I_snowball, c.I_snowball, c.I_snowball, c.I_snowball }, .width = 2, .height = 2, .output_item = c.I_snow_block, .output_count = 1 },
    .{ .shape = &.{ c.I_oak_slab, 0, c.I_oak_slab, c.I_oak_slab, 0, c.I_oak_slab, c.I_oak_slab, 0, c.I_oak_slab }, .width = 3, .height = 3, .output_item = c.I_composter, .output_count = 1 },
    .{ .shape = &.{ c.I_cobblestone, c.I_cobblestone, c.I_cobblestone, c.I_cobblestone, 0, c.I_cobblestone, c.I_cobblestone, c.I_cobblestone, c.I_cobblestone }, .width = 3, .height = 3, .output_item = c.I_furnace, .output_count = 1 },
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks, c.I_oak_planks, c.I_oak_planks, 0, c.I_oak_planks, c.I_oak_planks, c.I_oak_planks, c.I_oak_planks }, .width = 3, .height = 3, .output_item = c.I_chest, .output_count = 1 },

    // === Tools: Wood ===
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks, c.I_oak_planks, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_wooden_pickaxe, .output_count = 1 },
    .{ .shape = &.{ c.I_oak_planks, c.I_oak_planks, 0, c.I_oak_planks, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_wooden_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_oak_planks, c.I_oak_planks, 0, c.I_stick, c.I_oak_planks, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_wooden_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_oak_planks, 0, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_wooden_shovel, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_oak_planks, 0, 0, c.I_oak_planks, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_wooden_sword, .output_count = 1 },

    // === Tools: Stone ===
    .{ .shape = &.{ c.I_cobblestone, c.I_cobblestone, c.I_cobblestone, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_stone_pickaxe, .output_count = 1 },
    .{ .shape = &.{ c.I_cobblestone, c.I_cobblestone, 0, c.I_cobblestone, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_stone_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_cobblestone, c.I_cobblestone, 0, c.I_stick, c.I_cobblestone, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_stone_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_cobblestone, 0, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_stone_shovel, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_cobblestone, 0, 0, c.I_cobblestone, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_stone_sword, .output_count = 1 },

    // === Tools: Iron ===
    .{ .shape = &.{ c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_iron_pickaxe, .output_count = 1 },
    .{ .shape = &.{ c.I_iron_ingot, c.I_iron_ingot, 0, c.I_iron_ingot, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_iron_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_iron_ingot, c.I_iron_ingot, 0, c.I_stick, c.I_iron_ingot, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_iron_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_iron_ingot, 0, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_iron_shovel, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_iron_ingot, 0, 0, c.I_iron_ingot, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_iron_sword, .output_count = 1 },

    // === Tools: Golden ===
    .{ .shape = &.{ c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_golden_pickaxe, .output_count = 1 },
    .{ .shape = &.{ c.I_gold_ingot, c.I_gold_ingot, 0, c.I_gold_ingot, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_golden_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_gold_ingot, c.I_gold_ingot, 0, c.I_stick, c.I_gold_ingot, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_golden_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_gold_ingot, 0, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_golden_shovel, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_gold_ingot, 0, 0, c.I_gold_ingot, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_golden_sword, .output_count = 1 },

    // === Tools: Diamond ===
    .{ .shape = &.{ c.I_diamond, c.I_diamond, c.I_diamond, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_diamond_pickaxe, .output_count = 1 },
    .{ .shape = &.{ c.I_diamond, c.I_diamond, 0, c.I_diamond, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_diamond_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_diamond, c.I_diamond, 0, c.I_stick, c.I_diamond, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_diamond_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_diamond, 0, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_diamond_shovel, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_diamond, 0, 0, c.I_diamond, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_diamond_sword, .output_count = 1 },

    // === Tools: Netherite ===
    .{ .shape = &.{ c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_netherite_pickaxe, .output_count = 1 },
    .{ .shape = &.{ c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_netherite_ingot, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_netherite_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_stick, c.I_netherite_ingot, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_netherite_axe, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_netherite_ingot, 0, 0, c.I_stick, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_netherite_shovel, .output_count = 1 },
    .{ .shape = &.{ 0, c.I_netherite_ingot, 0, 0, c.I_netherite_ingot, 0, 0, c.I_stick, 0 }, .width = 3, .height = 3, .output_item = c.I_netherite_sword, .output_count = 1 },

    // === Armor: Leather ===
    .{ .shape = &.{ c.I_leather, c.I_leather, c.I_leather, c.I_leather, 0, c.I_leather }, .width = 3, .height = 2, .output_item = c.I_leather_helmet, .output_count = 1 },
    .{ .shape = &.{ c.I_leather, 0, c.I_leather, c.I_leather, c.I_leather, c.I_leather, c.I_leather, c.I_leather, c.I_leather }, .width = 3, .height = 3, .output_item = c.I_leather_chestplate, .output_count = 1 },
    .{ .shape = &.{ c.I_leather, c.I_leather, c.I_leather, c.I_leather, 0, c.I_leather, c.I_leather, 0, c.I_leather }, .width = 3, .height = 3, .output_item = c.I_leather_leggings, .output_count = 1 },
    .{ .shape = &.{ c.I_leather, 0, c.I_leather, c.I_leather, 0, c.I_leather }, .width = 3, .height = 2, .output_item = c.I_leather_boots, .output_count = 1 },

    // === Armor: Iron ===
    .{ .shape = &.{ c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, 0, c.I_iron_ingot }, .width = 3, .height = 2, .output_item = c.I_iron_helmet, .output_count = 1 },
    .{ .shape = &.{ c.I_iron_ingot, 0, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot }, .width = 3, .height = 3, .output_item = c.I_iron_chestplate, .output_count = 1 },
    .{ .shape = &.{ c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, c.I_iron_ingot, 0, c.I_iron_ingot, c.I_iron_ingot, 0, c.I_iron_ingot }, .width = 3, .height = 3, .output_item = c.I_iron_leggings, .output_count = 1 },
    .{ .shape = &.{ c.I_iron_ingot, 0, c.I_iron_ingot, c.I_iron_ingot, 0, c.I_iron_ingot }, .width = 3, .height = 2, .output_item = c.I_iron_boots, .output_count = 1 },

    // === Armor: Golden ===
    .{ .shape = &.{ c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, 0, c.I_gold_ingot }, .width = 3, .height = 2, .output_item = c.I_golden_helmet, .output_count = 1 },
    .{ .shape = &.{ c.I_gold_ingot, 0, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot }, .width = 3, .height = 3, .output_item = c.I_golden_chestplate, .output_count = 1 },
    .{ .shape = &.{ c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, c.I_gold_ingot, 0, c.I_gold_ingot, c.I_gold_ingot, 0, c.I_gold_ingot }, .width = 3, .height = 3, .output_item = c.I_golden_leggings, .output_count = 1 },
    .{ .shape = &.{ c.I_gold_ingot, 0, c.I_gold_ingot, c.I_gold_ingot, 0, c.I_gold_ingot }, .width = 3, .height = 2, .output_item = c.I_golden_boots, .output_count = 1 },

    // === Armor: Diamond ===
    .{ .shape = &.{ c.I_diamond, c.I_diamond, c.I_diamond, c.I_diamond, 0, c.I_diamond }, .width = 3, .height = 2, .output_item = c.I_diamond_helmet, .output_count = 1 },
    .{ .shape = &.{ c.I_diamond, 0, c.I_diamond, c.I_diamond, c.I_diamond, c.I_diamond, c.I_diamond, c.I_diamond, c.I_diamond }, .width = 3, .height = 3, .output_item = c.I_diamond_chestplate, .output_count = 1 },
    .{ .shape = &.{ c.I_diamond, c.I_diamond, c.I_diamond, c.I_diamond, 0, c.I_diamond, c.I_diamond, 0, c.I_diamond }, .width = 3, .height = 3, .output_item = c.I_diamond_leggings, .output_count = 1 },
    .{ .shape = &.{ c.I_diamond, 0, c.I_diamond, c.I_diamond, 0, c.I_diamond }, .width = 3, .height = 2, .output_item = c.I_diamond_boots, .output_count = 1 },

    // === Armor: Netherite ===
    .{ .shape = &.{ c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_netherite_ingot }, .width = 3, .height = 2, .output_item = c.I_netherite_helmet, .output_count = 1 },
    .{ .shape = &.{ c.I_netherite_ingot, 0, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot }, .width = 3, .height = 3, .output_item = c.I_netherite_chestplate, .output_count = 1 },
    .{ .shape = &.{ c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_netherite_ingot }, .width = 3, .height = 3, .output_item = c.I_netherite_leggings, .output_count = 1 },
    .{ .shape = &.{ c.I_netherite_ingot, 0, c.I_netherite_ingot, c.I_netherite_ingot, 0, c.I_netherite_ingot }, .width = 3, .height = 2, .output_item = c.I_netherite_boots, .output_count = 1 },
};
