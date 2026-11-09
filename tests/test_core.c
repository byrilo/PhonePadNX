#include "pad.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define ID "0123456789abcdef0123456789abcdef"
#define STATE "/api/state?id=" ID "&seq=%u&b=%u&lx=%d&ly=0&rx=0&ry=0"
int main(void) {
    PadSession s; pad_init(&s,"123456");
    char path[256];
    snprintf(path,sizeof(path),STATE,1u,1u,0);
    assert(pad_request(&s,path,1000)==403);
    assert(pad_request(&s,"/api/claim?id=" ID "&pin=000000",1000)==403);
    assert(pad_request(&s,"/api/claim?id=" ID "&pin=123456",1100)==429);
    assert(pad_request(&s,"/api/claim?id=" ID "&pin=123456",1400)==200);
    snprintf(path,sizeof(path),STATE,1u,65535u,32767);
    assert(pad_request(&s,path,1410)==200 && s.state.lx==32767 && s.state.buttons==65535);
    assert(pad_request(&s,path,1420)==409); // replay
    snprintf(path,sizeof(path),STATE,0u,1u,0); assert(pad_request(&s,path,1430)==409);
    snprintf(path,sizeof(path),STATE,2u,1u,32768); assert(pad_request(&s,path,1440)==400);
    snprintf(path,sizeof(path),STATE,2u,65536u,0); assert(pad_request(&s,path,1440)==400);
    assert(pad_request(&s,"/api/state?id=" ID "&seq=2&b=1&lx=1&ly=0&rx=0&ry=0&b=2",1440)==400);
    assert(pad_expire(&s,1800)); assert(s.state.buttons==0&&s.state.lx==0);
    pad_expire(&s,3000); assert(!s.owner[0]);
    assert(pad_request(&s,"/api/claim?id=" ID "&pin=123456",3001)==200);
    snprintf(path,sizeof(path),STATE,UINT32_MAX,1u,0); assert(pad_request(&s,path,3010)==200);
    snprintf(path,sizeof(path),STATE,0u,0u,0); assert(pad_request(&s,path,3020)==200); // wrap
    assert(pad_request(&s,"/api/state?id=" ID "&seq=1&b=1&lx=--1&ly=0&rx=0&ry=0",3030)==400);
    puts("Core tests passed (auth, bounds, duplicate parameters, ordering, timeout, wrap).");
}
