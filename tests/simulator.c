#include "pad.h"
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
uint64_t platform_ms(void) { return GetTickCount64(); }
#else
#include <time.h>
uint64_t platform_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;
}
#endif
bool platform_apply(const PhonePadState *s) {
    FILE *f=fopen("simulator-state.txt","w");
    if(f) { fprintf(f,"%u %d %d %d %d",s->buttons,s->lx,s->ly,s->rx,s->ry); fclose(f); }
    return true;
}
void platform_tick(void) {}
bool platform_running(void) { return true; }
int main(void) {
    PadSession session; pad_init(&session,"123456");
    puts("SIMULATOR ONLY: http://127.0.0.1:8080/?pin=123456 (no Switch input)");
    return phonepad_run(&session,8080);
}
