Optimization
============

XMC4700 features worth using, from the datasheet (`reference/xmc4700/`).
The DMA-driven display is in display-dma.md.

Current numbers (debug serial stats line, 2 panels):
fps 119, display 11% CPU, game 2.0 ms of each 8.4 ms frame, 4096 colors.

## 1. Watchdog, fault handler, bigger stack (reliability)
- Window Watchdog (WDT, RM ch. 9): kick once per game loop; a hang resets the
  board in ~100 ms. Use the pre-warning interrupt to print where it hung.
- HardFault handler: print the faulting PC over debug serial, then reset.
- Stack is 2 KB (build map). printf + game get close; raise to 8 KB.

## 2. Packet CRC with the FCE (reliability)
- Flexible CRC Engine does CRC-8 (SAE J1850), CRC-16 (CCITT), CRC-32 in hardware.
- Add a CRC-8 byte to `bns_packet_t` (protocol v3). ESPs compute it in software,
  the XMC checks it in `base/src/input.c`. Today only the sync byte and range
  checks guard against a corrupted byte casting the wrong spell.

## 3. Non-blocking debug output (one dropped frame per second)
- A stats line is ~75 chars at 115200 baud: 6.5 ms of blocking printf per second.
- Use the USIC transmit FIFO (or DMA) in `_write`, or raise the J-Link VCOM baud.

## 4. Hot code in zero-wait RAM (only if timing jitter shows up)
- Flash runs through an 8 KB instruction cache; the 96 KB program RAM (PSRAM)
  has no wait states. Placing the display interrupt there makes its timing
  deterministic. Not needed today.

## Not worth it
- Hard-float ABI: build uses `-mfloat-abi=softfp` with the FPU; game is integer math.
- EBU, USB, CAN, Ethernet, EtherCAT, ADC/DAC, POSIF: no use in this project
  (CCU8 maybe: hardware OE timing, see display-dma.md).

## Done
- Display in a CCU40 timer interrupt, double buffered (`hub75_present`).
- LED on-time from a timer compare, not a busy-wait; shift clock edges timed on
  the cycle counter; `-O2` for the display and drawing files: display CPU 65% -> 24%.
- No `__WFI()` while waiting for a frame (sleep stops the CCU clock; see bug-log).
- DMA + binary code modulation (display-dma.md): 24% -> 11-14% CPU, 8 -> 4096 colors.
