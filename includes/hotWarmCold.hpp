#ifndef HOTWARMCOLD_HPP
#define HOTWARMCOLD_HPP

#include "datatypes.hpp"
#include <string>
#include <vector>

// Popularity and zone management
void updatePopularityAndZone(int productID);

// Popularity decay system
void applyPopularityDecay(double currentTime);
void setDecayInterval(double intervalSeconds);
double getDecayInterval();
void resetDecayTimer();

// Zone utilities
std::string zoneToString(Zone zone);
Zone stringToZone(const std::string& str);

// Product location and analysis
int findProductPrimaryShelf(int productID);
std::vector<int> getProductsByZoneRecommendation(Zone zone);

// Reporting
void printPopularityReport();

#endif