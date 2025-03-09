#pragma once

#include <vector>
#include <optional>
#include <functional>  // Include for std::reference_wrapper
#include "bin.h"
#include "item.h"

class Packer {
public:
    Packer();
    
    const std::vector<Bin>& getBins() const;
    const std::vector<Item>& getItems() const;
    const std::vector<Item>& getUnfitItems() const;

    void addBin(const Bin& bin);
    void addItem(const Item& item);
    std::optional<std::reference_wrapper<Bin>> findFittedBin(Item& item);
    std::optional<std::reference_wrapper<Bin>> getBiggerBinThan(const Bin& other_bin);
    void unfitItem(std::vector<Item*>& item_ptrs);
    std::vector<Item*> packToBin(Bin& bin, std::vector<Item*>& item_ptrs);
    void pack();
    std::vector<Item> items;
    std::vector<Bin> bins;
    std::vector<Item> unfit_items;

private:
};
