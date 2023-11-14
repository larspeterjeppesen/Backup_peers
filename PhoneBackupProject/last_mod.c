#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>


int main(int argc, char** argv) {

  if (argc != 2) {
    fprintf(stderr, "Provide file\n");
    return 1;
  } 


  struct stat statbuf = {0};
  stat(argv[1], &statbuf);

  fprintf(stdout, "%ld\n", statbuf.st_mtim.tv_sec);

  return 0;
}
