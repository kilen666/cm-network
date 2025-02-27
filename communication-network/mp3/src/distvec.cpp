#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "node.h"
#include "util.h"

using namespace std;

#define MAX_COST 128

class DVNode : Node {
public:
  DVNode(int index) : Node(index) {
    // Initialize the distance vector
    distVec = vector<int>(nodes.size(), -1);
    distVec[index] = 0;

    // Initialize the cost table
    costTable =
        vector<vector<int>>(nodes.size(), vector<int>(nodes.size(), -1));
    costTable[index][index] = 0;
  }

  static void updateLink(int src, int dest, int cost) {
    if (nodes.size() <= max(src, dest)) {
      nodes.resize(max(src, dest) + 1);
    }

    if (nodes[src] == nullptr) {
      nodes[src] = new DVNode(src);
    }

    if (nodes[dest] == nullptr) {
      nodes[dest] = new DVNode(dest);
    }

    getNode(src)->updateLink(dest, cost);
    getNode(dest)->updateLink(src, cost);
  }

  // Get the node at the given index
  static DVNode *getNode(int index) {
    if (index < 0 || index >= nodes.size()) {
      return nullptr;
    }
    return (DVNode *)nodes[index];
  }

protected:
  void updateLink(int dest, int cost) override {
    // Update the cost table
    vector<int> &neighbors = costTable[index];
    updateSize(dest + 1);
    neighbors[dest] = cost;

    // Recalculate the distance vector
    if (initialized) {
      refreshRouteTable();
    }
  }

  // Get the cost of the path between two nodes
  int getPathCost(int dest) override { return distVec[dest]; }

  // Get the cost of the link between this node and another
  int getLinkCost(int nextHop) override { return costTable[index][nextHop]; }

  void refreshRouteTable() override {
    // Update the distance vector and route table
    bool updated = false;
    // Traverse all destinations
    for (int dest = 0; dest < distVec.size(); dest++) {
      if (dest != index) {
        // Find the minimum cost and next hop
        int minCost = -1;
        int minHop = -1;
        for (int hop = 0; hop < costTable.size(); hop++) {
          int costNodeToHop = costTable[index][hop];
          int costHopToDest = hop == dest ? 0 : costTable[hop][dest];
          if (hop != index && costNodeToHop >= 0 && costHopToDest >= 0) {
            int cost = costNodeToHop + costHopToDest;
            // If the cost is too large, set it to -1 (infinite)
            // Prevent the formation of loops
            cost = cost > MAX_COST ? -1 : cost;
            // Update the minimum cost and next hop
            // Tie-breaking: choose the smaller hop
            if (minCost < 0 || cost < minCost) {
              minCost = cost;
              minHop = hop;
            }
          }
        }

        // Update the distance vector
        if (minCost != distVec[dest]) {
          distVec[dest] = minCost;
          updated = true;
        }

        // Update the route table
        // If the cost is too large, set it to -1
        routeTable[dest] = minCost >= 0 ? minHop : -1;
      }
    }

    // Broadcast the distance vector if updated
    if (updated) {
      broadcastDistVec();
    }
  }

private:
  // The cost table of the node
  vector<vector<int>> costTable;

  // Distance vector
  vector<int> distVec;

  // Broadcast the distance vector to all neighbors
  void broadcastDistVec() {
    vector<int> &neighbors = costTable[index];
    for (int i = 0; i < neighbors.size(); i++) {
      if (neighbors[i] >= 0 && i != index) {
        // Poison reverse
        vector<int> distVec = this->distVec;
        for (int j = 0; j < min(distVec.size(), routeTable.size()); j++) {
          if (routeTable[j] == i) {
            distVec[j] = -1;
          }
        }

        // Send the distance vector to the neighbor
        getNode(i)->receiveDistVec(index, distVec);
      }
    }
  }

  // Update cost table based on the received distance vector
  void receiveDistVec(int src, vector<int> &distVec) {
    updateSize(max<int>(src + 1, distVec.size()));
    vector<int> &thisDistVec = costTable[src];
    for (int i = 0; i < distVec.size(); i++) {
      thisDistVec[i] = distVec[i];
    }
    // Update the distance vector
    refreshRouteTable();
  }

  // Update size of the data structures
  void updateSize(int size) {
    if (distVec.size() < size) {
      distVec.resize(size, -1);
    }
    for (int i = 0; i < costTable.size(); i++) {
      if (costTable[i].size() < size) {
        costTable[i].resize(size, -1);
      }
    }
    if (costTable.size() < size) {
      costTable.resize(size, vector<int>(size, -1));
    }
    if (routeTable.size() < size) {
      routeTable.resize(size, -1);
    }
  }
};

int main(int argc, char **argv) {
  // printf("Number of arguments: %d", argc);
  if (argc != 4) {
    printf("Usage: ./distvec topofile messagefile changesfile\n");
    return -1;
  }

  // Open the output file
  FILE *fpOut;
  fpOut = fopen("output.txt", "w");

  // Read the topology
  vector<Link> links;
  readTopology(argv[1], links);

  // Read the messages
  vector<Message> messages;
  readMessages(argv[2], messages);

  // Read the changes
  vector<Link> changes;
  readChanges(argv[3], changes);

  // Initialize the network
  for (Link link : links) {
    DVNode::updateLink(link.src, link.dest, link.cost);
  }
  Node::completeInit();

  // Print the initial route tables
  Node::printRouteTables(fpOut);

  // Send the messages
  sendMessages(messages, fpOut);

  // Update the network
  for (Link change : changes) {
    // Update a link
    DVNode::updateLink(change.src, change.dest, change.cost);
    // Print the route tables
    Node::printRouteTables(fpOut);
    // Send the messages
    sendMessages(messages, fpOut);
  }

  fclose(fpOut);

  return 0;
}
