#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <error.h>
#include <errno.h>
#include <ctype.h>
#include "config-parser.h"

char *read_file(const char *path) {
  FILE *fp = fopen(path, "r");
  if (fp == NULL)
    return NULL;
  fseek(fp, 0, SEEK_END);
  long len = ftell(fp) - 1;
  rewind(fp);
  char *buf = malloc((len + 1) * sizeof (char));
  fread(buf, len, 1, fp);
  return buf[len] = '\0', buf;
}

bool file_exists(const char *path) {
  struct stat st;
  stat(path, &st);
  if (S_ISREG(st.st_mode))
    return true;
  return false;
}

char *str_trim(const char *s) {
  size_t start = 0, end = strlen(s) - 1, len;
  while (start < end && isspace(s[start]))
    start++;
  while (end > start && isspace(s[end]))
    end--;
  len = end - start + 1;
  char *r = malloc((len + 1) * sizeof (char));
  r[len] = '\0';
  memcpy(r, s + start, len);
  return r;
}

tokens_t *make_tokens(const char *_s, const char *delim, const int max) {
  char *s = strdup(_s);
  ssize_t cap = 1;
  tokens_t *r = malloc(sizeof (tokens_t));
  r->tokens = malloc(cap * sizeof (char *));
  r->len = 0;
  char *tok = strtok(s, delim);
  while (tok) {
    if (strlen(tok) > 0) {
      r->tokens[r->len++] = strdup(tok);
      if (r->len >= cap)
        r->tokens = realloc(r->tokens, (cap <<= 1) * sizeof (char *));
    }
    if (r->len == max) {
      if ( (tok = strtok(NULL, "")) )
        r->tokens[r->len++] = strdup(tok);
      break;
    }
    tok = strtok(NULL, delim);
  }
  r->tokens = realloc(r->tokens, r->len * sizeof (char *));
  free(s);
  return r;
}

void free_tokens(tokens_t *t) {
  for (ssize_t i = 0; i < t->len; i++)
    free(t->tokens[i]);
  free(t->tokens);
  free(t);
}

void parse_config(const char *s, void (*callback)(char**)) {
  tokens_t *t = make_tokens(s, "\n", -1);
  for (int i = 0; i < t->len; i++) {
    tokens_t *r = make_tokens(t->tokens[i], ":", 1);
    if (r->len == 2)
      callback(r->tokens);
    free_tokens(r);
  }
  free_tokens(t);
}

int parse_config_file(const char *path, void (*callback)(char**)) {
  char *data = read_file(path);
  if (data == NULL)
    return -1;
  parse_config(data, callback);
  free(data);
  return 0;
}
