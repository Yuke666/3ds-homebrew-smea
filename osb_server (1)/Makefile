CC = gcc

# debugging on, source compiled into exe currently
CFLAGS  	:= -Wall -Wextra -g -pg -O2 -DDEBUG

LIBS 		:= -static-libgcc -lpthread -Wl,-Bstatic -lm

LFLAGS 		:= -g -pg -O2 -lc

SOURCES 	:= main.c log.c server.c memory.c

BUILD 		:= build

EXECUTABLE  := main

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT 		:= $(CURDIR)/$(EXECUTABLE)
export VPATH		:= $(CURDIR)
export OBJECTS 		:= $(SOURCES:.c=.o)

.PHONY: $(BUILD) clean all

all: $(BUILD) $(TARGET)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -rf $(BUILD)

else

$(OUTPUT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $(INC_PATH) $(CFLAGS) $< -o $@

endif