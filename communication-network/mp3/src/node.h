#include <algorithm>
#include <cstdio>
#include <ostream>
#include <string>
#include <vector>

using namespace std;

#ifndef PATH_H
#define PATH_H

class Path {
public:
  // Create a path
  Path() {
    hops = vector<int>();
    cost = 0;
    success = false;
  }

  // Add a hop to the path
  void addHop(int node, int cost) {
    hops.push_back(node);
    this->cost += cost;
  }

  // Get the cost of the path
  int getCost() { return cost; }

  // Get the hops in the path
  vector<int> &getHops() { return hops; }

  // Get the last hop in the path
  int getLastHop() { return hops.back(); }

  bool isSuccess() { return success; }

  void reachDest() { this->success = true; }

  // Clear the path
  void clear() {
    hops.clear();
    cost = 0;
    success = false;
  }

private:
  // The hops in the path
  vector<int> hops;

  // The cost of the path
  int cost;

  // The success of the path
  bool success;
};

#endif // PATH_H

#ifndef NODE_H
#define NODE_H

class Node {
public:
  // Complete the initialization of the network
  static void completeInit() {
    initialized = true;
    refreshRouteTables();
  };

  // Print the route tables of all nodes
  static void printRouteTables(FILE *fpOut) {
    for (Node *node : nodes) {
      if (node != nullptr) {
        node->printRouteTable(fpOut);
        fprintf(fpOut, "\n");
      }
    }
  }

  // Print the route table of a node to a stream
  void printRouteTable(FILE *fpOut) {
    for (int dest = 0; dest < routeTable.size(); dest++) {
      int nextHop = routeTable[dest];
      if (nextHop >= 0) {
        fprintf(fpOut, "%d %d %d\n", dest, nextHop, getPathCost(dest));
      }
    }
  }

  // Send a message from one node to another
  static void sendMessage(int src, int dest, string &message, Path &path) {
    path.clear();
    nodes[src]->sendMessage(dest, message, path);
  }

  // Refresh the route tables of all nodes
  static void refreshRouteTables() {
    for (Node *node : nodes) {
      if (node != nullptr) {
        node->refreshRouteTable();
      }
    }
  }

protected:
  // Hide constructors and destructors
  Node(){};
  Node(int index) {
    // Set the index of the node
    this->index = index;
    // Add the node to the list of nodes
    if (index >= nodes.size()) {
      nodes.resize(index + 1);
    }
    nodes[index] = this;
    // Initialize the route table
    this->routeTable = vector<int>(nodes.size(), -1);
    this->routeTable[index] = index;
  }
  virtual ~Node() {
    // Remove the node from the list of nodes
    nodes[index] = nullptr;
  };

  // The nodes in the network
  inline static vector<Node *> nodes = vector<Node *>(100);

  // Init state
  inline static bool initialized = false;

  // Send a message from this node to another
  void sendMessage(int dest, string message, Path &path) {
    // If the destination is this node, add the path to the list of paths
    if (dest == index) {
      path.reachDest();
      return;
    }

    // Find the next hop
    int nextHop = routeTable[dest];
    if (nextHop < 0) {
      return;
    }

    // Send the message to the next hop
    path.addHop(index, getLinkCost(nextHop));
    nodes[nextHop]->sendMessage(dest, message, path);
  }

  // Update the link between this node and another
  virtual void updateLink(int dest, int cost) = 0;

  // Refresh the route table of this node
  virtual void refreshRouteTable() = 0;

  // Get the cost of the path between this node and another
  virtual int getPathCost(int dest) = 0;

  // Get the cost of the link between this node and another
  virtual int getLinkCost(int nextHop) = 0;

  // The index of the node
  int index;

  // The route table of the node
  vector<int> routeTable;
};

#endif // NODE_H