/*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */
// vector of threads and usage found:
// https://thispointer.com/c11-how-to-create-vector-of-thread-objects/
#include "radix.h"

static unsigned int m_cores;

RadixSort::RadixSort(const unsigned int cores) { m_cores = cores; }

void RadixSort::msd(std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists) { 
  std::vector<std::thread>  vThreads;
  unsigned int uC = 0;
  for(unsigned int c = 0; c < lists.size(); c++, uC++) {
        if(uC < m_cores)
      vThreads.push_back(std::thread(entryFunc,std::ref(lists.at(c).get())));
	else {
	  for(std::thread & th : vThreads) {
	    if(th.joinable())
	      th.join();
	  }
	  vThreads.push_back(std::thread(entryFunc,std::ref(lists.at(c).get())));
	  uC = 0;
	}
    //entryFunc(lists.at(c).get());
  }

  for(std::thread & th : vThreads) {
    if(th.joinable())
      th.join();
  }

}// End of "msd"



// === compare ================================================================
//
// ============================================================================
bool  compare(unsigned int val1, unsigned int val2) {
  std::string    str1 = std::to_string(val1);
  std::string    str2 = std::to_string(val2);

  return (str1.compare(str2) < 0);
  
}// End of "compare"



// === entryFunc ==============================================================
//
// ============================================================================
void  entryFunc(std::vector<unsigned int> &list) {
  
  std::sort(list.begin(), list.end(), compare);
  
}// End of "entryFunc"
