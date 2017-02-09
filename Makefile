TARGET:=test
SRC_USE_CU := $(shell ls test_*.c)
SRC_NO_CU := $(shell ls *_issue.c)
OBJ_USE_CU := $(patsubst %.c,%.o,$(SRC_USE_CU))
OBJ_NO_CU := $(patsubst %.c,%.o,$(SRC_NO_CU))
TEST_USE_CU:=$(basename $(OBJ_USE_CU))
TEST_NO_CU:=$(basename $(OBJ_NO_CU))

CC:=gcc
CFLAGS:=-I./include -I. -std=c99
LDFLAGSCU:= -llightnvm -lcunit
LDFLAGS_:= -llightnvm

%.o:%.c
	$(CC) $(CFLAGS) -c $<
all:
	@echo "TEST Tagets:"
	@echo $(TEST_NO_CU) $(TEST_USE_CU)

ALLOBJ: $(OBJ_USE_CU) $(OBJ_NO_CU)

$(TEST_USE_CU): ALLOBJ
	$(CC) $(addsuffix .o, $@) -o $(TARGET) $(LDFLAGSCU)

$(TEST_NO_CU): ALLOBJ
	$(CC) $(addsuffix .o, $@) -o $(TARGET) $(LDFLAGS_)

.PHONY:clean
clean:
	rm -f *.o $(TARGET)

