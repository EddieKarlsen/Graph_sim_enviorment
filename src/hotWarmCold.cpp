#include <iostream>
#include <algorithm>
#include "../includes/hotWarmCold.hpp"
#include "../includes/datatypes.hpp"

void updatePopularityAndZone(int productID) {
    // 1. Hitta produkten
    auto it = std::find_if(products.begin(), products.end(), 
                           [productID](const Product& p) { return p.id == productID; });
    
    if (it != products.end()) {
        // 2. Öka populariteten
        it->popularity++;
        int pop = it->popularity;
        
        // 3. Logik för Hot/Warm/Cold klassificering
        Zone recommendedZone;
        if (pop >= 10) {
            recommendedZone = Zone::Hot;
        } else if (pop >= 5) {
            recommendedZone = Zone::Warm;
        } else {
            recommendedZone = Zone::Cold;
        }
        
        std::cout << "Product " << it->name << " popularity is now " << pop 
                  << " (Recommended zone: ";
        
        switch(recommendedZone) {
            case Zone::Hot: std::cout << "Hot"; break;
            case Zone::Warm: std::cout << "Warm"; break;
            case Zone::Cold: std::cout << "Cold"; break;
            default: std::cout << "Other"; break;
        }
        
        std::cout << ")\n";
    } else {
        std::cerr << "Product ID " << productID << " not found!\n";
    }
}