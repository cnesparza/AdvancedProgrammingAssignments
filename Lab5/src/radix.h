/*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */
#ifndef _RADIX_H_
#define _RADIX_H_

#include <iostream>
#include <algorithm>
#include <vector>
#include <functional>
#include <thread>

/*
 * Simple multi-threaded Radix Sort with support for Most Significant Digit 
 * sorting only.
 */
class RadixSort {
public:
    /*
     * Create a multi-threaded RadiX Sort restricted to using no more than 
     * CORES processor cores.
     */
    RadixSort(const unsigned int cores);

    /*
     * Perform an in-place Most Significant Digit Radix Sort on each list of
     * unsigned integers in LISTS.
     */
    void msd(std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists);

};

// Function Prototypes...
static void  entryFunc(std::vector<unsigned int> &list);
static bool  compare(unsigned int val1, unsigned int val2);

#endif
