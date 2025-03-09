#include "bin.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional> 

Bin::Bin(const std::string& name, long w, long h, long d, float max_weight, const std::string& image, const std::string& description, int id) 
    : Box(name, w, h, d), max_weight(max_weight), image(image), description(description), id(id) {
}

const std::vector<std::reference_wrapper<Item>>& Bin::getItems() const {
    return items;
}

void Bin::setItems(const std::vector<std::reference_wrapper<Item>>& items) {
    this->items = items;
}

void Bin::addItem(Item& item) {
    items.push_back(std::ref(item));
}

float Bin::scoreRotation(const Item& item, long rotationType) const {
    Item rotatedItem = item;
    rotatedItem.setRotationType(static_cast<RotationType>(rotationType));
    auto d = rotatedItem.getDimension();

    if (getWidth() < d[0] || getHeight() < d[1] || getDepth() < d[2]) {
        return 0;
    }
    float widthScore = std::pow(static_cast<float>(d[0]) / getWidth(), 2);
    float heightScore = std::pow(static_cast<float>(d[1]) / getHeight(), 2);
    float depthScore = std::pow(static_cast<float>(d[2]) / getDepth(), 2);

    float score = widthScore + heightScore + depthScore;
    return score;
}

std::vector<long> Bin::getBestRotationOrder(const Item& item) const {
    // Use map to maintain same structure as Python dictionary
    std::map<long, float> rotationScores;
    // Score all rotation types
    for (auto rotation : item.getAllowedRotations()) {
        // Explicitly cast RotationType to long for map key
        long rotationInt = static_cast<long>(rotation);
        rotationScores[rotationInt] = scoreRotation(item, rotationInt);
    }
    
    // Convert map to vector for sorting
    std::vector<long> bestRotations;
    bestRotations.reserve(rotationScores.size());
    
    // Extract keys and sort them based on their corresponding values
    for (const auto& [rotation, _] : rotationScores) {
        bestRotations.push_back(rotation);
    }
    
    std::sort(bestRotations.begin(), bestRotations.end(),
              [&rotationScores](long a, long b) {
                  return rotationScores[a] > rotationScores[b];
              });
    
    return bestRotations;
}

bool Bin::putItem(Item& item, const std::tuple<long, long, long>& p) {
    // Quick rejection checks
    const long x = std::get<0>(p);
    const long y = std::get<1>(p);
    const long z = std::get<2>(p);
    
    // CRITICAL FIX: Fast rejection for common cases
    if ((item.bottom_load_only && y != 0) || 
        x < 0 || y < 0 || z < 0 ||
        x >= getWidth() || y >= getHeight() || z >= getDepth()) {
        return false;
    }
    
    // CRITICAL FIX: Only try the single best rotation
    // This reduces the number of expensive intersection checks
    auto rotations = getBestRotationOrder(item);
    item.setPosition({x, y, z});
    item.setRotationType(static_cast<RotationType>(rotations[0]));
    auto d = item.getDimension();
    
    // Check bin boundaries
    if (getWidth() < x + d[0] || getHeight() < y + d[1] || getDepth() < z + d[2]) {
        return false; // Don't try other rotations - increase speed
    }
    
    // CRITICAL FIX: Optimized intersection checks
    for (const auto& other_ref : items) {
        const auto& other = other_ref.get();
        
        // Quick disable_stacking check
        if ((item.disable_stacking && y > std::get<1>(other.getPosition())) || 
            (other.disable_stacking && std::get<1>(other.getPosition()) > y)) {
                
            // Check for X-Z overlap
            const auto& other_pos = other.getPosition();
            const auto& other_dim = other.getDimension();
            
            bool x_overlap = (x < std::get<0>(other_pos) + other_dim[0]) && 
                            (x + d[0] > std::get<0>(other_pos));
            bool z_overlap = (z < std::get<2>(other_pos) + other_dim[2]) && 
                            (z + d[2] > std::get<2>(other_pos));
                            
            if (x_overlap && z_overlap) {
                return false;
            }
        }
        
        // CRITICAL FIX: Optimized intersection check - first check bounding boxes
        // This is much faster than full intersection check
        const auto& other_pos = other.getPosition();
        const auto& other_dim = other.getDimension();
        
        if (x + d[0] <= std::get<0>(other_pos) || 
            std::get<0>(other_pos) + other_dim[0] <= x ||
            y + d[1] <= std::get<1>(other_pos) || 
            std::get<1>(other_pos) + other_dim[1] <= y ||
            z + d[2] <= std::get<2>(other_pos) || 
            std::get<2>(other_pos) + other_dim[2] <= z) {
            // No intersection possible - continue to next item
            continue;
        }
        
        // Items might intersect, do full check
        if (other.doesIntersect(item)) {
            return false;
        }
    }

    // Apply "gravity" here directly instead of using a separate function
    if (!item.bottom_load_only && y > 0) {
        // Try placing at one level below
        std::tuple<long, long, long> lower_pos = {x, y-1, z};
        
        // Check if the item can be placed lower
        bool can_go_lower = true;
        item.setPosition(lower_pos);
        
        // Quick collision check with other items
        for (const auto& other_ref : items) {
            if (other_ref.get().doesIntersect(item)) {
                can_go_lower = false;
                break;
            }
        }
        
        // If can't go lower, reset position to original
        if (!can_go_lower) {
            item.setPosition({x, y, z});
        }
    }

    // Item fits, add it to the bin
    items.push_back(std::ref(item));
    return true;
}

// Replace putItemWithGravity with this simpler version
bool Bin::putItemWithGravity(Item& item, const std::tuple<long, long, long>& p) {
    return putItem(item, p); // We've integrated gravity into putItem
}

std::string Bin::toString() const {
    std::ostringstream oss;
    oss << "Bin: " << getName() << " (W x H x D = " << getWidth() << " x " << getHeight() << " x " << getDepth() << ")";
    return oss.str();
}
