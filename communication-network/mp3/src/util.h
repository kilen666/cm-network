#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

typedef struct {
    int src;
    int dest;
    int cost;
} Link;

typedef struct {
    int src;
    int dest;
    std::string msg;
} Message;

void readTopology(std::string filename, std::vector<Link> &links);

void readMessages(std::string filename, std::vector<Message> &messages);

void readChanges(std::string filename, std::vector<Link> &changes);

void sendMessages(std::vector<Message> &messages, FILE *fpOut);

#endif // UTIL_H