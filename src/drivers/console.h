#ifndef CONSOLE_H
#define CONSOLE_H

typedef struct exec {
  const char *cmd;
  void (*f)(void);
} exec_t;

void add_buffer(char c);

#endif /* CONSOLE_H */
