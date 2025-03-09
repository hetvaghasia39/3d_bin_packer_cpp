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
    
    // Sort items to prioritize bottom-load-only items first
    std::stable_sort(item_ptrs.begin(), item_ptrs.end(), [](const Item* a, const Item* b) {
        return a->bottom_load_only && !b->bottom_load_only;
    });

    if (!bin.putItem(*item_ptrs[0], START_POSITION)) {
        b2 = getBiggerBinThan(bin);
        if (b2) {
            return packToBin(b2->get(), item_ptrs);
        }
        return {item_ptrs.begin(), item_ptrs.end()};
    }
    
    for (size_t i = 1; i < item_ptrs.size(); ++i) {
        bool fitted = false;
        Item* current_item = item_ptrs[i];
        bool is_bottom_load = current_item->bottom_load_only;
        
        // Define axis priorities based on item type
        std::vector<Axis> axis_priorities;
        if (is_bottom_load) {
            axis_priorities = {Axis::width, Axis::depth, Axis::height};
        } else {
//old logic
            // for (const auto& axis : {Axis::depth, Axis::width, Axis::height}) {
            // for (const auto& axis : {Axis::width, Axis::height, Axis::depth}) {
            // for (const auto& axis : {Axis::height, Axis::width, Axis::depth}) {
            // for (const auto& axis : {Axis::depth, Axis::height, Axis::width}) {


            //axis_priorities = {Axis::depth, Axis::width, Axis::width};
            //axis_priorities = {Axis::width, Axis::height, Axis::depth};
            //axis_priorities = {Axis::height, Axis::width, Axis::depth};
            //axis_priorities = {Axis::depth, Axis::height, Axis::width};
            //axis_priorities = {Axis::depth, Axis::height, Axis::width};
            axis_priorities = {Axis::depth, Axis::height, Axis::width};
        }
        
        // Handle bottom-load-only items - they must be placed at y=0
        if (is_bottom_load) {
            // First try positions adjacent to existing bottom-load items
            for (const auto& item_b : bin.getItems()) {
                if (!fitted && std::get<1>(item_b.get().getPosition()) == 0) {  // Only consider items at y=0
                    // Try positions adjacent to this item
                    std::vector<std::tuple<long, long, long>> adjacent_positions = {
                        // Side by side (X direction)
                        {std::get<0>(item_b.get().getPosition()) + item_b.get().getDimension()[0], 0, std::get<2>(item_b.get().getPosition())},
                        // Behind (Z direction)
                        {std::get<0>(item_b.get().getPosition()), 0, std::get<2>(item_b.get().getPosition()) + item_b.get().getDimension()[2]}
                    };
                    
                    for (const auto& pos : adjacent_positions) {
                        if (bin.putItem(*current_item, pos)) {
                            fitted = true;
                            break;
                        }
                    }
                    
                    if (fitted) break;
                }
            }
            
            // If still not fitted, try a fine-grained grid search on the bottom surface
            if (!fitted) {
                // Use smaller step size for more precise placement
                const long step_x = std::max(1L, bin.getWidth() / 50);
                const long step_z = std::max(1L, bin.getDepth() / 50);
                
                for (long x = 0; x <= bin.getWidth() && !fitted; x += step_x) {
                    for (long z = 0; z <= bin.getDepth() && !fitted; z += step_z) {  // Fix: Changed x <= to z <=
                        std::tuple<long, long, long> item_position = {x, 0, z};
                        if (bin.putItem(*current_item, item_position)) {
                            fitted = true;
                            break;
                        }
                    }
                }
            }
        } 
        // For regular items, use more precise placement to avoid gaps
        else {
            // Step 1: Collect all possible positions adjacent to existing items
            std::vector<std::tuple<long, long, long>> candidate_positions;
            
            for (const auto& item_b : bin.getItems()) {
                // Generate 6 possible positions (all sides of the item)
                std::vector<std::tuple<long, long, long>> adjacent_positions = {
                    // Right side (+X)
                    {std::get<0>(item_b.get().getPosition()) + item_b.get().getDimension()[0], 
                     std::get<1>(item_b.get().getPosition()), 
                     std::get<2>(item_b.get().getPosition())},
                     
                    // On top (+Y)
                    {std::get<0>(item_b.get().getPosition()), 
                     std::get<1>(item_b.get().getPosition()) + item_b.get().getDimension()[1], 
                     std::get<2>(item_b.get().getPosition())},
                     
                    // Behind (+Z)
                    {std::get<0>(item_b.get().getPosition()), 
                     std::get<1>(item_b.get().getPosition()), 
                     std::get<2>(item_b.get().getPosition()) + item_b.get().getDimension()[2]},
                    
                    // Back-corner position (combining X and Z)
                    {std::get<0>(item_b.get().getPosition()) + item_b.get().getDimension()[0],
                     std::get<1>(item_b.get().getPosition()),
                     std::get<2>(item_b.get().getPosition()) + item_b.get().getDimension()[2]},
                     
                    // Top-corner position (combining X and Y)
                    {std::get<0>(item_b.get().getPosition()) + item_b.get().getDimension()[0],
                     std::get<1>(item_b.get().getPosition()) + item_b.get().getDimension()[1],
                     std::get<2>(item_b.get().getPosition())},
                     
                    // Top-back position (combining Y and Z)
                    {std::get<0>(item_b.get().getPosition()),
                     std::get<1>(item_b.get().getPosition()) + item_b.get().getDimension()[1],
                     std::get<2>(item_b.get().getPosition()) + item_b.get().getDimension()[2]}
                };
                
                // Add all these positions to our candidates
                candidate_positions.insert(candidate_positions.end(), 
                                          adjacent_positions.begin(), 
                                          adjacent_positions.end());
            }
            
            // Step 2: Sort positions by "compactness" - closer to (0,0,0) is better
            std::sort(candidate_positions.begin(), candidate_positions.end(), 
                [](const auto& a, const auto& b) {
                    // Calculate Manhattan distance to origin
                    long dist_a = std::get<0>(a) + std::get<1>(a) + std::get<2>(a);
                    long dist_b = std::get<0>(b) + std::get<1>(b) + std::get<2>(b);
                    return dist_a < dist_b;
                });
                
            // Step 3: Try each position
            for (const auto& pos : candidate_positions) {
                if (bin.putItem(*current_item, pos)) {
                    fitted = true;
                    break;
                }
            }
            
            // Step 4: If still not fitted, try a grid search with respect to axis priorities
            if (!fitted) {
                // Try a more precise grid search
                const long step_x = std::max(1L, bin.getWidth() / 30);
                const long step_y = std::max(1L, bin.getHeight() / 30);
                const long step_z = std::max(1L, bin.getDepth() / 30);
                
                // Start search closest to existing items for more compact packing
                std::vector<std::tuple<long, long, long>> grid_positions;
                for (long x = 0; x <= bin.getWidth() && !fitted; x += step_x) {
                    for (long y = 0; y <= bin.getHeight() && !fitted; y += step_y) {
                        for (long z = 0; z <= bin.getDepth() && !fitted; z += step_z) {
                            grid_positions.push_back({x, y, z});
                        }
                    }
                }
                
                // Sort grid positions by distance to the nearest item for more compact packing
                std::sort(grid_positions.begin(), grid_positions.end(), 
                    [&bin](const auto& a, const auto& b) {
                        // Find minimum distance to any existing item
                        long min_dist_a = std::numeric_limits<long>::max();
                        long min_dist_b = std::numeric_limits<long>::max();
                        
                        for (const auto& item_ref : bin.getItems()) {
                            const auto& item = item_ref.get();
                            const auto& pos = item.getPosition();
                            const auto& dim = item.getDimension();
                            
                            // Calculate center points
                            long cx = std::get<0>(pos) + dim[0]/2;
                            long cy = std::get<1>(pos) + dim[1]/2;
                            long cz = std::get<2>(pos) + dim[2]/2;
                            
                            // Manhattan distance from position to item center
                            long dist_a = std::abs(std::get<0>(a) - cx) + 
                                          std::abs(std::get<1>(a) - cy) + 
                                          std::abs(std::get<2>(a) - cz);
                            long dist_b = std::abs(std::get<0>(b) - cx) + 
                                          std::abs(std::get<1>(b) - cy) + 
                                          std::abs(std::get<2>(b) - cz);
                            
                            min_dist_a = std::min(min_dist_a, dist_a);
                            min_dist_b = std::min(min_dist_b, dist_b);
                        }
                        return min_dist_a < min_dist_b;
                    });
                
                // Try each position in order of closest to existing items
                for (const auto& pos : grid_positions) {
                    if (bin.putItem(*current_item, pos)) {
                        fitted = true;
                        break;
                    }
                }
            }
            
            // Step 5: If still not fitted, fall back to original axis-based approach
            if (!fitted) {
                for (const auto& axis : axis_priorities) {
                    if (fitted) break;
                    for (const auto& item_b : bin.getItems()) {
                        std::tuple<long, long, long> item_position;
                        
                        // Calculate position based on current axis
                        if (axis == Axis::width) {
                            item_position = {
                                std::get<0>(item_b.get().getPosition()) + item_b.get().getDimension()[0],
                                std::get<1>(item_b.get().getPosition()),
                                std::get<2>(item_b.get().getPosition())
                            };
                        } else if (axis == Axis::height) {
                            item_position = {
                                std::get<0>(item_b.get().getPosition()),
                                std::get<1>(item_b.get().getPosition()) + item_b.get().getDimension()[1],
                                std::get<2>(item_b.get().getPosition())
                            };
                        } else { // depth
                            item_position = {
                                std::get<0>(item_b.get().getPosition()),
                                std::get<1>(item_b.get().getPosition()),
                                std::get<2>(item_b.get().getPosition()) + item_b.get().getDimension()[2]
                            };
                        }
                        
                        // Fix: Use item_position instead of pos
                        if (bin.putItem(*current_item, item_position)) {
                            fitted = true;
                            break;
                        }
                    }
                }
            }
        }
        
        if (!fitted) {
            b2 = std::nullopt; // Reset b2 before trying to get a bigger bin
            while (true) {
                b2 = getBiggerBinThan(bin);
                if (!b2) break;
                
                b2->get().addItem(*current_item);
                std::vector<Item*> b2_item_ptrs;
                for (auto& ref : b2->get().getItems()) {
                    b2_item_ptrs.push_back(&ref.get());
                }
                auto left = packToBin(b2->get(), b2_item_ptrs);
                if (left.empty()) {
                    bin = b2->get();
                    fitted = true;
                    break;
                }
            }
            
            if (!fitted) {
                unpacked.push_back(current_item);
            }
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
