#include "log.h"
#include <stdio.h>
#include <stdarg.h>

void log_formatted_info(int color, const char *file, int line, const char *format, ...){

	(void)color;

	printf("%s, Line %i\n", file, line);

	va_list args;
	va_start(args, format);

	vprintf(format, args);

	va_end(args);
}

void log_unformatted_info(int color, const char *file, int line, const char *text){

	(void)color;
	// printf("\x1b[33m%s, Line %i\n\x1b[%im%s\x1b[0m", file, line, color, text);
	printf("%s, Line %i\n%s", file, line, text);
}

void log_formatted(int color, const char *format, ...){

	printf("\x1b[%im", color);

	va_list args;
	va_start(args, format);

	vprintf(format, args);

	va_end(args);

	printf("\x1b[0m");
}

void log_unformatted(int color, const char *text){

	printf("\x1b[%im%s\x1b[0m", color, text);
}