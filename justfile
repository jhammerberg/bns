# Simulator with auto-rebuild on save. `just watch 2` for two chained panels.
watch panels="1":
    make -C sim watch PANELS={{panels}}
