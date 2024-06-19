#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
// #include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Arrays (in a file)
char IP_FILE_NAME[] = "ip_addresses.txt";
// #define PORT 50000
#define INET_ADDRSTRLEN 16
#define RAND_RANGE 20000
#define MAX_CLIENTS 10

typedef struct {
  char ip[INET_ADDRSTRLEN];
  int port;
} client_info;

void handle_client(int client_socket, struct sockaddr_in client_addr);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s port\n", argv[0]);
    exit(1);
  }

  int ss = socket(AF_INET, SOCK_STREAM, 0);
  if (ss == -1) {
    perror("socket");
    exit(1);
  }

  // delete the file
  remove(IP_FILE_NAME);

  FILE *f = fopen(IP_FILE_NAME, "w");

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[1]));
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    close(ss);
    exit(1);
  }

  if (listen(ss, 10) == -1) {
    perror("listen");
    close(ss);
    exit(1);
  }

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int s = accept(ss, (struct sockaddr *)&client_addr, &len);

    if (s == -1) {
      perror("accept");
      continue;
    }

    printf("Connection from %s port %d\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    if (fork() == 0) {
      close(ss); // Child process does not need the listening socket
      handle_client(s, client_addr);
      printf("Connction to %s port %d closed\n",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
      close(s);
      exit(0);
    }
    close(s); // Parent process does not need the connected socket
  }

  close(ss);
  return 0;
}

void handle_client(int client_socket, struct sockaddr_in client_addr) {

  client_info clients[MAX_CLIENTS] = {0};
  FILE *f = fopen(IP_FILE_NAME, "r");
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (fscanf(f, "%s %d", clients[i].ip, &clients[i].port) == EOF) {
      break;
    }
  }
  // send ckient info to client
  send(client_socket, clients, sizeof(clients), 0);
  // Receive the port number from the client
  int port;
  recv(client_socket, &port, sizeof(port), 0);

  // Send all IP addresses to client
  // FILE *f = fopen(IP_FILE_NAME, "r");
  // char ip_addr[INET_ADDRSTRLEN];
  // while (fgets(ip_addr, sizeof(ip_addr), f) != NULL) {
  //   send(client_socket, ip_addr, strlen(ip_addr), 0);
  // }
  // fclose(f);

  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));

  // Write the IP address to the file
  f = fopen(IP_FILE_NAME, "a");
  fprintf(f, "%s\n %d\n", ip, port);
  printf("Phone server @ %s port %d\n", ip, port);
  fclose(f);
}
