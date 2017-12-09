CC = gcc
SRC_DIR = src
SRCS = main.c
ENTRY_POINT = main.c
OBJS = $(SRCS:%.c=%.o)
TARGET = Sli

DBG_DIR = Debug
DBG_TARGET = $(DBG_DIR)/$(TARGET)

REL_DIR = Release
REL_TARGET = $(REL_DIR)/$(TARGET)

TEST_DIR = Test
TEST_TARGET = $(TEST_DIR)/$(TARGET)_TEST
TEST_ENTRY_POINT = AllTest.c
TEST_SRCS = $(subst $(ENTRY_POINT),$(TEST_ENTRY_POINT),$(SRCS))
TEST_SRCS += CuTest.c
TEST_OBJS = $(TEST_SRCS:%.c=%.o)

CFLAGS += -MMD -MP

.SECONDEXPANSION:
.PRECIOUS: %.c %.o

.PHONY : all clean debug release dbg_prep rel_prep test

all : debug

debug : CFLAGS += -g -O0 -DDEBUG
debug : dbg_prep $(DBG_TARGET)

release : CFLAGS += -O2 -DNDEBUG
release : rel_prep $(REL_TARGET)

test : CFLAGS += -g -O0 -DDEBUG
test : test_prep $(TEST_TARGET)

%/$(TARGET) : $(addprefix %/, $(OBJS))
	$(CC) -o $@ $^

$(TEST_TARGET) : $(addprefix $(TEST_DIR)/, $(TEST_OBJS))
	$(CC) -o $@ $^

%.o : $(SRC_DIR)/$$(*F).c
	$(CC) $(CFLAGS) -MT $@ -MF $(patsubst %.c,$(@D)/.dep/%.d,$(notdir $<)) -c $< -o $@

dbg_prep :
	@mkdir -p $(DBG_DIR)/.dep

rel_prep :
	@mkdir -p $(REL_DIR)/.dep

test_prep :
	@mkdir -p $(TEST_DIR)/.dep

clean :
	@rm -drf $(DBG_DIR) $(REL_DIR) $(TEST_DIR)

-include $(SRCS:%.c=$(DBG_DIR)/.dep/%.d)
-include $(SRCS:%.c=$(REL_DIR)/.dep/%.d)
-include $(SRCS:%.c=$(TEST_DIR)/.dep/%.d)
