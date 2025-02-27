#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<vector>
#include <queue>
#include <set>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <limits>
using namespace std;
const int INF = numeric_limits<int>::max();
class edge{
    public:
        int node;
        int cost;
    edge(){
        node = -1;
        cost = -1;
    }
    edge(int node, int cost){
        this->node = node;
        this->cost = cost;
    }
    
};

void read_topology(char* filename, map<int, vector<edge>> &edges){
    ifstream file(filename);
    if(!file.is_open()){
        printf("file not open\n");
    }
    int source_id, dest_id, cost;
    while (file >> source_id >> dest_id >> cost) {  
        edge e;
        e.node = dest_id;
        e.cost = cost;
        edges[source_id].push_back(e);
        e.node = source_id;
        edges[dest_id].push_back(e);
    }

}
vector<tuple<int, int, string>> read_message(string filename){
    ifstream file(filename);
    int source_id, dest_id;
    string message;
    vector<tuple<int, int, string>> messages;
    while (file >> source_id >> dest_id) {
        getline(file, message);
        messages.push_back(make_tuple(source_id, dest_id, message));
    }
    return messages;
}
vector<tuple<int, int, int>> read_changes(string filename){
    ifstream file(filename);
    int source_id, dest_id, cost;
    vector<tuple<int, int, int>> changes;
    while (file >> source_id >> dest_id >> cost) {
        changes.push_back(make_tuple(source_id, dest_id, cost));
    }
    return changes;
}
map<int, pair<int,int>>djikstra(map<int, vector<edge>> edges, int source){
    map<int, pair<int,int>> forward_table;
    map<int, int> predecessor;
    map<int, int> distance;
    set<int> visited;
    set<pair<int, int>> pq;
    for(auto &edge: edges){
        distance[edge.first] = INF;
        predecessor[edge.first] = -1;
    }
    distance[source] = 0;
    pq.insert(make_pair(0, source));
    while (!pq.empty()) {
        pair<int, int> top = *pq.begin();
        pq.erase(pq.begin());
        int currentDistance = top.first;
        int currentNode = top.second;
        visited.insert(currentNode);
        for(auto &edge: edges[currentNode]){
            int neighbor = edge.node;
            int newdistacne = currentDistance + edge.cost;
            if(visited.find(neighbor)==visited.end())
            {
                if (newdistacne < distance[neighbor]) {
                    pq.erase(make_pair(distance[neighbor], neighbor));
                    distance[neighbor] = newdistacne;
                    predecessor[neighbor] = currentNode;
                    pq.insert(make_pair(newdistacne, neighbor));
                }
                else if(newdistacne == distance[neighbor]){
                    if(currentNode < predecessor[neighbor]){
                        predecessor[neighbor] = currentNode;
                    }
                }
            }
        }
    }
    for(auto &node: distance){
        if(node.second == INF){
            continue;
        }
        forward_table[node.first] = make_pair(predecessor[node.first], node.second);
        // printf("%d %d %d\n", node.first, predecessor[node.first], node.second);
    }
    return forward_table;
}
void print_topology_entries(int source,map<int, pair<int,int>> forward_table, FILE* fpOut){
    for(auto &entry: forward_table){
        // fprintf(fpOut, "Node %d: ", entry.first);
        int nexthop = entry.first;
        if(nexthop==source)
        {
            fprintf(fpOut,"%d %d %d\n",entry.first,nexthop,0);
            continue;
        }
        while(forward_table[nexthop].first != source && forward_table[nexthop].first != -1){
            nexthop = forward_table[nexthop].first;
        }
        if(entry.second.first == -1){
            continue;
        }
        else{
            fprintf(fpOut, "%d %d %d\n", entry.first, nexthop, entry.second.second);
        }
    }
}
void print_Message(int source, int destination, string msg, map<int, pair<int,int>> forward_table, FILE* fpOut){
    vector<int> path;
    int current = destination;
    if(forward_table.find(destination)==forward_table.end())
    {
        fprintf(fpOut, "from %d to %d cost infinite hops unreachable message%s\n", source, destination,msg.c_str());
        // fprintf(fpOut,"here is a message from %d to %d\n", source, destination);
        return;
    }
    int cost=forward_table[current].second;
    while(current != -1){
        path.push_back(current);
        current = forward_table[current].first;
    }
    reverse(path.begin(), path.end());
    // Output the path to a file
    if (path.front() == source) {
        fprintf(fpOut, "from %d to %d cost %d hops ", source, destination, cost);
        for (size_t i = 0; i < path.size()-1; ++i) {
            fprintf(fpOut, "%d ", path[i]);
        }
        fprintf(fpOut,"message%s\n", msg.c_str());
    } 
    // else {
    //     fprintf(fpOut, "from %d to %d cost infinite hops unreachable message ", source, destination);
    //     fprintf(fpOut,"here is a message from %d to %d\n", source, destination);
    // }
    
}
void print_all_topoly(map<int, vector<edge>> &edges, vector<map<int, pair<int,int>>> &forward_tables, FILE* fpOut)
{
    for(auto &edge:edges)
    {
        int source=edge.first;
        map<int, pair<int,int>> forward_table = djikstra(edges, source);
        forward_tables[source]=(forward_table);
        // fprintf( fpOut,"this is entryise of Node %d\n", source);
        print_topology_entries(source, forward_table, fpOut);
    }
}
void print_all_message_need(map<int, vector<edge>> &edges, vector<map<int, pair<int,int>>> &forward_tables,vector<tuple<int, int, string>> messages, FILE* fpOut)
{
    for(auto &message:messages)
    {
        int source= get<0>(message);
        int destination= get<1>(message);
        string msg= get<2>(message);
        if(edges.find(source)==edges.end()||edges.find(destination)==edges.end())
        {
            fprintf(fpOut, "from %d to %d cost infinite hops unreachable message", source, destination);
            fprintf(fpOut,"%s\n", msg.c_str());
            continue;
        }
        print_Message(source,destination, msg, forward_tables[source],fpOut);
    }
}
void change_edge(tuple<int, int, int>& change, map<int, vector<edge>>& edges) {
    int source = get<0>(change);
    int destination = get<1>(change);
    int cost = get<2>(change);


    bool found = false;
    for (auto it = edges[source].begin(); it != edges[source].end(); ++it) {
        if (it->node == destination) {
            found = true;
            if (cost == -999) {
                edges[source].erase(it);  
            } else {
                it->cost = cost;  
            }
            break;
        }
    }
    // If the edge does not exist and cost is not -999, add a new edge.
    if (!found && cost != -999) {
        edges[source].push_back(edge(destination, cost));
    }

    // Destination modification -> source side
    found = false;
    for (auto it = edges[destination].begin(); it != edges[destination].end(); ++it) {
        if (it->node == source) {
            found = true;
            if (cost == -999) {
                edges[destination].erase(it);  
            } else {
                it->cost = cost;  
            }
            break;
        }
    }
    if (!found && cost != -999) {
        edges[destination].push_back(edge(source, cost));
    }
}

int main(int argc, char** argv) {

    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }
    char* topofile = argv[1];
    string messagefile = argv[2];
    string changesfile = argv[3];
    map<int, vector<edge>> edges;
    read_topology(topofile, edges);
    vector<tuple<int, int, string>> messages = read_message(messagefile);
    vector<tuple<int, int, int>> changes = read_changes(changesfile);
    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    if (fpOut == NULL) {
        cerr << "Error opening output file." << endl;
        return -1;
    }
    vector<map<int, pair<int,int>>>forward_tables(100);
    print_all_topoly(edges, forward_tables,fpOut);// intial topology entry
    print_all_message_need(edges, forward_tables,messages,fpOut);
    for(auto &change:changes)
    {
        forward_tables.clear();
        forward_tables.resize(100);
        change_edge(change,edges);
        print_all_topoly(edges, forward_tables,fpOut);
        print_all_message_need(edges, forward_tables,messages,fpOut);
    }


    fclose(fpOut);

    return 0;
}

