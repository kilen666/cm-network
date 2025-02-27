#include<stdio.h>
#include<string.h>
#include <cstdlib>
#include<vector>
using namespace std;
typedef struct {
    int backoff;
    int collision_counter;
    int packet_l;
}Node;
int N;
int L;
vector<int> R;
int M;
int T;
void simulateCSMA() {
    Node nodes[N];
    for (int i = 0; i < N; i++) {
        nodes[i].backoff = i % R[0];
        nodes[i].collision_counter = 0;
        nodes[i].packet_l = L;
    }
    // int currentR_index = 0;
    int channelOccupied = 0; // 0 = idle, 1 = busy
    int successfulTransmissions = 0; // Count of successful transmissions
    int collisions = 0; // Count of collisions
    int transmittingNodes = 0; // Count of nodes trying to transmit
    int transmittingNodeIndex = -1; // Track the node transmitting
    for(int tick=0;tick<T;tick++)
    {
        if(channelOccupied==1)
        {
            if(nodes[transmittingNodeIndex].packet_l==0)
            {
                nodes[transmittingNodeIndex].collision_counter=0;
                int R_index=nodes[transmittingNodeIndex].collision_counter;
                nodes[transmittingNodeIndex].backoff = (transmittingNodeIndex + tick) % R[R_index];
                nodes[transmittingNodeIndex].packet_l=L;
                if(nodes[transmittingNodeIndex].backoff ==0)
                {
                    nodes[transmittingNodeIndex].packet_l--;
                    successfulTransmissions++;
                }
                else
                {
                    channelOccupied=0;
                    for (int i = 0; i < N; i++) {
                        if (nodes[i].backoff > 0) {
                            nodes[i].backoff--;
                        }
                    }
                }
            }
            else
            {
                nodes[transmittingNodeIndex].packet_l--;
                successfulTransmissions++;
            }
            continue;
        }
        // Check each node's backoff and channel status
        transmittingNodes = 0;
        transmittingNodeIndex = -1;
        for (int i = 0; i < N; i++) {
            if (nodes[i].backoff == 0) {
                transmittingNodes++;
                transmittingNodeIndex = i;
            }
        }
        if(transmittingNodes == 1)
        {
            // Successful transmission
            successfulTransmissions++;
            channelOccupied = 1;
            nodes[transmittingNodeIndex].packet_l--;
        }
        else if(transmittingNodes > 1)
        {
            // Collision
            collisions++;
            // channelOccupied = 1;
            tick++;
            for (int i = 0; i < N; i++) {
                if (nodes[i].backoff == 0) {
                    nodes[i].collision_counter++;
                    if (nodes[i].collision_counter >= M) {
                        nodes[i].collision_counter = 0;
                        nodes[i].backoff = (i + tick) % R[0]; // 重置为最小R范围
                    } else {
                        int R_index = nodes[i].collision_counter;
                        nodes[i].backoff = (i + tick) % R[R_index];
                    }
                }
            }
            tick--;
            // // Channel idle
            // for (int i = 0; i < N; i++) {
            //     if (nodes[i].backoff > 0) {
            //         nodes[i].backoff--;
            //     }
            // }
        }
        else
        {
            for (int i = 0; i < N; i++) {
                if (nodes[i].backoff > 0) {
                    nodes[i].backoff--;
                }
            }
        }
    }
    // Calculate link utilization rate
    float utilizationRate = (float)successfulTransmissions / T;

    // Write result to output.txt
    FILE *fpOut = fopen("output.txt", "w");
    if (fpOut == NULL) {
        printf("Error opening output file\n");
        return;
    }
    fprintf(fpOut, "%.2f\n", utilizationRate);
    fclose(fpOut);

}
void read_input(char* filename) {
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        exit(1);
    }
    fscanf(fp, "N %d\n", &N);
    fscanf(fp, "L %d\n", &L);
    
    fscanf(fp, "M %d\n", &M);
    char buffer[100];
    fscanf(fp, "R %[^\n]\n", buffer);
    char* token = strtok(buffer, " ");
    while (token != NULL) {
        R.push_back(atoi(token));
        token = strtok(NULL, " ");
    }
    fscanf(fp, "T %d\n", &T);


    printf("N: %d\n", N);
    printf("L: %d\n", L);
    printf("R: ");
    for (int i = 0; i < R.size(); i++) {
        printf("%d ", R[i]);
    }
    printf("\nM: %d\n", M);
    printf("T: %d\n", T);
    fclose(fp);

}
int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }
    read_input(argv[1]);
    // Simulate CSMA protocol
    simulateCSMA();
    

    return 0;
}

