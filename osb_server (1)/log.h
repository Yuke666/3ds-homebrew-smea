#ifndef LOG_DEF
#define LOG_DEF

enum {
	LOG_NORMAL = 0,
	LOG_RED = 31,
	LOG_GREEN = 32,
	LOG_YELLOW = 33,
	LOG_BLUE = 34,
	LOG_MAGENTA = 35,
	LOG_CYAN = 36,
};

void log_formatted_info(int color, const char *file, int line, const char *format, ...);
void log_unformatted_info(int color, const char *file, int line, const char *text);
void log_unformatted(int color, const char *text);
void log_formatted(int color, const char *format, ...);

#define INFO_LOGF(color, format, ...) log_formatted_info(color, __FILE__, __LINE__, format, __VA_ARGS__);
#define INFO_LOG(color, text) log_unformatted_info(color, __FILE__, __LINE__, text);

#define LOGF(color, format, ...) log_formatted(color, format, __VA_ARGS__);
#define LOG(color, text) log_unformatted(color, text);

#endif