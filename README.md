BNS
===

Two-player wand duel. Wands (ESP32-C6 + BNO055) send directions over
ESP-NOW to a receiver ESP32-C6, which forwards them over UART to the
XMC4700 base driving two HUB75 panels.

  base/       XMC4700 Relax Kit game + display (C, ModusToolbox)
  esp/wand/   XIAO ESP32-C6 in each wand (C, ESP-IDF)
  esp/receiver/  XIAO ESP32-C6 at the base (C, ESP-IDF)
  common/     protocol.h, shared by base and esp
  sim/        virtual 64x64 / 128x64 panels running the real base firmware
  reference/  datasheets

Build:

- sim:  cd sim && make run [PANELS=2]   live in terminal; sim/out/frame.png
        make shot SECONDS=2               headless run, then exit

- base: cd base && make getlibs && make build   (make program to flash)
  If ModusToolbox is not in a default location, set CY_TOOLS_PATHS.
  VS Code: run `make vscode` in base/, then open base/.
- esp:  . ~/esp/esp-idf/export.sh (ESP-IDF v6.1.1), then
  cd esp/wand && idf.py build flash monitor
  Wand 2: idf.py menuconfig -> BNS wand -> Wand ID = 2.
