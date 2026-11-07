/*
    Project: FreeBASIC C backend regression tests
    File: c-bulk-copy-subobjects.c
    Purpose: Reproduce GCC's merged-address bounds diagnostic on valid record copies.
    Responsibilities: Compare leading-member reads with complete containing-record copies.
    This file intentionally does NOT suppress diagnostics or replace memcpy semantics.
*/
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct { float x, y, z; } Vector;
typedef struct { Vector position; unsigned char payload[132]; } State;
typedef struct { unsigned char prefix[6544]; State state; Vector offset; } Prediction;
typedef Prediction PredictionView __attribute__((aligned(1)));
typedef State StateView __attribute__((aligned(1)));

float copy_state(PredictionView *source, StateView *destination, int copy);

/* The leading member and complete record have the same address. Their copy
   extents remain different even if the optimizer merges those addresses. */
float copy_state(PredictionView *source, StateView *destination, int copy)
{
    Vector saved;
    memcpy(&saved, &source->state.position, sizeof(saved));
    saved.x += source->offset.x;
    saved.y += source->offset.y;
    saved.z += source->offset.z;
    if (copy)
        memcpy(destination, &source->state, sizeof(*destination));
    return saved.x + saved.y + saved.z;
}

int main(void)
{
    Prediction source = {0};
    State destination = {0};
    unsigned int i;
    source.state.position.x = 1.0f;
    source.state.position.y = 2.0f;
    source.state.position.z = 3.0f;
    source.offset.x = 4.0f;
    source.offset.y = 5.0f;
    source.offset.z = 6.0f;
    for (i = 0; i < sizeof(source.state.payload); ++i)
        source.state.payload[i] = (unsigned char)(i + 17);
    if (copy_state(&source, &destination, 1) != 21.0f ||
        memcmp(&source.state, &destination, sizeof(destination)) != 0)
        return 1;
    if (offsetof(Prediction, state) != 6544 || sizeof(State) != 144)
        return 2;
    puts("BULK_SUBOBJECT_COPY=PASS");
    return 0;
}

/* end of c-bulk-copy-subobjects.c */
