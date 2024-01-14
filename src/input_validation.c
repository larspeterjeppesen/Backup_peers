#include <stdio.h>
#include <string.h>
#include <errno.h>

int isValidIP(char* ip_string) {
  int ip[4];
  int n = sscanf(ip_string, "%d.%d.%d.%d", ip, ip+1, ip+2, ip+3);
  if (n < 0) {
    fprintf(stderr, "Error scanning ip: %s\n", strerror(errno));
    return 0; 
  }
  if (n != 4) {
    fprintf(stderr, "Invalid ip provided\n");
    return 0; 
  }
  for (int i = 0; i < 4; i++) {
    if (ip[i] < 0 || ip[i] > 255) {
      fprintf(stderr, "Invalid ip provided\n");
      return 0;
    }
  }  
  return 1;
}

int isValidPort(char* port_string) {
  int port;
  int n = sscanf(port_string, "%d", &port);
  if (n < 0) {
    fprintf(stderr, "Error scanning port: %s\n", strerror(errno));
    return 0; 
  }
  if (n != 1 || port < 0 || port > 65535) {
    fprintf(stderr, "Invalid port provided\n");
    return 0;
  } 
  return 1;
}

int isValidFile(char* file_path) {
  FILE* f = fopen(file_path, "r");
  if (f) {
    fclose(f);
    return 1;
  }
  return 0;
}



void print_help(void) {
  char* manual_usage = "For manual usage, use at least one of the following modes:\n";
  char* s = "-s [IP] [Port] [filepath]` - will send `[filepath]` to the given address.\n";
  char* r = "-r [Port]` - will listen for incoming connections on `[Port]`.\n";
  char* predef_usage = "\nFor predefined modes, use only one of the following:\n";
  char* premodes = "--phone, --pi, --desktop\n";
  fprintf(stdout, "%s%s%s%s%s", manual_usage, s, r, predef_usage, premodes);
  return;
}
