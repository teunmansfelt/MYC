BIN_DIR := bin
BLD_DIR := bin-int
INC_DIR := include
SRC_DIR := src
EX_DIR := examples

CFLAGS := -Wall -Wextra -std=gnu11 -I./$(INC_DIR)
DEFINES := -D_GNU_SOURCE

ifeq ($(filter shared, $(MAKECMDGOALS)),shared)
	DEFINES += -fPIC
endif



.PHONY: all shared
MYC_STATIC_LIB := $(BIN_DIR)/libmyc.a
MYC_SHARED_LIB := $(BIN_DIR)/libmyc.so
MYC_OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BLD_DIR)/%.o, $(shell find $(SRC_DIR) -type f -name *.c))

all: $(MYC_STATIC_LIB)
$(MYC_STATIC_LIB): $(MYC_OBJECTS)
	ar rcs -o $@ $(MYC_OBJECTS)
	@printf "==================================================\ntarget '$@' finished!\n\n"

shared: $(MYC_SHARED_LIB)
$(MYC_SHARED_LIB): $(MYC_OBJECTS)
	cc -shared -o $@ $(MYC_OBJECTS)
	@printf "==================================================\ntarget '$@' finished!\n\n"



$(BLD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	cc $(CFLAGS) $(DEFINES) -o $@ -c $<



.PHONY: examples
examples: $(MYC_STATIC_LIB)
	cc $(CFLAGS) $(DEFINES) -o $(BIN_DIR)/log-example $(EX_DIR)/ex_log.c $(MYC_STATIC_LIB)
	cc $(CFLAGS) $(DEFINES) -o $(BIN_DIR)/memory-example $(EX_DIR)/ex_mem.c $(MYC_STATIC_LIB)
	@printf "==================================================\ntarget '$@' finished!\n\n"


.PHONY: clean
clean:
	rm -f $(BLD_DIR)/*.o