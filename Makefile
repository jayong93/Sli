CC = gcc
SRC_DIR = src
SRCS = main.c Render.c ClientCommu.c Util.c
OBJS = $(SRCS:%.c=%.o)
TARGET = Sli

DBG_DIR = Debug
DBG_TARGET = $(DBG_DIR)/$(TARGET)

REL_DIR = Release
REL_TARGET = $(REL_DIR)/$(TARGET)

LIB_HEAD_DIR = $(SRC_DIR)/ncurses
LIB_DIR = lib
LIB_NAME = ncurses pthread

CFLAGS += -MMD -MP -I$(LIB_HEAD_DIR)
ifeq ($(PIPE), 1)
	CFLAGS += -DUSE_FIFO
endif

.SECONDEXPANSION:
.PRECIOUS: %.c %.o

.PHONY : all clean debug release dbg_prep rel_prep test

all : debug

debug : CFLAGS += -g -O0 -DDEBUG
debug : dbg_prep $(DBG_TARGET)

release : CFLAGS += -O2 -DNDEBUG
release : rel_prep $(REL_TARGET)

%/$(TARGET) : $(addprefix %/, $(OBJS))
	$(CC) -o $@ $^ -L$(LIB_DIR) $(addprefix -l,$(LIB_NAME))

%.o : $(SRC_DIR)/$$(*F).c
	$(CC) $(CFLAGS) -MT $@ -MF $(patsubst %.c,$(@D)/.dep/%.d,$(notdir $<)) -c $< -o $@

dbg_prep :
	@mkdir -p $(DBG_DIR)/.dep

rel_prep :
	@mkdir -p $(REL_DIR)/.dep

clean :
	@rm -drf $(DBG_DIR) $(REL_DIR) $(TEST_DIR)

-include $(SRCS:%.c=$(DBG_DIR)/.dep/%.d)
-include $(SRCS:%.c=$(REL_DIR)/.dep/%.d)
