#include "packer.h"
#include <algorithm> 
#include <iostream>
#include <vector>
#include <functional>
#include <limits>  // Add this include for std::numeric_limits

const std::tuple<long, long, long> START_POSITION = {0, 0, 0};

Packer::Packer() {}

const std::vector<Bin>& Packer::getBins() const {
    return bins;
}

const std::vector<Item>& Packer::getItems() const {
    return items;
}

const std::vector<Item>& Packer::getUnfitItems() const {
    return unfit_items;
}

void Packer::addBin(const Bin& bin) {
    bins.push_back(bin);
}

void Packer::addItem(const Item& item) {
    items.push_back(item);
}

std::optional<std::reference_wrapper<Bin>> Packer::findFittedBin(Item& item) {
    for (auto& bin : bins) {
        if (!bin.putItem(item, START_POSITION)) {
            continue;
        }
        if (bin.getItems().size() == 1 && &bin.getItems()[0].get() == &item) {
            bin.setItems({});
        }
        return std::ref(bin);
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<Bin>> Packer::getBiggerBinThan(const Bin& other_bin) {
    auto it = std::find_if(bins.begin(), bins.end(), [&other_bin](Bin& bin) {
        return bin.getVolume() > other_bin.getVolume();
    });
    if (it != bins.end()) {
        return std::ref(*it);
    }
    return std::nullopt;
}

void Packer::unfitItem(std::vector<Item*>& item_ptrs) {
    if (!item_ptrs.empty()) {
        unfit_items.push_back(*item_ptrs.front());
        item_ptrs.erase(item_ptrs.begin());
    }
}

std::vector<Item*> Packer::packToBin(Bin& bin, std::vector<Item*>& item_ptrs) {
    std::vector<Item*> unpacked;
    std::optional<std::reference_wrapper<Bin>> b2;
    
    // Fast sort with simpler criteria
    std::sort(item_ptrs.begin(), item_ptrs.end(), [](const Item* a, const Item* b) {
        return a->getVolume() > b->getVolume();
    });

    if (!bin.putItem(*item_ptrs[0], START_POSITION)) {
        b2 = getBiggerBinThan(bin);
        if (b2) {
            return packToBin(b2->get(), item_ptrs);
        }
        return {item_ptrs.begin(), item_ptrs.end()};
    }
    
    // Cache bin dimensions
    const long binWidth = bin.getWidth();
    const long binHeight = bin.getHeight();
    const long binDepth = bin.getDepth();

    for (size_t i = 1; i < item_ptrs.size(); ++i) {
        bool fitted = false;
        Item* current_item = item_ptrs[i];
        
        // CRITICAL FIX: Use a single approach for all item types with special case handling
        // This drastically reduces the time complexity
        
        // Calculate step size based on bin dimensions - much larger steps
        const long step_x = std::max(10L, binWidth / 5);
        const long step_y = std::max(10L, binHeight / 5);
        const long step_z = std::max(10L, binDepth / 5);
        
        // Try adjacent to existing items first (most efficient)
        for (const auto& item_b : bin.getItems()) {
            if (fitted) break;
            
            const auto& pos_b = item_b.get().getPosition();
            const auto& dim_b = item_b.get().getDimension();
            
            // Only generate a few high-value positions
            std::vector<std::tuple<long, long, long>> positions;
            
            // Bottom load only at ground level
            if (current_item->bottom_load_only) {
                positions = {
                    {std::get<0>(pos_b) + dim_b[0], 0, std::get<2>(pos_b)},
                    {std::get<0>(pos_b), 0, std::get<2>(pos_b) + dim_b[2]}
                };
            }
            // For regular and disable_stacking items
            else {
                positions = {
                    {std::get<0>(pos_b) + dim_b[0], std::get<1>(pos_b), std::get<2>(pos_b)}, // Right
                    {std::get<0>(pos_b), std::get<1>(pos_b) + dim_b[1], std::get<2>(pos_b)}, // Above
                    {std::get<0>(pos_b), std::get<1>(pos_b), std::get<2>(pos_b) + dim_b[2]}  // Behind
                };
                
                // Don't try above position for disable_stacking
                if (item_b.get().disable_stacking || current_item->disable_stacking) {
                    positions.erase(positions.begin() + 1);
                }
            }
            
            // Try each position
            for (const auto& pos : positions) {
                if (bin.putItem(*current_item, pos)) {
                    fitted = true;
                    break;
                }
            }
        }
        
        // CRITICAL FIX: If not fitted, use very sparse grid search
        if (!fitted) {
            // Use much larger increment for grid search
            if (current_item->bottom_load_only) {
                // Bottom load items only need 2D search at y=0
                for (long x = 0; x <= binWidth && !fitted; x += step_x) {
                    for (long z = 0; z <= binDepth && !fitted; z += step_z) { // FIX: z <= binDepth 
                        if (bin.putItem(*current_item, {x, 0, z})) {
                            fitted = true;
                            break;
                        }
                    }
                }
            } 
            else {
                // Regular items - just do a very sparse 3D grid
                for (long y = 0; y <= binHeight && !fitted; y += step_y) {
                    // Skip higher y values for disable_stacking
                    if (current_item->disable_stacking && y > step_y) continue;
                    
                    for (long x = 0; x <= binWidth && !fitted; x += step_x) {
                        for (long z = 0; z <= binDepth && !fitted; z += step_z) { // FIX: z <= binDepth
                            if (bin.putItem(*current_item, {x, y, z})) {
                                fitted = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        if (!fitted) {
            unpacked.push_back(current_item);
        }
    }
    return unpacked;
}

void Packer::pack() {
    std::sort(bins.begin(), bins.end(), [](const Bin& a, const Bin& b) {
        return a.getVolume() < b.getVolume();
    });
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        return a.getVolume() > b.getVolume();
    });

    std::vector<Item*> item_ptrs;
    for (auto& itm : items) {
        item_ptrs.push_back(&itm);
    }

    while (!item_ptrs.empty()) {
        auto bin = findFittedBin(*item_ptrs[0]);
        if (!bin) {
            unfitItem(item_ptrs);
            continue;
        }
        auto unpacked_items = packToBin(bin->get(), item_ptrs);
        item_ptrs = unpacked_items;
    }
}
