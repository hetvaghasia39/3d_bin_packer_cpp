#include "pallet.h"
#include <algorithm>

Pallet::Pallet(double length, double width, double thickness, double max_height, double max_weight)
    : length_(length)
    , width_(width)
    , thickness_(thickness)
    , max_height_(max_height)
    , max_weight_(max_weight)
    , current_height_(thickness)  // Initial height is just the pallet thickness
    , current_weight_(0.0)
{}

double Pallet::getLength() const { return length_; }
double Pallet::getWidth() const { return width_; }
double Pallet::getThickness() const { return thickness_; }
double Pallet::getMaxHeight() const { return max_height_; }
double Pallet::getMaxWeight() const { return max_weight_; }
double Pallet::getCurrentHeight() const { return current_height_; }
double Pallet::getCurrentWeight() const { return current_weight_; }
const std::vector<Item*>& Pallet::getItems() const { return items_; }

bool Pallet::isWithinBaseArea(const Item& item) const {
    // Check if item dimensions fit within pallet base area
    // Previously used getLength() which doesn't exist. Now using getDepth().
    return (item.getWidth() <= width_ && item.getDepth() <= length_) ||
           (item.getDepth() <= width_ && item.getWidth() <= length_);
}

bool Pallet::canAddItem(const Item& item) const {
    // Check if item fits within pallet constraints
    if (!isWithinBaseArea(item)) {
        return false;
    }
    
    // Check height constraint
    if (current_height_ + item.getHeight() > max_height_) {
        return false;
    }
    
    // Check weight constraint - use weight member directly instead of getWeight()
    if (current_weight_ + item.weight > max_weight_) {
        return false;
    }
    
    return true;
}

bool Pallet::addItem(Item* item) {
    if (!item || !canAddItem(*item)) {
        return false;
    }
    
    items_.push_back(item);
    current_weight_ += item->weight;  // Access weight directly
    current_height_ = std::max(current_height_, thickness_ + item->getHeight());
    return true;
}

void Pallet::removeItem(Item* item) {
    if (!item) return;
    
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        current_weight_ -= (*it)->weight;  // Access weight directly
        items_.erase(it);
        
        // Recalculate current height
        current_height_ = thickness_;
        for (const auto& remaining_item : items_) {
            current_height_ = std::max(current_height_, 
                thickness_ + remaining_item->getHeight());
        }
    }
}

double Pallet::getAvailableHeight() const {
    return max_height_ - current_height_;
}

double Pallet::getAvailableWeight() const {
    return max_weight_ - current_weight_;
}