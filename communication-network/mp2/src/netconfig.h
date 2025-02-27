#ifndef NET_CONFIG_H
#define NET_CONFIG_H

#define MTU 1500
#define UDP_HEADER_SIZE 8
#define IPV4_HEADER_SIZE 20

#define RTT 40
#define BANDWIDTH (20 * 1024 * 1024 / 8.0)

// TCP over UDP protocol constants
#define DEFAULT_SSTHRESH __INT_MAX__
#define PKT_TIMEOUT (3*RTT)
#define SYN_RETRIES __INT_MAX__
#define SYN_ACK_RETRIES __INT_MAX__
#define DATA_RETRIES __INT_MAX__
#define FIN_RETRIES 5
#define DUP_ACK_THRESHOLD 3

#endif /* NET_CONFIG_H */