CC = gcc
SRC_DIR = src
CLIT_SRCS = main.c ClientCommu.c Render.c Util.c
SERV_SRCS = main_s.c functions.c listen.c update.c
SERV_OBJS = $(SERV_SRCS:%.c=%.o)
CLIT_OBJS = $(CLIT_SRCS:%.c=%.o)
TARGET = Sli
SERV_TARGET = Sli_Server

DBG_DIR = Debug
DBG_TARGET = $(DBG_DIR)/$(TARGET) $(DBG_DIR)/$(SERV_TARGET)

REL_DIR = Release
REL_TARGET = $(REL_DIR)/$(TARGET) $(REL_DIR)/$(SERV_TARGET)

LIB_HEAD_DIR = $(SRC_DIR)/ncurses
LIB_DIR = lib
LIB_NAME = pthread m

CFLAGS += -MMD -MP -I$(LIB_HEAD_DIR)
ifeq ($(PIPE), 1)
	CFLAGS += -DUSE_FIFO
endif

.SECONDEXPANSION:
.PRECIOUS: %.c %.o

.PHONY : all clean debug release dbg_prep rel_prep test

all : debug

debug : CFLAGS += -g -O0 -DDEBUG
debug : LIB_NAME += ncursestw_g
debug : dbg_prep $(DBG_TARGET)

release : CFLAGS += -O2 -DNDEBUG
release : LIB_NAME += ncursestw
release : rel_prep $(REL_TARGET)

%/$(TARGET) : $(addprefix %/, $(CLIT_OBJS))
	$(CC) -o $@ $^ -L$(LIB_DIR) $(addprefix -l,$(LIB_NAME))

%/$(SERV_TARGET) : $(addprefix %/, $(SERV_OBJS))
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
