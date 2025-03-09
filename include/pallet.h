#ifndef PALLET_H
#define PALLET_H

#include <vector>
#include <string>
#include "item.h"

class Pallet {
public:
    Pallet(double length, double width, double thickness, double max_height, double max_weight);
    
    // Core properties
    double getLength() const;
    double getWidth() const;
    double getThickness() const;
    double getMaxHeight() const;
    double getMaxWeight() const;
    
    // Dynamic properties
    double getCurrentHeight() const;
    double getCurrentWeight() const;
    const std::vector<Item*>& getItems() const;
    
    // Operations
    bool canAddItem(const Item& item) const;
    bool addItem(Item* item);
    void removeItem(Item* item);
    
    // Utility
    double getAvailableHeight() const;
    double getAvailableWeight() const;
    bool isWithinBaseArea(const Item& item) const;

private:
    double length_;
    double width_;
    double thickness_;
    double max_height_;
    double max_weight_;
    double current_height_;
    double current_weight_;
    std::vector<Item*> items_;
};

#endif // PALLET_H