# Simulator with auto-rebuild on save. `just watch 2` for two chained panels.
watch panels="1":
    make -C sim watch PANELS={{panels}}

# Demo: real wands -> receiver XIAO (USB) -> simulator. Port is auto-detected
# when exactly one /dev/cu.usbmodem* is plugged in.
demo panels="2" port="":
    #!/bin/sh
    port="{{port}}"
    if [ -z "$port" ]; then
        set -- /dev/cu.usbmodem*
        if [ "$#" -ne 1 ] || [ ! -e "$1" ]; then
            echo "found: $*"; echo "pass the receiver's port: just demo {{panels}} /dev/cu.usbmodemXXXX"; exit 1
        fi
        port="$1"
    fi
    echo "receiver on $port"
    make -C sim build/bns-sim-{{panels}}p PANELS={{panels}} -s
    cd sim && ./build/bns-sim-{{panels}}p --serial "$port"

# Flash a board: xmc (kit J-Link on X101), wand or receiver (ESP32-C6 USB, auto port)
flash target="xmc" port="":
    #!/bin/sh
    set -e
    case "{{target}}" in
    xmc)
        cd base
        make build -j8
        make program -j8
        ;;
    wand|receiver)
        port="{{port}}"
        if [ -z "$port" ]; then
            set -- /dev/cu.usbmodem*
            if [ "$#" -ne 1 ] || [ ! -e "$1" ]; then
                echo "found: $*"; echo "pass the port: just flash {{target}} /dev/cu.usbmodemXXXX"; exit 1
            fi
            port="$1"
        fi
        command -v idf.py >/dev/null || . "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh" >/dev/null
        cd esp/{{target}}
        idf.py -p "$port" build flash
        ;;
    *)
        echo "unknown target {{target}}: use xmc, wand or receiver"; exit 1
        ;;
    esac
