TARGET:=test
OBJ_USE_CU := test_dev_ywj.o	\
	test_addr_conv_ywj.o	\
	test_addr_io_ywj.o
OBJ_NO_CU := test_geo_ywj.o	\
	test_addr_ywj.o	\
	test_io_issue_ywj.o
TEST_USE_CU:=$(basename $(OBJ_USE_CU))
TEST_NO_CU:=$(basename $(OBJ_NO_CU))

CC:=gcc
CFLAGS:=-I./libs/include -std=c99
LDFLAGSCU:=-L./libs -llightnvm -lcunit
LDFLAGS_:=-L./libs -llightnvm

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

