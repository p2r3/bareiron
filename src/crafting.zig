const std = @import("std");
const c = @import("c_api.zig").c;
const recipes_mod = @import("recipes.zig");
const Recipe = recipes_mod.Recipe;

/// Checks a shaped recipe against the player's grid at a specific offset.
/// This function verifies two things:
/// 1. The items in the grid match the recipe's shape at the given position.
/// 2. There are no other items on the grid that are not part of the recipe.
fn checkShapedMatchAt(
    grid: [9]u16,
    recipe: Recipe,
    start_row: u8,
    start_col: u8,
) bool {
    // Create a temporary copy of the grid to mark off items as they are matched.
    var temp_grid = grid;

    // Iterate through the recipe's shape (defined by its width and height).
    var r_row: u8 = 0;
    while (r_row < recipe.height) : (r_row += 1) {
        var r_col: u8 = 0;
        while (r_col < recipe.width) : (r_col += 1) {
            const recipe_idx = r_row * recipe.width + r_col;
            const grid_idx = (start_row + r_row) * 3 + (start_col + r_col);

            const recipe_item = recipe.shape[recipe_idx];
            const grid_item = grid[grid_idx];

            // If the recipe specifies an item but the grid slot is different, it's not a match.
            if (recipe_item != 0 and recipe_item != grid_item) {
                return false;
            }

            // If the recipe specifies an empty slot but the grid has an item, it's not a match.
            if (recipe_item == 0 and grid_item != 0) {
                return false;
            }

            // If they match, mark this grid slot as "accounted for" in our temporary grid.
            if (recipe_item != 0) {
                temp_grid[grid_idx] = 0;
            }
        }
    }

    // After checking the recipe shape, ensure the rest of the grid is empty.
    // This prevents recipes from matching if there are extra, unrelated items.
    for (temp_grid) |item| {
        if (item != 0) return false;
    }

    return true;
}

/// Determines the output of the player's crafting grid.
/// This function is exported to be called from C code.
pub export fn getCraftingOutputC(
    _: *c.ServerContext,
    player: *c.PlayerData,
    count: *u8,
    item: *u16,
) void {
    var filled: u8 = 0;
    var first_item: u16 = 0;
    var all_identical = true;

    // Analyze the player's crafting grid to get basic info.
    for (player.craft_items) |grid_item| {
        if (grid_item != 0) {
            filled += 1;
            if (first_item == 0) {
                first_item = grid_item;
            } else if (grid_item != first_item) {
                all_identical = false;
            }
        }
    }

    // If the grid is empty, there's no output.
    if (filled == 0) {
        item.* = 0;
        count.* = 0;
        return;
    }

    // The 'inline for' loop iterates over all recipes at COMPILE TIME.
    // The compiler unrolls this loop and generates a series of direct checks,
    // which is highly efficient.
    inline for (recipes_mod.recipes) |recipe| {
        // --- 1. Check Shapeless Recipes ---
        // These are simple checks based on item count and type.
        if (recipe.shapeless_count > 0) {
            if (filled == recipe.shapeless_count and all_identical and first_item == recipe.shape[0]) {
                item.* = recipe.output_item;
                count.* = recipe.output_count;
                return;
            }
        }
        // --- 2. Check Shaped Recipes ---
        else {
            // First, do a quick check on the number of ingredients. If it doesn't match
            // the number of filled slots, we can skip the more expensive shape check.
            var ingredient_count: u8 = 0;
            // ================== FIX START ==================
            for (recipe.shape) |ing| {
                if (ing != 0) ingredient_count += 1;
            }
            // =================== FIX END ===================

            if (filled == ingredient_count) {
                // Iterate through all possible top-left starting positions for the recipe shape
                // within the 3x3 grid.
                const max_start_row = 3 - recipe.height;
                const max_start_col = 3 - recipe.width;
                var start_row: u8 = 0;
                while (start_row <= max_start_row) : (start_row += 1) {
                    var start_col: u8 = 0;
                    while (start_col <= max_start_col) : (start_col += 1) {
                        // Check if the recipe matches at this specific position.
                        if (checkShapedMatchAt(player.craft_items, recipe, start_row, start_col)) {
                            item.* = recipe.output_item;
                            count.* = recipe.output_count;
                            return;
                        }
                    }
                }
            }
        }
    }

    // If the loop finishes without finding a match, there is no output.
    item.* = 0;
    count.* = 0;
}

/// Determines the output of the furnace based on the input and fuel.
/// This function remains unchanged as it is not related to shaped crafting.
pub export fn getSmeltingOutput(ctx: *c.ServerContext, player: *c.PlayerData) void {
    const material_count = &player.craft_count[0];
    const fuel_count = &player.craft_count[1];

    if (material_count.* == 0 or fuel_count.* == 0) return;

    const material_val: u16 = player.craft_items[0];
    const fuel_val: u16 = player.craft_items[1];
    if (material_val == 0 or fuel_val == 0) return;

    const output_count = &player.craft_count[2];
    var output_item_val: u16 = player.craft_items[2];

    var fuel_value: u8 = 0;
    if (fuel_val == c.I_coal) fuel_value = 8 else if (fuel_val == c.I_charcoal) fuel_value = 8 else if (fuel_val == c.I_coal_block) fuel_value = 80 else if (fuel_val == c.I_oak_planks) fuel_value = 1 + (@as(u8, @truncate(c.fast_rand(ctx))) & 1) else if (fuel_val == c.I_oak_log) fuel_value = 1 + (@as(u8, @truncate(c.fast_rand(ctx))) & 1) else if (fuel_val == c.I_crafting_table) fuel_value = 1 + (@as(u8, @truncate(c.fast_rand(ctx))) & 1) else if (fuel_val == c.I_stick) fuel_value = (@as(u8, @truncate(c.fast_rand(ctx))) & 1) else if (fuel_val == c.I_oak_sapling) fuel_value = (@as(u8, @truncate(c.fast_rand(ctx))) & 1) else if (fuel_val == c.I_wooden_axe) fuel_value = 1 else if (fuel_val == c.I_wooden_pickaxe) fuel_value = 1 else if (fuel_val == c.I_wooden_shovel) fuel_value = 1 else if (fuel_val == c.I_wooden_sword) fuel_value = 1 else if (fuel_val == c.I_wooden_hoe) fuel_value = 1 else return;

    const exchange: u8 = if (material_count.* > fuel_value) fuel_value else material_count.*;

    if (material_val == c.I_cobblestone and (output_item_val == c.I_stone or output_item_val == 0)) output_item_val = c.I_stone else if (material_val == c.I_oak_log and (output_item_val == c.I_charcoal or output_item_val == 0)) output_item_val = c.I_charcoal else if (material_val == c.I_oak_wood and (output_item_val == c.I_charcoal or output_item_val == 0)) output_item_val = c.I_charcoal else if (material_val == c.I_raw_iron and (output_item_val == c.I_iron_ingot or output_item_val == 0)) output_item_val = c.I_iron_ingot else if (material_val == c.I_raw_gold and (output_item_val == c.I_gold_ingot or output_item_val == 0)) output_item_val = c.I_gold_ingot else if (material_val == c.I_sand and (output_item_val == c.I_glass or output_item_val == 0)) output_item_val = c.I_glass else if (material_val == c.I_chicken and (output_item_val == c.I_cooked_chicken or output_item_val == 0)) output_item_val = c.I_cooked_chicken else if (material_val == c.I_beef and (output_item_val == c.I_cooked_beef or output_item_val == 0)) output_item_val = c.I_cooked_beef else if (material_val == c.I_porkchop and (output_item_val == c.I_cooked_porkchop or output_item_val == 0)) output_item_val = c.I_cooked_porkchop else if (material_val == c.I_mutton and (output_item_val == c.I_cooked_mutton or output_item_val == 0)) output_item_val = c.I_cooked_mutton else return;

    output_count.* += exchange;
    material_count.* -= exchange;
    player.craft_items[2] = output_item_val;

    fuel_count.* -= 1;
    if (fuel_count.* == 0) player.craft_items[1] = 0;

    if (material_count.* <= 0) {
        material_count.* = 0;
        player.craft_items[0] = 0;
    } else {
        getSmeltingOutput(ctx, player);
    }
}
