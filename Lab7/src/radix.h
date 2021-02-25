/*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */
#ifndef RADIX_H
#define RADIX_H

#include <stdio.h>
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
    /*
     * Start a TCP/IP all address listener on PORT and wait for clients to send 
     * lists of unsiged integers to be sorted. When sorting, do not use more
     * that CORES cpu cores.
     */
    RadixServer(const int port, const unsigned int cores);
};

class RadixClient {
public:
    /*
     * Connect via TCP/IP to the server on HOSTNAME listening on PORT. Send
     * the unsigned integers in each list in LISTS to the server and wait
     * for the sorted lists to be returned.
     */
    void msd(const char *hostname, const int port, std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists);
};

#endif
