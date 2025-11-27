#include <iostream>
#include <algorithm>
#include <cmath>
#include "../includes/hotWarmCold.hpp"
#include "../includes/datatypes.hpp"

// Decay configuration
const double DECAY_RATE = 0.95;  // 5% decay per interval
const double MIN_POPULARITY = 0.0;
const double POPULARITY_INCREMENT = 1.0;

// Time tracking for decay
static double lastDecayTime = 0.0;
static double decayInterval = 600.0;  // Apply decay every 10 minutes (600 seconds)

void updatePopularityAndZone(int productID) {
    // 1. Hitta produkten
    auto it = std::find_if(products.begin(), products.end(), 
                           [productID](const Product& p) { return p.getId() == productID; });
    
    if (it != products.end()) {
        // 2. Öka populariteten
        int currentPop = it->getPopularity();
        it->setPopularity(currentPop + static_cast<int>(POPULARITY_INCREMENT));
        int newPop = it->getPopularity();
        
        // 3. Logik för Hot/Warm/Cold klassificering
        Zone recommendedZone;
        if (newPop >= 10) {
            recommendedZone = Zone::Hot;
        } else if (newPop >= 5) {
            recommendedZone = Zone::Warm;
        } else {
            recommendedZone = Zone::Cold;
        }
        
        std::cerr << "[POPULARITY] Product " << it->getName() << " popularity: " 
                  << currentPop << " -> " << newPop 
                  << " (Recommended zone: ";
        
        switch(recommendedZone) {
            case Zone::Hot: std::cerr << "Hot"; break;
            case Zone::Warm: std::cerr << "Warm"; break;
            case Zone::Cold: std::cerr << "Cold"; break;
            default: std::cerr << "Other"; break;
        }
        
        std::cerr << ")\n";
        
        // 4. Check if product should be moved
        int productLocation = findProductPrimaryShelf(productID);
        if (productLocation >= 0 && productLocation < static_cast<int>(nodes.size())) {
            Zone currentZone = nodes[productLocation].getZone();
            if (currentZone != recommendedZone && recommendedZone != Zone::Other) {
                std::cerr << "[ZONE] ⚠️  Product " << it->getName() 
                          << " should be moved from " << zoneToString(currentZone)
                          << " to " << zoneToString(recommendedZone) << " zone!\n";
            }
        }
    } else {
        std::cerr << "[POPULARITY] Product ID " << productID << " not found!\n";
    }
}

void applyPopularityDecay(double currentTime) {
    // Check if enough time has passed since last decay
    if (currentTime - lastDecayTime < decayInterval) {
        return;
    }
    
    lastDecayTime = currentTime;
    
    std::cerr << "[DECAY] Applying popularity decay (rate: " 
              << (1.0 - DECAY_RATE) * 100 << "%)\n";
    
    int productsDecayed = 0;
    
    for (auto& product : products) {
        int oldPop = product.getPopularity();
        
        if (oldPop > 0) {
            // Apply exponential decay
            double newPopFloat = oldPop * DECAY_RATE;
            int newPop = std::max(static_cast<int>(MIN_POPULARITY), 
                                 static_cast<int>(std::floor(newPopFloat)));
            
            if (newPop != oldPop) {
                product.setPopularity(newPop);
                productsDecayed++;
                
                std::cerr << "[DECAY]   " << product.getName() 
                          << ": " << oldPop << " -> " << newPop << "\n";
            }
        }
    }
    
    if (productsDecayed > 0) {
        std::cerr << "[DECAY] Decayed " << productsDecayed << " products\n";
    } else {
        std::cerr << "[DECAY] No products needed decay\n";
    }
}

void setDecayInterval(double intervalSeconds) {
    decayInterval = intervalSeconds;
    std::cerr << "[DECAY] Decay interval set to " << intervalSeconds << " seconds\n";
}

double getDecayInterval() {
    return decayInterval;
}

void resetDecayTimer() {
    lastDecayTime = 0.0;
}

int findProductPrimaryShelf(int productID) {
    int maxQuantity = 0;
    int primaryShelf = -1;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].getType() != NodeType::Shelf) continue;
        
        const Shelf* shelf = nodes[i].getShelf();
        if (!shelf) continue;
        
        for (int j = 0; j < shelf->getSlotCount(); ++j) {
            Slot slot = shelf->getSlot(j);
            if (slot.getProductID() == productID) {
                int quantity = slot.getOccupied();
                if (quantity > maxQuantity) {
                    maxQuantity = quantity;
                    primaryShelf = i;
                }
            }
        }
    }
    
    return primaryShelf;
}

std::string zoneToString(Zone zone) {
    switch(zone) {
        case Zone::Hot: return "Hot";
        case Zone::Warm: return "Warm";
        case Zone::Cold: return "Cold";
        case Zone::Other: return "Other";
        default: return "Unknown";
    }
}

Zone stringToZone(const std::string& str) {
    if (str == "Hot") return Zone::Hot;
    if (str == "Warm") return Zone::Warm;
    if (str == "Cold") return Zone::Cold;
    if (str == "Other") return Zone::Other;
    return Zone::Other;
}

std::vector<int> getProductsByZoneRecommendation(Zone zone) {
    std::vector<int> productIDs;
    
    for (const Product& p : products) {
        int pop = p.getPopularity();
        Zone recommendedZone;
        
        if (pop >= 10) {
            recommendedZone = Zone::Hot;
        } else if (pop >= 5) {
            recommendedZone = Zone::Warm;
        } else {
            recommendedZone = Zone::Cold;
        }
        
        if (recommendedZone == zone) {
            productIDs.push_back(p.getId());
        }
    }
    
    return productIDs;
}

void printPopularityReport() {
    std::cerr << "\n=== Popularity Report ===\n";
    
    // Sort products by popularity
    std::vector<Product> sortedProducts = products;
    std::sort(sortedProducts.begin(), sortedProducts.end(),
              [](const Product& a, const Product& b) {
                  return a.getPopularity() > b.getPopularity();
              });
    
    std::cerr << "Top Products:\n";
    int count = std::min(10, static_cast<int>(sortedProducts.size()));
    for (int i = 0; i < count; ++i) {
        const Product& p = sortedProducts[i];
        if (p.getPopularity() == 0) break;
        
        Zone recommendedZone;
        int pop = p.getPopularity();
        if (pop >= 10) recommendedZone = Zone::Hot;
        else if (pop >= 5) recommendedZone = Zone::Warm;
        else recommendedZone = Zone::Cold;
        
        std::cerr << "  " << (i+1) << ". " << p.getName() 
                  << " (pop: " << pop << ", zone: " 
                  << zoneToString(recommendedZone) << ")\n";
    }
    
    std::cerr << "\nZone Distribution:\n";
    std::cerr << "  Hot zone:  " << getProductsByZoneRecommendation(Zone::Hot).size() << " products\n";
    std::cerr << "  Warm zone: " << getProductsByZoneRecommendation(Zone::Warm).size() << " products\n";
    std::cerr << "  Cold zone: " << getProductsByZoneRecommendation(Zone::Cold).size() << " products\n";
    std::cerr << "=========================\n\n";
}