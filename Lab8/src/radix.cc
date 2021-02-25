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

/*
 * Start a UDP listener on PORT and accept lists of unsiged integers from
 * clients to be MSD RAdix sorted. After sorting the lists are to be returned 
 * to the client.
 */
void RadixServer::start(const int port) {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0)
    error("Server","socket");

  struct sockaddr_in server_addr;
  bzero((char *) &server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    error("Server","bind");

  struct sockaddr_in remote_addr;

  for(;;) {
    std::vector<Message> srvDatagrams;
    unsigned int recvPackets = 0;
    do {
      recvList(sockfd, (struct sockaddr *) &remote_addr, srvDatagrams);
      recvPackets++;
    }while(srvDatagrams.back().flag != LAST);

    if(recvPackets - 1 != srvDatagrams.back().sequence)
      resend(sockfd, (struct sockaddr *) &remote_addr, srvDatagrams, srvDatagrams.back().sequence + 1);
    
    std::vector<unsigned int> srvList;
    fillList(srvList, srvDatagrams);
   
    ParallelRadixSort::bucketSort(srvList, srvList.size(), 24);

    srvDatagrams.clear();
    fillMessage(srvList, srvDatagrams);

    sendList(sockfd, (struct sockaddr *) &remote_addr, srvDatagrams);
    srvDatagrams.clear();
  }

  close(sockfd);
}// End of "RadixServer::start"

/*
 * Send the contents of the lists contained with LISTS to the server on HIOSTNAME
 * listening on PORT in datagrams containing batches of unsigned integers. These
 * will be returned to you MSD Radix sorted and should be retied to the caller
 * via LISTS.
 */
// Code Credit: https://stackoverflow.com/questions/13547721/udp-socket-set-timeout
void RadixClient::msd(const char *hostname, const int port, std::vector<std::reference_wrapper<std::vector<unsigned int>>> &lists) {
  std::vector<Message> datagrams;
  unsigned int numPackets;
  
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd < 0)
    error("Client","socket");

  struct hostent *server = gethostbyname(hostname);
  if(server == NULL)
    error("Client","gethostbyname");

  struct sockaddr_in remote_addr;
  bzero((char *) &remote_addr, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&remote_addr.sin_addr.s_addr, server->h_length);
  remote_addr.sin_port = htons(port);


  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 50000;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    error("Client","settimeout");
  }
  
  for( auto &list : lists) {
    numPackets = list.get().size()/(MAX_VALUES);
    if((list.get().size() % MAX_VALUES) > 0)
      ++numPackets;

    //std::cout << "Size of numPackets: " << numPackets << std::endl;
    //Fill messages
    fillMessage(list,datagrams);

    //Send datagrams
    sendList(sockfd, (struct sockaddr *) &remote_addr, datagrams);

    //Receive datagrams
    datagrams.clear();
    for(unsigned int i = 0; i < numPackets; i++) {
      if(recvList(sockfd, (struct sockaddr *) &remote_addr, datagrams) < 1) {
	resend(sockfd, (struct sockaddr *) &remote_addr, datagrams, numPackets);
	break;
      }
    }

    //Refill list
    fillList(list, datagrams);

    datagrams.clear();
  }

  close(sockfd);
}// End of "RadixClient::msd"



// === Error ==================================================================
//
// ============================================================================
static void error(const char *msg1, const char *msg2) {
  std::cout << "Radix " << msg1 << " Error: " << msg2 << "() failed....\n";
  exit(-1);
}// End of "error"



// === recvList ===============================================================
//
// ============================================================================
static int recvList(int &sockfd, struct sockaddr *rAddr, std::vector<Message> &dataG) {
  Message recvDatagram;
  size_t numBytes = sizeof(Message);
  socklen_t len = sizeof(struct sockaddr_in);

  bzero((void *) &recvDatagram, numBytes);
  int n = recvfrom(sockfd,(void *) &recvDatagram, numBytes, 0, rAddr, &len);
  if(n < 1) {
    //std::cout << "Value of n = " << n << std::endl;
    return n;
  }

  Message localData;
  unsigned int i = 0;
  for(unsigned int data : recvDatagram.values) {
    unsigned int local = ntohl(data);
    localData.values[i++] = local;
  }
  localData.num_values = ntohl(recvDatagram.num_values);
  localData.sequence = ntohl(recvDatagram.sequence);
  localData.flag = ntohl(recvDatagram.flag);

  dataG.push_back(localData);

  return n;
}// End of "recvList"



// === sendList ===============================================================
//
// ============================================================================
static void sendList(int &sockfd, struct sockaddr *rAddr, std::vector<Message> &datagrams) {
  socklen_t len = sizeof(struct sockaddr_in);

  for(Message &datagram : datagrams) {
    int n = sendto(sockfd,(void *) &datagram, sizeof(Message), 0, rAddr, len);
    if(n < 0) {
      //std::cout << "Value of n = " << n << std::endl;
      error("sendList","sendto");
    }
  }
}// End of "sendList" */



// === fillMessage ============================================================
//
// ============================================================================
static void fillMessage(std::vector<unsigned int> &list, std::vector<Message> &datagrams) {
  Message datagram;
  unsigned int index = 0;
  unsigned int seqNum = 0;
  unsigned int numVals = list.size();
  

  for(unsigned int num : list) {
    unsigned int onWire = htonl(num);
    datagram.values[index++] = onWire;
    if(index == MAX_VALUES && numVals != 0) {
      datagram.num_values = htonl(index);
      datagram.sequence = htonl(seqNum);
      datagram.flag = htonl(NONE);
      datagrams.push_back(datagram);
      index = 0;
      ++seqNum;
    }
    --numVals;
  }

  datagram.num_values = htonl(index);
  datagram.sequence = htonl(seqNum);
  datagram.flag = htonl(LAST);

  datagrams.push_back(datagram);
}// End of "fillMessage"



// === fillList ===============================================================
//
// ============================================================================
static void fillList(std::vector<unsigned int> &list, std::vector<Message> &datagrams) {
  list.clear();
  std::sort(datagrams.begin(), datagrams.end(), compareMsg);
    for(Message datagram : datagrams) {
      for(unsigned int i = 0; i < datagram.num_values; i++) {
	list.push_back(datagram.values[i]);
      }
    }
}// End of "fillList"



// === resend =================================================================
//
// ============================================================================
static void resend(int &sockfd, struct sockaddr *rAddr, std::vector<Message> &datagrams, const unsigned int numPackets) {
  std::vector<unsigned int> missingP;
  socklen_t len = sizeof(struct sockaddr_in);

  //std::cout << "value of numPackets: " << numPackets << std::endl;
  //std::cout << "size of datagrams: " << datagrams.size() << std::endl;
  
  unsigned int i = 0;
  unsigned int j = 0;
  while(missingP.size() != (numPackets - datagrams.size())) {
    if(i != datagrams[j].sequence) {
      missingP.push_back(i);
      i++;
    }
    else {
      i++;
      j++;
    }
  }

  Message resendM;
  unsigned int c = 0;
  for(auto missSeq : missingP) {
    resendM.values[c++] = htonl(missSeq);
  }
  resendM.num_values = htonl(missingP.size());
  resendM.sequence = htonl(0);
  resendM.flag = htonl(RESEND);

  int n = sendto(sockfd,(void *) &resendM, sizeof(Message), 0, rAddr, len);
  if(n < 0)
    error("Resend","sendto");

  do {
    recvList(sockfd, rAddr, datagrams);
  }while(datagrams.back().flag != LAST);

}// End of "resend"




// === compareMsg ==================================================
//
// =================================================================
static bool compareMsg(const Message a, const Message b)
{
  return a.sequence < b.sequence;
}// End of "compareMsg"
