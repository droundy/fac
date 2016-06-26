#include <stdbool.h>

void string_argument(const char *long_name, char short_name, char **output,
                     const char *description, const char *outname);
void string_argument_list(const char *long_name, char short_name, const char ***output,
                          const char *description, const char *outname);
void int_argument(const char *long_name, char short_name, int *output,
                  const char *description, const char *outname);
void no_argument(const char *long_name, char short_name, bool *output,
                 const char *description);

const char **parse_arguments_return_extras(const char **argv);
