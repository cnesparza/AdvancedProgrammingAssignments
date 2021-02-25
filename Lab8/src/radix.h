/*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */

#ifndef RADIX_H
#define RADIX_H

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <functional>
#include <algorithm>
#include <thread>
#include <string.h>

#define MAX_VALUES 128

#define NONE    0 // Default flag for on-wire datagram
#define LAST    1 // Identifies last datagram in batch
#define RESEND  2 // Request for a re-send of missing batches
                  // If set, VALUES contains the sequence numbers of missing batches

/*
 * On-wire datagram. 
 *
 * All elements should be converted to network byte-order before transmission
 * and converted back to host byte-order on recepit.
 */
typedef struct message_t {
    unsigned int values[MAX_VALUES]; // Sorted or to-be sorted numbers if FLAG is NONE or LAST
                                     // Missing sequence numbers if FLAG is RESEND
    unsigned int num_values; // Number of valid entries in VALUES
    unsigned int sequence;   // Unique sequence number of trasmission batch, starting at zero
    unsigned int flag;       // One of NONE, LAST, RESEND
}
Message;

/*
 * Parallel Radix Sort with support for Most Significant Digit sorting only.
 *
 * If allowed to use ten cores or more, should sort a large set of randomly
 * selected unsigned integers apromixmately ten times faster than a single
 * threaded sort of the same random set.
 *
 * Whilst the choice of sorting algorithnm is a matter or personal choice,
 * a bucket sort where each bucket is sorted in a different thread will almost 
 * certainly deliver the best results.
 */
class ParallelRadixSort {
public:
    /*
     * Perform an in-place Most Significant Digit Radix Sort on each list of
     * unsigned integers in LISTS using nore that CORES cpu cores.
     */
    void msd(std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists, const unsigned int cores);

    static void bucketSort(std::vector<unsigned int> &list, const unsigned int size, unsigned int cores);
    static void entryFunc(std::vector<unsigned int> &list);
};


class RadixServer {
public:
    void start(const int port);
};

class RadixClient {
public:
    void msd(const char *hostname, const int port, std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists);
};

//Prototypes for functions used....

static void error(const char *msg1, const char *msg2);
static void sendList( int &sockfd,  struct sockaddr *rAddr, std::vector<Message> &datagrams);
static void fillMessage(std::vector<unsigned int> &list, std::vector<Message> &datagrams);
static int recvList( int &sockfd,  struct sockaddr *rAddr, std::vector<Message> &dataG);
static void fillList(std::vector<unsigned int> &list, std::vector<Message> &datagrams);
static void resend(int &sockfd, struct sockaddr *rAddr, std::vector<Message> &datagrams, const unsigned int numPackets);
static bool compareMsg(const Message a, const Message b);
#endif


