#ifndef INITSIM_HPP
#define INITSIM_HPP

#include "datatypes.hpp"

void assignProductToSlot(Shelf& shelf, int slotIndex, int productID, int capacity, int occupied);
void initGraphLayout();
void resetInventory();
void initProducts();

#endif