/*
 * Copyright (C) 2018 David C. Harrison. All right reserved.
 *
 * You may not use, distribute, publish, or modify this code without 
 * the express written permission of the copyright holder.
 */

#include "crack.h"

// === Error ==================================================================
//
// ============================================================================
static void error(const char *msg) {
  std::cout << " Error: " << msg << "() failed....\n";
  exit(-1);
}// End of "error"


// === EntryFunc ==============================================================
//
// ============================================================================
static void entryFunc(char *pass) {
  
  char plainPasswd[5];
  crack(pass, plainPasswd);
  strncpy(pass, plainPasswd, sizeof(plainPasswd));
  
}// End of "entryFunc"



/*/ === Threaded Crack =========================================================
//
// ============================================================================
static void threadedCrack(Message &givenMsg) {
  std::vector<std::thread> threads;
  
  for(unsigned int i = 0; i < givenMsg.num_passwds; i++) {
    threads.push_back(std::thread(entryFunc,givenMsg.passwds[i]));
  }
  for(auto &th : threads) {
    th.join();
  }
  threads.clear();
    
}// End of "threadedCrack" */



// === PrintMsg ===============================================================
//
// ============================================================================
static void printMsg(const Message &msg) {
  std::cout << "Cruzid: " << msg.cruzid << std::endl;
  for(unsigned int i = 0; i < msg.num_passwds; i++) {
    std::cout << "Password: " << msg.passwds[i] << std::endl;
  }
  std::cout << "Num Passwords: " << msg.num_passwds << std::endl;
  std::cout << "Hostname: " << msg.hostname << std::endl;
  std::cout << "Port Number: " << msg.port << std::endl;
}


// === ConvertNTOHL ===========================================================
//
// ============================================================================
static Message convertNTOHL(const Message &ogMsg) {
  Message brdcstMsg;
  unsigned int i = 0;
  
  for(auto passwords : ogMsg.passwds) {
    strcpy(brdcstMsg.passwds[i++], passwords);
  }
  strcpy(brdcstMsg.cruzid, ogMsg.cruzid);
  brdcstMsg.num_passwds = ntohl(ogMsg.num_passwds);
  strcpy(brdcstMsg.hostname, ogMsg.hostname);
  brdcstMsg.port = ntohs(ogMsg.port);

  return brdcstMsg;
  
}// End of "convertNTOHL"



// === ConvertHTONL ===========================================================
//
// ============================================================================
static Message convertHTONL(const Message &ogMsg) {
  Message brdcstMsg;
  unsigned int i = 0;
  
  for(auto passwords : ogMsg.passwds) {
    strcpy(brdcstMsg.passwds[i++], passwords);
  }
  strcpy(brdcstMsg.cruzid, ogMsg.cruzid);
  brdcstMsg.num_passwds = htonl(ogMsg.num_passwds);
  strcpy(brdcstMsg.hostname, ogMsg.hostname);
  brdcstMsg.port = htonl(ogMsg.port);

  return brdcstMsg;
  
}// End of "convertHTONL"



/*/ === Fill Message ===========================================================
//
// ============================================================================
static Message fillMsg(const Message &msg, unsigned int start, unsigned int amount, const char *host) {
  Message srvMsg;
  unsigned int i = 0;

  if(strcmp(host, "grolliffe") != 0) {
    for(unsigned int j = start; j < start + amount; j++, i++) {
      strcpy(srvMsg.passwds[i], msg.passwds[j]);
    }
  }
  else {
    for(unsigned int j = start; j < msg.num_passwds; j++, i++) {
      strcpy(srvMsg.passwds[i], msg.passwds[j]);
    }
  }
  strcpy(srvMsg.cruzid, msg.cruzid);
  srvMsg.num_passwds = i;
  strcpy(srvMsg.hostname, host);
  srvMsg.port = msg.port;

  return srvMsg;
  
}// End of "fillMsg" */



/*/ === Cat Message ============================================================
//
// ============================================================================
static void catMsg(Message &fullMsg, const Message &local, unsigned int start) {
  unsigned int i = start;
  unsigned int j = 0;
  
  for(; j < local.num_passwds; i++, j++) {
    strcpy(fullMsg.passwds[i], local.passwds[j]);
  }
  fullMsg.num_passwds = start + local.num_passwds;
  
}// End of "catMsg" */



/*/ === Send to Master =========================================================
//
// ============================================================================
static void send_to_master(const Message &crackedMsg) {
  char master[] = "grolliffe";
  int outSockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(outSockfd < 0)
    error("open");

  struct hostent *server = gethostbyname(master);
  if(server == NULL)
    error("gethostbyname");

  struct sockaddr_in serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(get_unicast_port());

  if(connect(outSockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    error("connect");

  Message wireMsg = convertHTONL(crackedMsg);
  int n = send(outSockfd,(void *) &wireMsg, sizeof(Message), 0);
  if(n < 0)
    error("write");

  close(outSockfd);
      
}// End of "send_to_master" */



/*/ === SplitPasswds ===========================================================
//
// ============================================================================
static void splitPasswds(const Message &masterMsg) {
  char hostname[MAX_HOSTNAME_LEN];
  hostname[MAX_HOSTNAME_LEN - 1] = '\0';

  gethostname(hostname, MAX_HOSTNAME_LEN - 1);
  unsigned int amntPDB = masterMsg.num_passwds / NUM_SERVERS;
  unsigned int servID = 0;

  if(strcmp(hostname, "thor") == 0) {
    printf("Currently on %s\n", hostname);
    Message thorMsg = fillMsg(masterMsg, servID, amntPDB, hostname);
    std::cout << hostname << "has " << thorMsg.num_passwds << std::endl;
    std::cout << "Now cracking on: " << hostname << std::endl;
    threadedCrack(thorMsg);
    std::cout << "Done cracking on: " << hostname << std::endl;
    send_to_master(thorMsg);
  }
  else if(strcmp(hostname, "olaf") == 0) {
    printf("Currently on %s\n", hostname);
    Message olafMsg = fillMsg(masterMsg, servID + (1 * amntPDB), amntPDB, hostname);
    std::cout << hostname << "has " << olafMsg.num_passwds << std::endl;
    std::cout << "Now cracking on: " << hostname << std::endl;
    threadedCrack(olafMsg);
    std::cout << "Done cracking on: " << hostname << std::endl;
    send_to_master(olafMsg);
  }
  else if(strcmp(hostname, "graculus") == 0) {
    printf("Currently on %s\n", hostname);
    Message graculusMsg = fillMsg(masterMsg, servID + (2 * amntPDB), amntPDB, hostname);
    std::cout << hostname << "has " << graculusMsg.num_passwds << std::endl;
    std::cout << "Now cracking on: " << hostname << std::endl;
    threadedCrack(graculusMsg);
    std::cout << "Done cracking on: " << hostname << std::endl;
    send_to_master(graculusMsg);
  }
  else if(strcmp(hostname, "grolliffe") == 0) {
    printf("Currently on %s\n", hostname);
    Message grolliffeMsg = fillMsg(masterMsg, servID + (3 * amntPDB), amntPDB, hostname);
    std::cout << hostname << "has " << grolliffeMsg.num_passwds << std::endl;
    std::cout << "Now cracking on: " << hostname << std::endl;    
    threadedCrack(grolliffeMsg);
    std::cout << "Done cracking on: " << hostname << std::endl;
    send_to_master(grolliffeMsg);
  }
  
}// End of "splitPasswds" */



// === Send to TCP ============================================================
//
// ============================================================================
static void send_to_TCP(const Message &msg) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    error("open");

  struct hostent *server = gethostbyname(msg.hostname);
  if(server == NULL)
    error("server");

  struct sockaddr_in serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(msg.port);

  if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    error("connect");

  Message onWire = convertHTONL(msg);
  
  int n = send(sockfd,(void *) &onWire, sizeof(Message), 0);
  if(n < 0)
    error("send");

  close(sockfd);
  
}// End of "send_to_TCP"



/*/ === Recv from Others =======================================================
//
// ============================================================================
static Message recv_from_others(const Message &master) {
  std::vector<Message> messages;
  char prev[MAX_HOSTNAME_LEN];
  bzero(prev, MAX_HOSTNAME_LEN);
  prev[MAX_HOSTNAME_LEN - 1] = '\0';
  
  int inSockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (inSockfd < 0)
    error("open");

  struct sockaddr_in server_addr;
  bzero((char *) &server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(get_unicast_port());

  if(bind(inSockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    error("bind");

  listen(inSockfd, 4);

  Message recvWire;
  Message fullMsg;
  strcpy(fullMsg.cruzid, master.cruzid);
  fullMsg.num_passwds = 0;
  strcpy(fullMsg.hostname, master.hostname);
  fullMsg.port = master.port;
  
  for(;;) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    int newsockfd = accept(inSockfd, (struct sockaddr *) &client_addr, &len);
    if(newsockfd < 0)
      error("accept");

    int n = recv(newsockfd, recvWire, sizeof(Message), 0);
    if(n < 0)
      error("read");

    Message local = convertNTOHL(recvWire);
    if(strcmp(local.hostname, prev) != 0)
      messages.push_back(local);
    
    strcpy(prev, local.hostname);

    close(newsockfd);
  }
  
  close(inSockfd);

  unsigned int amntPDB = master.num_passwds / NUM_SERVERS;
  for(Message &msg : messages) {
    if(strcmp(msg.hostname, "thor") == 0) {
      catMsg(fullMsg, msg, 0);
    }
    else if(strcmp(msg.hostname, "olaf") == 0) {
      catMsg(fullMsg, msg, 0 + (1 * amntPDB));
    }
    else if(strcmp(msg.hostname, "graculus") == 0) {
      catMsg(fullMsg, msg, 0 + (2 * amntPDB));
    }
    else if(strcmp(msg.hostname, "grolliffe") == 0) {
      catMsg(fullMsg, msg, 0 + (3 * amntPDB));
    }
  }
  
  return fullMsg;
  
}// End of "recv_from_others" */



// === Main ===================================================================
// ============================================================================
int main(int argc, char *argv[]) {
  char currHost[MAX_HOSTNAME_LEN];
  currHost[MAX_HOSTNAME_LEN - 1] = '\0';
  gethostname(currHost, MAX_HOSTNAME_LEN - 1);
  std::vector<std::thread> threads;
  for(;;) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
      error("open");

    struct sockaddr_in server_addr;
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(get_multicast_port());

    if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
      error("bind");
  
    struct ip_mreq multicastRequest;
    multicastRequest.imr_multiaddr.s_addr = get_multicast_address();
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);

    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &multicastRequest, sizeof(multicastRequest)) < 0)
      error("setsockopt");

    Message wireMsg;
    //Receive datagram from broadcast
    bzero((void *) &wireMsg, sizeof(Message));
    while(strcmp(wireMsg.cruzid, "cnesparz") != 0) {
      int n = recvfrom(sockfd, (void *) &wireMsg, sizeof(Message), 0, NULL, 0);
      if(n < 0)
	error("read");
    }
    //Create converted copy of broadcast datagram from NL to HL
    Message localMsg = convertNTOHL(wireMsg);
    printf("Received Message: \n");
    printMsg(localMsg);

    /*/Split Message so each server can crack separate sets of passwords
    //printf("Now cracking passwords...\n");
    //if(localMsg.num_passwds > 24) {
    Message tcpMsg = recv_from_others(localMsg);
    splitPasswds(localMsg);
    if(strcmp(currHost, "grolliffe") != 0) {
      printf("%s Server Done...\n", currHost);
      close(sockfd);
      break;
    }
    //}
    //else {
    //  threadedCrack(localMsg);
    //}
    //printf("Finished cracking passwords...\n");*/
    
    for(unsigned int i = 0; i < localMsg.num_passwds; i++) {
      threads.push_back(std::thread(entryFunc,localMsg.passwds[i]));
    }
    for(auto &th : threads) {
      th.join();
    }
    threads.clear();
    
    
    
    send_to_TCP(localMsg);
    
    close(sockfd);
  }
}// End of "main"

