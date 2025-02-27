/**
 * TCP over UDP Packet
 * 
 * Since UDP is a connectionless protocol, we need to implement a reliable data transfer protocol on top of it.
 * 
 * UDP packet already has a header that contains the source and destination port numbers, the length of the packet, and a checksum.
 * We will implement a simple reliable TCP-like data transfer protocol on top of UDP.
 * 
 * The header and data format of the packets are as follows:
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-------------------------------+-------------------------------+
   |                        Sequence Number                        |
   +---------------------------------------------------------------+
   |                    Acknowledgment Number                      |
   +-------+-------+-+-+-+-+-+-+-+-+-------------------------------+
   |  Data |       |C|E|U|A|P|R|S|F|                               |
   | Offset|Rsved  |W|C|R|C|S|S|Y|I|            Window             |
   |       |       |R|E|G|K|H|T|N|N|                               |
   +-------+-----------+-+-+-+-+-+-+-------------------------------+
   |                             data                              |
   +---------------------------------------------------------------+

   The header is 12 bytes long.

   The header and data are placed in the data field of the UDP packet.
 */

#include "netconfig.h"

#ifndef PKTHEADER_H
#define PKTHEADER_H

typedef unsigned int seqNum_t;

// The packet header structure  12 bytes
typedef struct {
    // Sequence number          4 bytes
    seqNum_t seqNum;
    // Acknowledgment number    4 bytes
    seqNum_t ackNum;
    // Data offset & Reserved   1 byte
    unsigned char dataOffset;
    // Flags                    1 byte
    unsigned char flags;
    // Window                   2 bytes
    unsigned short int window;
} PacketHeader;

// Sequence number functions
void setSeqNum(PacketHeader *header, seqNum_t seqNum);
seqNum_t getSeqNum(PacketHeader *header);

// Acknowledgment number functions
void setAckNum(PacketHeader *header, seqNum_t ackNum);
seqNum_t getAckNum(PacketHeader *header);

// Header offset functions
void setDataOffset(PacketHeader *header, unsigned char offset);
unsigned char getDataOffset(PacketHeader *header);

// Flag bit positions
typedef enum {
    FLAG_FIN = 0,
    FLAG_SYN = 1,
    FLAG_RST = 2,
    FLAG_PSH = 3,
    FLAG_ACK = 4,
    FLAG_URG = 5,
    FLAG_ECE = 6,
    FLAG_CWR = 7
} TCP_FLAG;

// Flag functions
void setFlag(PacketHeader *header, TCP_FLAG flag);
void clearFlag(PacketHeader *header, TCP_FLAG flag);
char getFlag(PacketHeader *header, TCP_FLAG flag);

// Window size functions
void setWindow(PacketHeader *header, unsigned short int window);
unsigned short int getWindow(PacketHeader *header);

// Header size
#define HEADER_SIZE (sizeof(PacketHeader))

// Data size
#define MAX_PAYLOAD_SIZE (MTU - UDP_HEADER_SIZE - IPV4_HEADER_SIZE)
#define MAX_DATA_SIZE (MAX_PAYLOAD_SIZE - HEADER_SIZE)
#define MSS MAX_DATA_SIZE

// The packet structure
#pragma pack(1)
typedef struct {
    PacketHeader header;    // 12   bytes
    char data[MSS];         // 1460 bytes
    // Total                   1472 bytes
} Packet;
#pragma pack()

// Packet *createPacket();

#endif /* PKTHEADER_H */
