#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

#define ERROR(msg) fprintf(stderr, "error: %s\n", msg)

void help(char *name)
{
  printf("usage: %s <command> [<args>]\n\n", name);
  printf("The most common Boiling commands used:\n\n");
  printf("new: creates a new project in current directory\n");
  printf("  --name:          specify the name of the application\n");
  printf("  --lang | -l:     set the programming language\n");
  printf("  --license:       indicate the license of the project\n");
  printf("  --git:           create an empty git repository\n\n");
  printf("config: verify the configuration of the application\n\n");
  printf("  --verify | -v:   verify the syntactic and lexical correctness of the configuration file\n");
  printf("  --where  | -w:   prints the config file path\n");
}

int verify_config()
{
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2 || argc == 2) {
    ERROR("Not enough arguments provided.");
    help(argv[0]);
    return 1;
  }

  char *command = argv[1];
  if (strcmp(command, "new") == 0) assert(0 && "Not implemented");

  if (command[0] == '-') {
    ERROR("The first argument must be a command.");
    help(argv[0]);
    return 1;
  }

  int i = 2;
  while (i < argc) {
    char *arg = argv[i];
    if (arg[0] != '-') {
      ERROR("Unknown flag specified.");
      return 1;
    }
    // Skip dashes
    arg += arg[1] == '-' ? 2 : 1;

    if (strcmp(arg, "verify") == 0 || strcmp(arg, "v") == 0) {
      return verify_config();
    }
    else if (strcmp(arg, "where") == 0 || strcmp(arg, "w") == 0) {
      char *home = getenv("HOME");
      if (home == NULL) {
        ERROR("$HOME environment variabile is not found.");
        return 1;
      }
      char path[512];
      sprintf(path, "%s/.config/boiling/boiling.conf", home);
      struct stat buf;
      if (stat(path, &buf) == 0) printf("%s\n", path);
      else printf("No config found.\n");
      return 0;
    }
    i++;
  }
  return 0;
}
