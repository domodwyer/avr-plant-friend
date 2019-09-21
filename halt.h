#ifndef HALT_H
#define HALT_H

/// A helper macro that executes halt() if expr is false.
#define ASSERT(expr)                                                           \
  if (!(expr)) {                                                               \
    halt();                                                                    \
  }

// Emergency graceful stop - disable pumps and sleep forever.
extern void halt() __attribute__((__noreturn__));

#endif /* HALT_H */