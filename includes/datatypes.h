#ifndef DATATYPES_H
#define DATATYPES_H

#define MAX_SLOTS 10
#define MAX_NAME_LENGTH 32
#define MAX_DELIVERIES 30



enum Lorry {
    BIG_LORRY = 30,      
    MEDIUM_LORRY = 20,   
    SMALL_LORRY = 10     
};

enum NodeType {
    NODE_SHELF,
    NODE_LOADING_DOCK
};

enum ItemType {
    ITEM_SODA,
    ITEM_CANDY,
    ITEM_COSMETICS,
    ITEM_CLOTHES,
    ITEM_EMPTY
};




struct Slot {
    int occupied;       
    char itemName[32];  
    int quantity;     
};

struct Shelf {
    char name[16];
    struct Slot slots[MAX_SLOTS];
    int slotCount;
};

struct LoadingDock {
    char name[16];
    int isOccupied;
    struct Item deliveries[MAX_DELIVERIES];
    int deliveryCount;
    enum Lorry currentLorry;
};

struct Node
{
    int id;
    enum NodeType type;

        union {
        struct Shelf shelf;
        struct LoadingDock dock;
    } data;
};

struct Item
{
    char name[32];
    int quantity;
};

struct Edge
{
    int fromNodeId;
    int toNodeId;
    int id;
    int capacity;
    int directed;
};

#endif