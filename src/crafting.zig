const std = @import("std");
const c = @import("c_api.zig").c;

pub export fn getCraftingOutputC(
    _: *c.ServerContext,
    player: *c.PlayerData,
    count: *u8,
    item: *u16,
) void {
    var filled: u8 = 0;
    var first: u8 = 10;
    var identical = true;

    var i: u8 = 0;
    while (i < 9) : (i += 1) {
        if (player.craft_items[i] != 0) {
            filled += 1;
            if (first == 10) {
                first = i;
            } else if (player.craft_items[i] != player.craft_items[first]) {
                identical = false;
            }
        }
    }

    if (filled == 0) {
        item.* = 0;
        count.* = 0;
        return;
    }

    const first_item: u16 = player.craft_items[first];
    const first_col: u8 = first % 3;
    const first_row: u8 = first / 3;

    switch (filled) {
        0 => {
            item.* = 0;
            count.* = 0;
            return;
        },
        1 => {
            switch (first_item) {
                c.I_oak_log => {
                    item.* = c.I_oak_planks;
                    count.* = 4;
                    return;
                },
                c.I_oak_planks => {
                    item.* = c.I_oak_button;
                    count.* = 1;
                    return;
                },
                c.I_iron_block => {
                    item.* = c.I_iron_ingot;
                    count.* = 9;
                    return;
                },
                c.I_gold_block => {
                    item.* = c.I_gold_ingot;
                    count.* = 9;
                    return;
                },
                c.I_diamond_block => {
                    item.* = c.I_diamond;
                    count.* = 9;
                    return;
                },
                c.I_redstone_block => {
                    item.* = c.I_redstone;
                    count.* = 9;
                    return;
                },
                c.I_coal_block => {
                    item.* = c.I_coal;
                    count.* = 9;
                    return;
                },
                c.I_copper_block => {
                    item.* = c.I_copper_ingot;
                    count.* = 9;
                    return;
                },
                else => {},
            }
        },
        2 => {
            switch (first_item) {
                c.I_oak_planks => {
                    if (first_col != 2 and player.craft_items[first + 1] == c.I_oak_planks) {
                        item.* = c.I_oak_pressure_plate;
                        count.* = 1;
                        return;
                    } else if (first_row != 2 and player.craft_items[first + 3] == c.I_oak_planks) {
                        item.* = c.I_stick;
                        count.* = 4;
                        return;
                    }
                },
                c.I_charcoal, c.I_coal => {
                    if (first_row != 2 and player.craft_items[first + 3] == c.I_stick) {
                        item.* = c.I_torch;
                        count.* = 4;
                        return;
                    }
                },
                c.I_iron_ingot => {
                    if ((first_row != 2 and first_col != 2 and player.craft_items[first + 4] == c.I_iron_ingot) or
                        (first_row != 2 and first_col != 0 and player.craft_items[first + 2] == c.I_iron_ingot))
                    {
                        item.* = c.I_shears;
                        count.* = 1;
                        return;
                    }
                },
                else => {},
            }
        },
        3 => {
            switch (first_item) {
                c.I_oak_planks, c.I_cobblestone, c.I_stone, c.I_snow_block => {
                    if (first_col == 0 and player.craft_items[first + 1] == first_item and player.craft_items[first + 2] == first_item) {
                        if (first_item == c.I_oak_planks) item.* = c.I_oak_slab else if (first_item == c.I_cobblestone) item.* = c.I_cobblestone_slab else if (first_item == c.I_stone) item.* = c.I_stone_slab else if (first_item == c.I_snow_block) item.* = c.I_snow;
                        count.* = 6;
                        return;
                    }
                },
                else => {},
            }
            switch (first_item) {
                c.I_oak_planks, c.I_cobblestone, c.I_iron_ingot, c.I_gold_ingot, c.I_diamond, c.I_netherite_ingot => {
                    if (first_row == 0 and player.craft_items[first + 3] == c.I_stick and player.craft_items[first + 6] == c.I_stick) {
                        if (first_item == c.I_oak_planks) item.* = c.I_wooden_shovel else if (first_item == c.I_cobblestone) item.* = c.I_stone_shovel else if (first_item == c.I_iron_ingot) item.* = c.I_iron_shovel else if (first_item == c.I_gold_ingot) item.* = c.I_golden_shovel else if (first_item == c.I_diamond) item.* = c.I_diamond_shovel else if (first_item == c.I_netherite_ingot) item.* = c.I_netherite_shovel;
                        count.* = 1;
                        return;
                    }
                    if (first_row == 0 and player.craft_items[first + 3] == first_item and player.craft_items[first + 6] == c.I_stick) {
                        if (first_item == c.I_oak_planks) item.* = c.I_wooden_sword else if (first_item == c.I_cobblestone) item.* = c.I_stone_sword else if (first_item == c.I_iron_ingot) item.* = c.I_iron_sword else if (first_item == c.I_gold_ingot) item.* = c.I_golden_sword else if (first_item == c.I_diamond) item.* = c.I_diamond_sword else if (first_item == c.I_netherite_ingot) item.* = c.I_netherite_sword;
                        count.* = 1;
                        return;
                    }
                },
                else => {},
            }
        },
        4 => {
            switch (first_item) {
                c.I_oak_planks, c.I_oak_log, c.I_snowball => {
                    if (first_col != 2 and first_row != 2 and player.craft_items[first + 1] == first_item and player.craft_items[first + 3] == first_item and player.craft_items[first + 4] == first_item) {
                        if (first_item == c.I_oak_planks) {
                            item.* = c.I_crafting_table;
                            count.* = 1;
                        } else if (first_item == c.I_oak_log) {
                            item.* = c.I_oak_wood;
                            count.* = 3;
                        } else if (first_item == c.I_snowball) {
                            item.* = c.I_snow_block;
                            count.* = 3;
                        }
                        return;
                    }
                },
                c.I_leather, c.I_iron_ingot, c.I_gold_ingot, c.I_diamond, c.I_netherite_ingot => {
                    if (first_col == 0 and first_row < 2 and player.craft_items[first + 2] == first_item and player.craft_items[first + 3] == first_item and player.craft_items[first + 5] == first_item) {
                        if (first_item == c.I_leather) item.* = c.I_leather_boots else if (first_item == c.I_iron_ingot) item.* = c.I_iron_boots else if (first_item == c.I_gold_ingot) item.* = c.I_golden_boots else if (first_item == c.I_diamond) item.* = c.I_diamond_boots else if (first_item == c.I_netherite_ingot) item.* = c.I_netherite_boots;
                        count.* = 1;
                        return;
                    }
                },
                else => {},
            }
        },
        5 => {
            switch (first_item) {
                c.I_oak_planks, c.I_cobblestone => {
                    if (first == 0 and player.craft_items[first + 1] == first_item and player.craft_items[first + 2] == first_item and player.craft_items[first + 4] == c.I_stick and player.craft_items[first + 7] == c.I_stick) {
                        if (first_item == c.I_oak_planks) item.* = c.I_wooden_pickaxe else if (first_item == c.I_cobblestone) item.* = c.I_stone_pickaxe;
                        count.* = 1;
                        return;
                    }
                    if (first < 2 and player.craft_items[first + 1] == first_item and ((player.craft_items[first + 3] == first_item and player.craft_items[first + 4] == c.I_stick and player.craft_items[first + 7] == c.I_stick) or (player.craft_items[first + 4] == first_item and player.craft_items[first + 3] == c.I_stick and player.craft_items[first + 6] == c.I_stick))) {
                        if (first_item == c.I_oak_planks) item.* = c.I_wooden_axe else if (first_item == c.I_cobblestone) item.* = c.I_stone_axe;
                        count.* = 1;
                        return;
                    }
                },
                c.I_iron_ingot, c.I_gold_ingot, c.I_diamond, c.I_netherite_ingot, c.I_leather => {
                    if (first_item != c.I_leather) {
                        if (first == 0 and player.craft_items[first + 1] == first_item and player.craft_items[first + 2] == first_item and player.craft_items[first + 4] == c.I_stick and player.craft_items[first + 7] == c.I_stick) {
                            if (first_item == c.I_iron_ingot) item.* = c.I_iron_pickaxe else if (first_item == c.I_gold_ingot) item.* = c.I_golden_pickaxe else if (first_item == c.I_diamond) item.* = c.I_diamond_pickaxe else if (first_item == c.I_netherite_ingot) item.* = c.I_netherite_pickaxe;
                            count.* = 1;
                            return;
                        }
                        if (first < 2 and player.craft_items[first + 1] == first_item and ((player.craft_items[first + 3] == first_item and player.craft_items[first + 4] == c.I_stick and player.craft_items[first + 7] == c.I_stick) or (player.craft_items[first + 4] == first_item and player.craft_items[first + 3] == c.I_stick and player.craft_items[first + 6] == c.I_stick))) {
                            if (first_item == c.I_iron_ingot) item.* = c.I_iron_axe else if (first_item == c.I_gold_ingot) item.* = c.I_golden_axe else if (first_item == c.I_diamond) item.* = c.I_diamond_axe else if (first_item == c.I_netherite_ingot) item.* = c.I_netherite_axe;
                            count.* = 1;
                            return;
                        }
                    }
                    if (first_col == 0 and first_row < 2 and player.craft_items[first + 1] == first_item and player.craft_items[first + 2] == first_item and player.craft_items[first + 3] == first_item and player.craft_items[first + 5] == first_item) {
                        if (first_item == c.I_leather) item.* = c.I_leather_helmet else if (first_item == c.I_iron_ingot) item.* = c.I_iron_helmet else if (first_item == c.I_gold_ingot) item.* = c.I_golden_helmet else if (first_item == c.I_diamond) item.* = c.I_diamond_helmet else if (first_item == c.I_netherite_ingot) item.* = c.I_netherite_helmet;
                        count.* = 1;
                        return;
                    }
                },
                else => {},
            }
        },
        7 => {
            if (identical and player.craft_items[4] == 0 and player.craft_items[7] == 0) {
                switch (first_item) {
                    c.I_leather => {
                        item.* = c.I_leather_leggings;
                        count.* = 1;
                        return;
                    },
                    c.I_iron_ingot => {
                        item.* = c.I_iron_leggings;
                        count.* = 1;
                        return;
                    },
                    c.I_gold_ingot => {
                        item.* = c.I_golden_leggings;
                        count.* = 1;
                        return;
                    },
                    c.I_diamond => {
                        item.* = c.I_diamond_leggings;
                        count.* = 1;
                        return;
                    },
                    c.I_netherite_ingot => {
                        item.* = c.I_netherite_leggings;
                        count.* = 1;
                        return;
                    },
                    else => {},
                }
            }
            if (first_item == c.I_oak_slab and identical and player.craft_items[1] == 0 and player.craft_items[4] == 0) {
                item.* = c.I_composter;
                count.* = 1;
                return;
            }
        },
        8 => {
            if (identical) {
                if (player.craft_items[4] == 0) {
                    switch (first_item) {
                        c.I_cobblestone => {
                            item.* = c.I_furnace;
                            count.* = 1;
                            return;
                        },
                        c.I_oak_planks => {
                            item.* = c.I_chest;
                            count.* = 1;
                            return;
                        },
                        else => {},
                    }
                } else if (player.craft_items[1] == 0) {
                    switch (first_item) {
                        c.I_leather => {
                            item.* = c.I_leather_chestplate;
                            count.* = 1;
                            return;
                        },
                        c.I_iron_ingot => {
                            item.* = c.I_iron_chestplate;
                            count.* = 1;
                            return;
                        },
                        c.I_gold_ingot => {
                            item.* = c.I_golden_chestplate;
                            count.* = 1;
                            return;
                        },
                        c.I_diamond => {
                            item.* = c.I_diamond_chestplate;
                            count.* = 1;
                            return;
                        },
                        c.I_netherite_ingot => {
                            item.* = c.I_netherite_chestplate;
                            count.* = 1;
                            return;
                        },
                        else => {},
                    }
                }
            }
        },
        9 => {
            if (identical) {
                switch (first_item) {
                    c.I_iron_ingot => {
                        item.* = c.I_iron_block;
                        count.* = 1;
                        return;
                    },
                    c.I_gold_ingot => {
                        item.* = c.I_gold_block;
                        count.* = 1;
                        return;
                    },
                    c.I_diamond => {
                        item.* = c.I_diamond_block;
                        count.* = 1;
                        return;
                    },
                    c.I_redstone => {
                        item.* = c.I_redstone_block;
                        count.* = 1;
                        return;
                    },
                    c.I_coal => {
                        item.* = c.I_coal_block;
                        count.* = 1;
                        return;
                    },
                    c.I_copper_ingot => {
                        item.* = c.I_copper_block;
                        count.* = 1;
                        return;
                    },
                    else => {},
                }
            }
        },
        else => {},
    }

    count.* = 0;
    item.* = 0;
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
