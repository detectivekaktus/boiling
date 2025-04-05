#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ERROR(msg) fprintf(stderr, "error: %s\n", msg)
#define ERRORF(msg, ...) fprintf(stderr, "error: "); fprintf(stderr, msg, ##__VA_ARGS__)

#define ISSTREQ(str1, str2) strcmp(str1, str2) == 0

#define MAX_CONFIG_PATH 512

char *trim_trailing_space(char *str)
{
  int i = strlen(str) - 1;
  while (str[i] == ' ') i--;
  char *res = malloc(i + 2);
  strncpy(res, str, i + 1);
  res[i + 1] = '\0';
  return res;
}

void help(const char *name)
{
  printf("usage: %s <command> [<args>]\n\n", name);
  printf("The most common Boiling commands used:\n\n");
  printf("new: creates a new project in current directory\n");
  printf("  --name:          specify the name of the application\n");
  printf("  --lang | -l:     set the programming language\n");
  printf("  --license:       indicate the license of the project\n");
  printf("  --git:           create an empty git repository\n\n");
  printf("config: verify the configuration of the application\n");
  printf("  --verify | -v:   verify the syntactic and lexical correctness of the configuration file\n");
  printf("  --where  | -w:   prints the config file path\n");
}

char *find_config()
{
  char *home = getenv("HOME");
  if (home == NULL)
    return NULL;
  char *path = malloc(MAX_CONFIG_PATH);
  sprintf(path, "%s/.config/boiling/boiling.conf", home);
  if (access(path, F_OK) != 0) {
    free(path);
    return NULL;
  }
  return path;
}

int handle_where_config()
{
  char *path = find_config();
  if (path == NULL) {
    printf("No config found. Be sure to have set $HOME value. If $HOME is set up correctly, check/create $HOME/.config/boiling directory.\n");
    return 1;
  }
  printf("Config path: %s\n", path);
  free(path);
  return 0;
}

char *load_config()
{
  char *path = find_config();
  if (path == NULL)
    return NULL;

  FILE *f = fopen(path, "r");
  if (f == NULL)
    return NULL;
  fseek(f, 0, SEEK_END);
  int bytes = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *config = malloc(bytes + 1);
  char c;
  int i = 0;
  while ((c = getc(f)) != EOF)
    config[i++] = c;
  config[i] = '\0';

  free(path);
  fclose(f);
  return config;
}

typedef enum {
  CONFIG_KEY,
  CONFIG_VALUE,
  CONFIG_SECTION,
} ConfigTokenType;

typedef struct {
  ConfigTokenType type;
  char *value;
} ConfigToken;

typedef struct {
  ConfigToken *items;
  size_t size;
  size_t capacity;
} ConfigTokens;

#define YIELD_TOKEN(tipe, val) (ConfigToken) { .type = tipe, .value = val }
#define MAX_SECTION_NAME_LEN 64
#define MAX_KEY_NAME_LEN 256
#define MAX_VALUE_LEN 256

ConfigTokens *lex_config()
{
  char *config = load_config();
  if (config == NULL) {
    ERROR("Could not load config to generate new project.");
    exit(1);
  }

  ConfigTokens *tokens = malloc(sizeof(ConfigTokens));
  tokens->items = malloc(sizeof(ConfigToken) * 32);
  tokens->size = 0;
  tokens->capacity = 32;

  ConfigTokenType global_type = CONFIG_SECTION;
  int i = 0;
  while (config[i] != '\0') {
    switch (config[i]) {
      case ' ':
      case '\n':
      case '\t': {
        i++;
      } break;

      case '[': {
        int start = ++i;
        while (config[i] != ']' && config[i] != ' ' && config[i] != '\n' && config[i] != '\0')
          i++;
        if (config[i] == '\n' || config[i] == '\0') {
          ERROR("Section has start but has no end.");
          exit(1);
        }
        else if (config[i] == ' ') {
          ERROR("Section name must not contain spaces.");
          exit(1);
        }

        if (i - start > MAX_SECTION_NAME_LEN - 1) {
          ERRORF("Section name is too long. Max chars: %d\n", MAX_SECTION_NAME_LEN);
          exit(1);
        }
        char *value = malloc(MAX_SECTION_NAME_LEN);
        strncpy(value, config + start, i - start);
        value[i - start] = '\0';
        if (tokens->size >= tokens->capacity) {
          tokens->capacity *= 2;
          tokens->items = realloc(tokens->items, sizeof(ConfigToken) * tokens->capacity);
        }
        tokens->items[tokens->size++] = YIELD_TOKEN(CONFIG_SECTION, value);
        global_type = CONFIG_SECTION;
        i++;
      } break;

      case '#': {
        while (config[i] != '\n' && config[i] != '\0')
          i++;
      } break;

      default: {
        if (global_type == CONFIG_SECTION || global_type == CONFIG_VALUE) {
          int start = i;
          while (config[i] != '=' && config[i] != ' ' && config[i] != '\n' && config[i] != '\0')
            i++;
          if (config[i] == '\n' || config[i] == '\0') {
            ERROR("configuration key but no value");
            exit(1);
          }
          else if (config[i] == ' ') {
            ERROR("Key must not contain spaces.");
            exit(1);
          }

          if (i - start > MAX_KEY_NAME_LEN - 1) {
            ERRORF("Key name is too long. Max chars: %d\n", MAX_KEY_NAME_LEN);
            exit(1);
          }
          char *key_value = malloc(MAX_KEY_NAME_LEN);
          strncpy(key_value, config + start, i - start);
          key_value[i - start] = '\0';
          if (tokens->size >= tokens->capacity) {
            tokens->capacity *= 2;
            tokens->items = realloc(tokens->items, sizeof(ConfigToken) * tokens->capacity);
          }
          tokens->items[tokens->size++] = YIELD_TOKEN(CONFIG_KEY, key_value);
          i++;
          if (config[i] == ' ') {
            ERROR("There must be no space after `=` token.");
            exit(1);
          }
          start = i;
          while (config[i] != '\n' && config[i] != '\0')
            i++;
          if (i - start > MAX_VALUE_LEN - 1) {
            ERRORF("Value is too long. Max chars: %d", MAX_VALUE_LEN);
            exit(1);
          }
          char *val_value = malloc(MAX_VALUE_LEN);
          strncpy(val_value, config + start, i - start);
          val_value[i - start] = '\0';
          char *trimmed_val_value = trim_trailing_space(val_value);
          free(val_value);
          if (tokens->size >= tokens->capacity) {
            tokens->capacity *= 2;
            tokens->items = realloc(tokens->items, sizeof(ConfigToken) * tokens->capacity);
          }
          tokens->items[tokens->size++] = YIELD_TOKEN(CONFIG_VALUE, trimmed_val_value);
          global_type = CONFIG_VALUE;
          i++;
        }
      } break;
    }
  }

  free(config);
  return tokens;
}

typedef struct {
  char *key;
  char *value;
  void *next;
} ConfigEntry;

typedef struct {
  ConfigEntry **buckets;
  size_t size;
  size_t capacity;
} Config;

#define CONFIG_INIT_CAPACITY 256

#define GLOBAL_CONFIG 0
#define CLANG_CONFIG  1
#define CPP_CONFIG    2
#define PYTHON_CONFIG 3

#define TOTAL_CONFIGS 4

typedef struct {
  Config **items;
  size_t capacity;
  size_t size;
} Configs;

void add_config(Configs *confs, Config *conf)
{
  if (confs->size >= confs->capacity) {
    ERROR("Maximum configs limit extended.");
    exit(1);
  }
  confs->items[confs->size++] = conf;
}

Config *create_config()
{
  Config *conf = malloc(sizeof(Config));
  conf->capacity = CONFIG_INIT_CAPACITY;
  conf->size = 0;
  conf->buckets = calloc(conf->capacity, sizeof(ConfigEntry *));
  return conf;
}

ConfigEntry *create_config_entry(char *key, char *value, void *next)
{
  ConfigEntry *entry = malloc(sizeof(ConfigEntry));
  entry->key = key;
  entry->value = value;
  entry->next = next;
  return entry;
}

void destroy_config_entry(ConfigEntry *entry)
{
  free(entry->key);
  free(entry->value);
  ConfigEntry *next = entry->next;
  while (next != NULL)
    destroy_config_entry(next);
  free(entry);
}

int add_conf_entry(Config *conf, char *key, char *value)
{
  int hash = 0;
  for (size_t i = 0; i < strlen(key) + 1; i++)
    hash += key[i];
  hash %= conf->capacity;
  if (conf->buckets[hash] == NULL)
    conf->buckets[hash] = create_config_entry(key, value, NULL);
  else {
    ConfigEntry *entry = conf->buckets[hash];
    while (entry->next != NULL) {
      entry = entry->next;
    }
    entry->next = create_config_entry(key, value, NULL);
  }
  conf->size++;
  return 0;
}

ConfigEntry *get_conf_entry(Config *conf, char *key)
{
  int hash = 0;
  for (size_t i = 0; i < strlen(key) + 1; i++)
    hash += key[i];
  hash %= conf->capacity;
  if (conf->buckets[hash] == NULL)
    return NULL;
  if (ISSTREQ(conf->buckets[hash]->key, key))
    return conf->buckets[hash];
  else {
    ConfigEntry *entry = conf->buckets[hash]->next;
    if (entry == NULL)
      return NULL;
    while (!ISSTREQ(entry->key, key)) {
      entry = entry->next;
    }
    return entry;
  }
}

void destroy_config(Config *conf)
{
  for (size_t i = 0; i < conf->capacity; i++) {
    ConfigEntry *entry = conf->buckets[i];
    if (entry == NULL) continue;
    destroy_config_entry(entry);
  }
  free(conf->buckets);
  free(conf);
}

bool is_known_key(char *key)
{
  return ISSTREQ(key, "name") || ISSTREQ(key, "license") || 
    ISSTREQ(key, "gitrepo") || ISSTREQ(key, "readme") ||
    ISSTREQ(key, "build") || ISSTREQ(key, "src") ||
    ISSTREQ(key, "bin") || ISSTREQ(key, "version");
}

Configs *parse_config(ConfigTokens *tokens)
{
  // Arena?
  Configs *confs = malloc(sizeof(Configs));
  confs->capacity = TOTAL_CONFIGS;
  confs->size = 0;
  confs->items = malloc(sizeof(Config *) * confs->capacity);
  Config *global = create_config();
  add_config(confs, global);
  Config *clang = create_config();
  add_config(confs, clang);
  Config *cpp = create_config();
  add_config(confs, cpp);
  Config *python = create_config();
  add_config(confs, python);
  
  size_t i = 0;
  size_t conf = GLOBAL_CONFIG;
  while (i < tokens->size) {
    ConfigToken token = tokens->items[i];
    switch (token.type) {
      case CONFIG_SECTION: {
        if (ISSTREQ(token.value, "Core"))
          conf = GLOBAL_CONFIG;
        else if (ISSTREQ(token.value, "Language")) {
          token = tokens->items[++i];
          if (!ISSTREQ(token.value, "name")) {
            ERROR("The first config entry after `Language` section must be `name`");
            exit(1);
          }
          free(token.value);
          token = tokens->items[++i];
          char *value = token.value;
          if (ISSTREQ(value, "clang"))
            conf = CLANG_CONFIG;
          else if (ISSTREQ(value, "cpp"))
            conf = CPP_CONFIG;
          else if (ISSTREQ(value, "py"))
            conf = PYTHON_CONFIG;
          else {
            ERRORF("Unknown language `%s`\n", value);
            exit(1);
          }
          free(token.value);
        }
        else {
          ERRORF("Unknown section name `%s`\n", token.value);
          exit(1);
        }
        i++;
      } break;

      case CONFIG_KEY: {
        char *key = token.value;
        if (!is_known_key(key)) {
          ERRORF("`%s` doesn't appear to be a known key.\n", key);
          exit(1);
        }
        token = tokens->items[++i];
        char *value = token.value;
        add_conf_entry(confs->items[conf], key, value);
        i++;
      } break;

      default: {
        assert(0 && "unreachable");
      } break;
    }
  }
  free(tokens);
  return confs;
}

bool is_bool(char *str)
{
  return ISSTREQ(str, "true") || ISSTREQ(str, "false");
}

bool file_exists(char *path)
{
  return access(path, F_OK) == 0;
}

bool is_dir(char *path)
{
  struct stat pstat;
  stat(path, &pstat);
  return S_ISDIR(pstat.st_mode);
}

bool is_valid_path(char *str)
{
  char *s = strchr(str, 0);
  return (long long int) strlen(str) == s - str;
}

bool is_valid_core_config(Config *config)
{
  if (config->size == 0) {
    ERROR("`Core` must have `name` key value pair.");
    return false;
  }

  ConfigEntry *entry = get_conf_entry(config, "name");
  if (entry == NULL) {
    ERROR("no `name` specified for boiling.");
    return false;
  }

  entry = get_conf_entry(config, "license");
  if (entry != NULL) {
    if (!file_exists(entry->value)) {
      ERRORF("%s path does not exist. Omitting `license` key.\n", entry->value);
      return false;
    }
    if (is_dir(entry->value)) {
      ERRORF("%s path is directory, not a file. Omitting `license` key.\n", entry->value);
      return false;
    }
  }

  entry = get_conf_entry(config, "gitrepo");
  if (entry != NULL && !is_bool(entry->value)) {
    ERRORF("`gitrepo` expects boolean value. `%s` provided.\n", entry->value);
    return false;
  }
  return true;
}

bool is_valid_cstd(char *std)
{
  return ISSTREQ(std, "c89") || ISSTREQ(std, "c99") ||
    ISSTREQ(std, "c11") || ISSTREQ(std, "c17") ||
    ISSTREQ(std, "c23");
}

bool is_valid_clang_config(Config *config)
{
  if (config->size == 0)
    return true;
  ConfigEntry *entry = get_conf_entry(config, "bin");
  if (entry != NULL && !is_valid_path(entry->value)) {
    ERRORF("`%s` is not a valid path.\n", entry->value);
    return false;
  }
  entry = get_conf_entry(config, "src");
  if (entry != NULL && !is_valid_path(entry->value)) {
    ERRORF("`%s` is not a valid path.\n", entry->value);
    return false;
  }
  entry = get_conf_entry(config, "build");
  if (entry != NULL) {
    if (!file_exists(entry->value)) {
      ERRORF("`%s` does not exist.\n", entry->value);
      return false;
    }
    if (is_dir(entry->value)) {
      ERRORF("`%s` is a directory. Expected a file.\n", entry->value);
      return false;
    }
  }
  entry = get_conf_entry(config, "version");
  if (entry != NULL && !is_valid_cstd(entry->value)) {
    ERRORF("`%s` is not a valid c standard.\n", entry->value);
    return false;
  }
  return true;
}

bool is_valid_cppstd(char *str)
{
  return ISSTREQ(str, "c++03") || ISSTREQ(str, "c++11") ||
    ISSTREQ(str, "c++14") || ISSTREQ(str, "c++17") ||
    ISSTREQ(str, "c++20") || ISSTREQ(str, "c++23");
}

bool is_valid_cpp_config(Config *config)
{
  if (config == NULL)
    return true;
  
  ConfigEntry *entry = get_conf_entry(config, "bin");
  if (entry != NULL && !is_valid_path(entry->value)) {
    ERRORF("`%s` is not a valid path.\n", entry->value);
    return false;
  }
  entry = get_conf_entry(config, "src");
  if (entry != NULL && !is_valid_path(entry->value)) {
    ERRORF("`%s` is not a valid path.\n", entry->value);
    return false;
  }
  entry = get_conf_entry(config, "build");
  if (entry != NULL) {
    if (!file_exists(entry->value)) {
      ERRORF("`%s` does not exist.\n", entry->value);
      return false;
    }
    if (is_dir(entry->value)) {
      ERRORF("`%s` is a directory. Expected a file.\n", entry->value);
      return false;
    }
  }
  entry = get_conf_entry(config, "version");
  if (entry != NULL && !is_valid_cppstd(entry->value)) {
    ERRORF("`%s` is not a valid c++ standard.\n", entry->value);
    return false;
  }
  return true;
}

int verify_config()
{
  Configs *confs = parse_config(lex_config());
  if (confs == NULL)
    return 1;

  if (!is_valid_core_config(confs->items[GLOBAL_CONFIG])) return 1;
  if (!is_valid_clang_config(confs->items[CLANG_CONFIG])) return 1;
  if (!is_valid_cpp_config(confs->items[CPP_CONFIG])) return 1;

  return 0;
}

int handle_verify_config()
{
  int status = verify_config();
  if (status == 0) {
    printf("Config contains no errors.\n");
    return 0;
  }
  return 1;
}

int handle_config_command(int argc, char **argv)
{
  bool verified = false;
  bool found = false;

  for (int i = 2; i < argc; i++) {
    char *arg = argv[i];
    if (arg[0] != '-') {
      ERRORF("Unknown flag `%s`.\n", arg);
      return 1;
    }
    // Skip dashes
    arg += arg[1] == '-' ? 2 : 1;

    if (ISSTREQ(arg, "verify") || ISSTREQ(arg, "v")) {
      if (verified) continue;
      if (handle_verify_config() != 0)
        return 1;
      verified = true;
    }
    else if (ISSTREQ(arg, "where") || ISSTREQ(arg, "w")) {
      if (found) continue;
      if (handle_where_config() != 0)
        return 1;
      found = true;
    }
    else {
      ERRORF("Unknown flag `%s`.\n", arg);
      return 1;
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 3) {
    ERROR("Not enough arguments provided. Expected at least 3.\n");
    help(argv[0]);
    return 1;
  }

  char *command = argv[1];
  if (command[0] == '-') {
    ERROR("The first argument must be a command.\n");
    help(argv[0]);
    return 1;
  }
  if (ISSTREQ(command, "new"))
    assert(0 && "Not implemented");
  if (ISSTREQ(command, "config"))
    return handle_config_command(argc, argv);
  else {
    ERRORF("Unknown command `%s`.\n", command);
    return 1;
  }
  return 0;
}
