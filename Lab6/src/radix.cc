 /*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */

#include "radix.h"

// === compare =====================================================
//
// =================================================================
static bool compare(const unsigned int a, const unsigned int b)
{
  return std::to_string(a) < std::to_string(b);
}// End of "compare"

// === Entry Function ==============================================
//
// =================================================================
void ParallelRadixSort::entryFunc(std::vector<unsigned int> &list)
{
    std::sort(list.begin(), list.end(), compare);
}// End of "entryFunc"

// === Bucket Sort =================================================
// https://www.dyclassroom.com/sorting-algorithm/bucket-sort
// =================================================================
void ParallelRadixSort::bucketSort(std::vector<unsigned int> &list, const unsigned int size, unsigned int cores)
{
  std::vector<std::thread> threads;
  std::vector<unsigned int> buckets[90];
  unsigned int numBuckets = 90;
  unsigned int i, j, k;
  unsigned int dig1, dig2;
  
  //insert elements within buckets
  threads.push_back(std::thread([&] {
	for(i = 0; i < size; i++) {
	  if(list[i] == 0) {
	    buckets[0].insert(buckets[0].begin(),0);
	  }
	  else {
	    std::string    temp = std::to_string(list[i]);
	    dig1 = temp[0] - '1';
	    if (temp.length() > 1)
	      dig2 = temp[1] - '0';
	    else
	      dig2 = 0;
	    buckets[dig1 * 10 + dig2].push_back(list[i]);
	  }
	}
      }));
  for(auto &th : threads) {
    th.join();
  }
  threads.clear();
  

  //sort elements in each bucket
  for(i = 0; i < numBuckets; i++) {
    threads.push_back(std::thread(entryFunc,ref(buckets[i])));
    if(threads.size() == cores) {
      for(std::thread & th : threads)
	th.join();
      threads.clear();
    }
  }

  //While at least one thread is still running
  for(std::thread & th : threads) {
    th.join();
  }
  
  //append back the elements from buckets
  k = 0;
  for(i = 0; i < numBuckets; i++) {
    for(j = 0; j < buckets[i].size(); j++) {
      list[k++] = buckets[i][j];
    }
  }
  
}// End of "bucketSort"

// === MSD =========================================================
//
// =================================================================
void ParallelRadixSort::msd(std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists, unsigned int cores)
{ 
  for(auto list : lists)
    {
      bucketSort(list, list.get().size(), cores);
    }
}// End of "MSD"
