#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef struct { uint32_t buttons; int lx,ly,rx,ry; } PhonePadState;
typedef struct {
    char pin[7], owner[33];
    PhonePadState state;
    uint64_t seen, claim_after;
    uint32_t sequence;
    bool has_sequence;
} PadSession;
void pad_init(PadSession *s, const char *pin);
int pad_request(PadSession *s, const char *path, uint64_t now);
bool pad_expire(PadSession *s, uint64_t now);
int phonepad_run(PadSession *session, unsigned port);
uint64_t platform_ms(void);
bool platform_apply(const PhonePadState *state);
void platform_tick(void);
