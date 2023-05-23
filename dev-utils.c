#include "./dev-utils.h"

// @note O símbolo NO_TRACE controla a emissão de logs para o stdout

#ifdef NO_TRACE
  #define trace(message) ((void)0);
  #define tracef(message) ((void)0);
  #define trace_timed(message) ((void)0);
#else
  #define trace(message) printf("[%s:%d] %s\n", __FILE__, __LINE__, message);
  #define tracef(...) (printf("[%s:%d] ", __FILE__, __LINE__), printf(__VA_ARGS__), printf("\n"));
  #define trace_timed(message) log_trace(__FILE__, __LINE__, message);
#endif

void log_trace(const char *file_name, int line, const char *message)
{
  time_t now;
  time(&now);
  char formated_date_buff[20];
  strftime(formated_date_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

  printf("[%s] [%s:%d] %s\n", formated_date_buff, file_name, line, message);
}
