#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "util.h"
#include "node.h"

using namespace std;

// Read the topology from the given file
void readTopology(std::string filename, std::vector<Link> &links) {
  // Open the file
  ifstream file(filename);
  if (!file.is_open()) {
    cout << "Error opening file: " << filename << endl;
    return;
  }

  // Read the links
  int src, dest, cost;
  while (file >> src >> dest >> cost) {
    Link link = {src, dest, cost};
    links.push_back(link);
  }

  // Close the file
  file.close();
}

// Read the messages from the given file
void readMessages(std::string filename, std::vector<Message> &messages) {
  // Open the file
  ifstream file(filename);
  if (!file.is_open()) {
    cout << "Error opening file: " << filename << endl;
    return;
  }

  // Read the messages
  int src, dest;
  string message;
  while (file >> src >> dest) {
    getline(file >> std::ws, message);
    Message msg = {src, dest, message};
    messages.push_back(msg);
  }

  // Close the file
  file.close();
}

// Read the changes from the given file
void readChanges(std::string filename, std::vector<Link> &changes) {
  // Open the file
  ifstream file(filename);
  if (!file.is_open()) {
    cout << "Error opening file: " << filename << endl;
    return;
  }

  // Read the changes
  int src, dest, cost;
  while (file >> src >> dest >> cost) {
    Link link = {src, dest, cost};
    changes.push_back(link);
  }

  // Close the file
  file.close();
}

// Send the messages
void sendMessages(std::vector<Message> &messages, FILE *fpOut) {
  for (Message message : messages) {
    Path path;
    Node::sendMessage(message.src, message.dest, message.msg, path);
    if (path.isSuccess()) {
      vector<int> hops = path.getHops();
      fprintf(fpOut, "from %d to %d cost %d hops ", message.src, message.dest,
              path.getCost());
      for (int hop : hops) {
        fprintf(fpOut, "%d ", hop);
      }
      fprintf(fpOut, "message %s\n", message.msg.c_str());
    } else {
      fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n",
              message.src, message.dest, message.msg.c_str());
    }
    fprintf(fpOut, "\n");
  }
}