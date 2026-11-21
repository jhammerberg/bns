#!/bin/sh
# Rebuild and restart the simulator whenever a .c/.h file under base/src,
# common/ or sim/ is saved. Like pressing reset on the board: the firmware
# starts over from main(). A failed build keeps the error on screen until
# the next save. Ctrl-C to quit.
#   make watch [PANELS=2]

cd "$(dirname "$0")" || exit 1
PANELS=${PANELS:-1}
BIN=build/bns-sim-${PANELS}p
STAMP=build/.watch-stamp
WATCH="../base/src ../common ."
pid=

start() {
    printf '\033[2J\033[H' # clear screen
    if make -s "$BIN" PANELS="$PANELS"; then
        ./"$BIN" </dev/tty & # background jobs get /dev/null as stdin otherwise
        pid=$!
    else
        printf '\nbuild failed: fix and save to retry\n'
    fi
}

stop() {
    if [ -n "$pid" ]; then
        kill "$pid" 2>/dev/null
        wait "$pid" 2>/dev/null
        pid=
    fi
}

trap 'stop; printf "\033[0m\033[?25h\n"; exit 0' INT TERM

mkdir -p build
touch "$STAMP"
start
while sleep 0.3; do
    changed=$(find $WATCH -newer "$STAMP" \( -name '*.c' -o -name '*.h' \) \
              -not -path './build/*' -print 2>/dev/null | head -1)
    if [ -n "$changed" ]; then
        touch "$STAMP" # before building, so saves during the build are caught next round
        stop
        start
    fi
done
