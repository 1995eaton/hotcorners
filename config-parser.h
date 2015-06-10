#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

char *read_file(const char *);
char *str_trim(const char*);

typedef struct {
  char **tokens;
  ssize_t len;
} tokens_t;


tokens_t *make_tokens(const char*, const char*, const int);
void free_tokens(tokens_t*);
void parse_config(const char*, void (*)(char**));
int parse_config_file(const char *, void (*)(char**));

#endif
