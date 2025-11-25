#include "../includes/datatypes.hpp"

std::vector<Node> nodes;
std::vector<std::vector<Edge>> adj;
std::vector<Product> products;

int loadingDockNode = -1;
int shelfANode = -1;
int shelfBNode = -1;
int shelfCNode = -1;
int shelfDNode = -1;
int shelfENode = -1;
int shelfFNode = -1;
int shelfGNode = -1;
int shelfHNode = -1;
int shelfINode = -1;
int shelfJNode = -1;
int chargingStationNode = -1;
int frontDeskNode = -1;

// Implementation of DataAccess namespace
namespace DataAccess {
    int getNodeCount() {
        return static_cast<int>(nodes.size());
    }
    
    Node* getNode(int index) {
        if (index >= 0 && index < static_cast<int>(nodes.size())) {
            return &nodes[index];
        }
        return nullptr;
    }
    
    const Node* getNodeConst(int index) {
        if (index >= 0 && index < static_cast<int>(nodes.size())) {
            return &nodes[index];
        }
        return nullptr;
    }
    
    int getProductCount() {
        return static_cast<int>(products.size());
    }
    
    Product* getProduct(int index) {
        if (index >= 0 && index < static_cast<int>(products.size())) {
            return &products[index];
        }
        return nullptr;
    }
    
    const Product* getProductConst(int index) {
        if (index >= 0 && index < static_cast<int>(products.size())) {
            return &products[index];
        }
        return nullptr;
    }
    
    int getAdjListSize(int nodeIndex) {
        if (nodeIndex >= 0 && nodeIndex < static_cast<int>(adj.size())) {
            return static_cast<int>(adj[nodeIndex].size());
        }
        return 0;
    }
    
    Edge getEdge(int nodeIndex, int edgeIndex) {
        if (nodeIndex >= 0 && nodeIndex < static_cast<int>(adj.size()) &&
            edgeIndex >= 0 && edgeIndex < static_cast<int>(adj[nodeIndex].size())) {
            return adj[nodeIndex][edgeIndex];
        }
        return Edge{-1, false, 0.0};
    }
    
    int getLoadingDockNode() {
        return loadingDockNode;
    }
    
    int getShelfNode(char shelfLetter) {
        switch (shelfLetter) {
            case 'A': return shelfANode;
            case 'B': return shelfBNode;
            case 'C': return shelfCNode;
            case 'D': return shelfDNode;
            case 'E': return shelfENode;
            case 'F': return shelfFNode;
            case 'G': return shelfGNode;
            case 'H': return shelfHNode;
            case 'I': return shelfINode;
            case 'J': return shelfJNode;
            default: return -1;
        }
    }
    
    int getChargingStationNode() {
        return chargingStationNode;
    }
    
    int getFrontDeskNode() {
        return frontDeskNode;
    }
}