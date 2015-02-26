SRC = utf8.c
HDR = utf8.h
TEST = test.c

OBJ := $(SRC:.c=.o)
LIB = libutf8.o

#LIBS=
CFLAGS+=-g  #-I$(dir $(LIBS))

.PHONY: all clean test

all: $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $?

$(LIBS):
	$(foreach var,$@, $(MAKE) $(MAKEFLAGS) -C $(dir $(var));)

$(LIB): $(OBJ) $(LIBS)
	$(LD) -o $@ -r $^

clean:
	$(RM) $(LIB) $(OBJ) $(TEST:.c=.o)
	$(foreach var,$(LIBS), $(MAKE) $(MAKEFLAGS) -C $(dir $(var)) clean;)

# run tests
test: $(TEST:.c=.o) $(LIB)
	$(CC) $(CFLAGS) -o "test" $^
	./test
