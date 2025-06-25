#ifndef CONSOLE_H
#define CONSOLE_H

typedef struct exec {
  const char *cmd;
  void (*f)(void);
} exec_t;

void console_add_buffer(char c);

void console_init(void);

#endif /* CONSOLE_H */
