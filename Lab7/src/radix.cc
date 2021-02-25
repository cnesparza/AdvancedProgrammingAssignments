/*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */

#include "radix.h"

// +++ PARALLEL RADIX SORT FUNCTIONS +++++++++++++++++++++++++++++++

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

// === PRS::MSD ====================================================
//
// =================================================================
void ParallelRadixSort::msd(std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists, const unsigned int cores) { 
    for(auto list : lists)
    {
      bucketSort(list, list.get().size(), cores);
    }
}// End of "ParallelRadixSort::msd"



// +++ RADIX SERVER FUNCTIONS ++++++++++++++++++++++++++++++++++++++

// === Radix Server ================================================
//
// =================================================================
RadixServer::RadixServer(const int port, const unsigned int cores) {
  // your server implementation goes here :)
  std::vector<unsigned int> list;
  unsigned int onWire;
  unsigned int local;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    std::cout << "Server Error: socket() fail...\n";
    exit(-1);
  }

  struct sockaddr_in server_addr;
  bzero((char *) &server_addr, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    std::cout << "Server Error: bind() fail...\n";
    exit(-1);
  }

  listen(sockfd, cores);

  for(;;) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    int newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &len);
    if(newsockfd < 0) {
      std::cout << "Server Error: accept() fail...\n";
      exit(-1);
    }
    onWire = 0;
    int n = 1;
    while (true) {
      for(;;) {
	int i = 0;
	if (onWire == 0)
	  n = recv(newsockfd, (void*) &onWire, sizeof(unsigned int), 0);
	if(n < 0) {
	  std::cout << i << " call -> Server Error: recv() fail...\n";
	  exit(-1);
	}
	local = ntohl(onWire);
	if(local == 0) {
	  break;
	}
	list.push_back(local);
	i++;
	onWire = 0;
      }
  
      ParallelRadixSort::bucketSort(list, list.size(), cores);

      for(unsigned int i = 0; i < list.size(); i++) {
	local = list[i];
	onWire = htonl(local);
	int n = send(newsockfd, (void*)&onWire, sizeof(unsigned int), 0);
	if(n < 0) {
	  std::cout << "Server Error: send() fail...\n";
	  exit(-1);
	}
      }
      local = 0;
      onWire = htonl(local);
      int n = send(newsockfd, (void*) &onWire, sizeof(unsigned int), 0);
      if(n < 0) {
	std::cout << "Server Error: send() fail...\n";
	exit(-1);
      }
      list.clear();
      recv(newsockfd, (void*) &onWire, sizeof(unsigned int), 0);
      if (onWire == 0)
	break;
    }
    close(newsockfd);
  }

  close(sockfd);
    
}// End of "RadixServer"



// +++ RADIX CLIENT FUNCTIONS ++++++++++++++++++++++++++++++++++++++

// === RC::MSD =====================================================
//
// =================================================================
void RadixClient::msd(const char *hostname, const int port, std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists) { 
    // your client implementation goes here :)
  unsigned int local;
  unsigned int onWire;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) {
    std::cout << "Unable to open socket...\n";
    exit(-1);
  }
    

  struct hostent *server = gethostbyname(hostname);
  if(server == NULL)
    exit(-1);

  struct sockaddr_in serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

  serv_addr.sin_port = htons(port);

  if(connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    exit(-1);

  for(auto &list : lists) {
    for(unsigned int i = 0; i < list.get().size(); i++) {
      local = list.get().at(i);
      onWire = htonl(local);
      int n = send(sockfd, (void*)&onWire, sizeof(unsigned int), 0);
      if(n < 0)
	exit(-1);
    }
    local = 0;
    onWire = htonl(local);
    send(sockfd, (void*) &onWire, sizeof(unsigned int), 0);
  }

  local = -1;
  
  for(auto &list : lists) {
    list.get().clear();
    for(;;) {
      onWire = 0;
      int n = recv(sockfd, (void*) &onWire, sizeof(unsigned int), 0);
      if(n < 0)
	exit(-1);
      local = ntohl(onWire);
      if(local == 0)
	break;
      list.get().push_back(local);
    }
  }

  close(sockfd);
  
}// End of "RadixClient::msd"
