# Virtual HUB75 panels running the real firmware (base/src/main.c).
#   make run              64x64 (1 panel), live in the terminal
#   make run PANELS=2     128x64 (2 chained panels)
#   make shot SECONDS=2   run headless for 2 s, then exit (out/frame.png)
#   make watch            like run, but rebuilds + restarts on every save
# Output: out/frame.png (latest frame), out/uart.log (firmware printf)

PANELS  ?= 1
SECONDS ?= 2
SRC_DIR := ../base/src

# all firmware sources except the XMC-only ones: main.c is pulled in via
# firmware_main.c, hub75.c/hub75_dma.c and timer.c are replaced by hub75_sim.c, input.c
# by input_sim.c (keyboard)
FW_SRCS := $(filter-out $(SRC_DIR)/main.c $(SRC_DIR)/display/hub75.c $(SRC_DIR)/display/hub75_dma.c \
             $(SRC_DIR)/timer.c $(SRC_DIR)/input.c, \
             $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c))
SIM_SRCS := sim.c hub75_sim.c input_sim.c render.c firmware_main.c
HEADERS  := $(wildcard *.h stubs/*.h $(SRC_DIR)/*.h $(SRC_DIR)/*/*.h ../common/*.h)

CC     ?= cc
CFLAGS ?= -std=gnu11 -O1 -g -Wall
CFLAGS += -pthread -DHUB75_PANELS=$(PANELS) -DGAME_COUNTDOWN=0 -include stubs/sim_firmware.h \
          -Istubs -I. -I$(SRC_DIR) -I$(SRC_DIR)/assets -I$(SRC_DIR)/display -I$(SRC_DIR)/game -I../common

BIN := build/bns-sim-$(PANELS)p

run: $(BIN)
	@./$(BIN)

shot: $(BIN)
	@./$(BIN) --seconds $(SECONDS)

watch:
	@PANELS=$(PANELS) ./watch.sh

$(BIN): $(SIM_SRCS) $(FW_SRCS) $(HEADERS)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SIM_SRCS) $(FW_SRCS)

# laptop test of the display encoder (base/src/display/hub75_encode.c)
test: build/test_encode
	@./build/test_encode

build/test_encode: test_encode.c $(SRC_DIR)/display/hub75_encode.c $(HEADERS)
	@mkdir -p build
	$(CC) -std=c11 -O1 -Wall -Wextra -DHUB75_PANELS=2 -I$(SRC_DIR)/display -o $@ test_encode.c $(SRC_DIR)/display/hub75_encode.c

clean:
	rm -rf build out

.PHONY: run shot watch test clean
