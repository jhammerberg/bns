Hardware reference
==================

Primary sources for our hardware.
  xmc4700/       main MCU: datasheet, reference manual, Relax Kit manual, errata
  xmc4200/       wand MCU: datasheet, reference manual, errata (steps AA, BA)
  bno055/        wand IMU: Bosch datasheet
  xiao-esp32c6/  radio: ESP32-C6 datasheet, XIAO schematic/pinout/wiki, ESP-NOW guide
  hub75-panel/   P3 64x64 panel: FM6124 driver datasheet (Chinese)


- Panels: generic HUB75, FM6124 drivers, no init sequence. We can chain both for
  (128x64). Own 5 V supply, ~4 A each, common ground.
