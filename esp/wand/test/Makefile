# Laptop test of the wand's shape recogniser against captures/.
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
CFLAGS += -I../main -I../../../common

test_shapes: test_shapes.c ../main/shape.c ../main/shape.h ../main/shape_templates.h
	$(CC) $(CFLAGS) -o $@ test_shapes.c ../main/shape.c -lm
	./test_shapes

.PHONY: test_shapes
