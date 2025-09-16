const std = @import("std");
const c = @import("c_api.zig").c;
const pre = @import("precomputed_recipes.zig");

pub export fn getCraftingOutputC(
    _: *c.ServerContext,
    player: *c.PlayerData,
    count: *u8,
    item: *u16,
) void {
    if (pre.shapedLookup(player.craft_items)) |out| {
        item.* = out.item;
        count.* = out.count;
        return;
    }

    var filled: u8 = 0;
    var first_item: u16 = 0;
    var all_identical = true;
    for (player.craft_items) |grid_item| {
        if (grid_item != 0) {
            filled += 1;
            if (first_item == 0) first_item = grid_item else if (grid_item != first_item) all_identical = false;
        }
    }
    if (filled == 0) {
        item.* = 0;
        count.* = 0;
        return;
    }
    if (all_identical) {
        if (pre.shapelessLookup(first_item, filled)) |out2| {
            item.* = out2.item;
            count.* = out2.count;
            return;
        }
    }

    item.* = 0;
    count.* = 0;
}

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
