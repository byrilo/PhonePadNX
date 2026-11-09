#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "qrcodegen.h"
#include "pad.h"

static atomic_bool running=false;
static atomic_bool finished=false;
static atomic_int server_error=0;
static Mutex state_mutex;
static PhonePadState received;
static unsigned received_count;
static uint64_t received_at;
static PadSession session;
static Thread server_thread;
static unsigned char server_stack[0x10000] __attribute__((aligned(0x1000)));

uint64_t platform_ms(void) { return armTicksToNs(armGetSystemTick())/1000000; }
bool platform_running(void) { return atomic_load(&running); }
void platform_tick(void) {}
bool platform_apply(const PhonePadState *state) {
    mutexLock(&state_mutex);
    received=*state; received_count++; received_at=platform_ms();
    mutexUnlock(&state_mutex);
    // Diagnostic only. No virtual controllers or game input.
    return true;
}
static void log_event(const char *event,unsigned code) {
    FILE *f=fopen("sdmc:/config/phonepad/diagnostic.log","a");
    if(f) { fprintf(f,"%llu %s %08X\n",(unsigned long long)platform_ms(),event,code); fclose(f); }
}
static void serve(void *arg) {
    (void)arg;
    int error=phonepad_run(&session,8080);
    atomic_store(&server_error,error);
    atomic_store(&finished,true);
    log_event("server stopped",(unsigned)error);
}
static void draw_qr(const char *url) {
    uint8_t temp[qrcodegen_BUFFER_LEN_FOR_VERSION(4)],qr[qrcodegen_BUFFER_LEN_FOR_VERSION(4)];
    if(!qrcodegen_encodeText(url,temp,qr,qrcodegen_Ecc_LOW,1,4,qrcodegen_Mask_AUTO,true)) return;
    int size=qrcodegen_getSize(qr);
    for(int y=-4;y<size+4;y++) {
        printf("\x1b[%d;39H",y+8);
        for(int x=-4;x<size+4;x++) printf(qrcodegen_getModule(qr,x,y)?"\x1b[40m ":"\x1b[47m ");
        printf("\x1b[0m");
    }
}
int main(void) {
    consoleInit(NULL);
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);
    PadState physical; padInitializeDefault(&physical);
    mkdir("sdmc:/config",0777); mkdir("sdmc:/config/phonepad",0777);
    log_event("diagnostic start",0);
    unsigned random_value; randomGet(&random_value,sizeof(random_value));
    char pin[7]; snprintf(pin,sizeof(pin),"%06u",random_value%1000000);
    pad_init(&session,pin);
    mutexInit(&state_mutex);
    Result socket_rc=socketInitializeDefault();
    Result nifm_rc=nifmInitialize(NifmServiceType_User);
    Result thread_rc=0;
    bool created=false,started=false;
    if(R_SUCCEEDED(socket_rc)) {
        thread_rc=threadCreate(&server_thread,serve,NULL,server_stack,sizeof(server_stack),0x2c,-2);
        if(R_SUCCEEDED(thread_rc)) {
            created=true; atomic_store(&running,true);
            thread_rc=threadStart(&server_thread);
            if(R_SUCCEEDED(thread_rc)) started=true;
            else atomic_store(&running,false);
        }
    }
    log_event("sockets",socket_rc); log_event("thread",thread_rc);
    while(appletMainLoop()) {
        padUpdate(&physical);
        if(padGetButtonsDown(&physical)&HidNpadButton_Plus) break;
        u32 ip=0;
        if(R_SUCCEEDED(nifm_rc)) nifmGetCurrentIpAddress(&ip);
        struct in_addr address={.s_addr=ip}; char url[128];
        snprintf(url,sizeof(url),"http://%s:8080/?pin=%s",inet_ntoa(address),pin);
        mutexLock(&state_mutex);
        PhonePadState state=received; unsigned count=received_count; uint64_t seen=received_at;
        mutexUnlock(&state_mutex);
        printf("\x1b[2J\x1b[1;2HPhonePadNX Check 0.2.0");
        printf("\x1b[2;2H%s",url);
        printf("\x1b[5;2HPIN: %s",pin);
        printf("\x1b[7;2HSockets: %08X",socket_rc);
        printf("\x1b[8;2HNetwork: %08X",nifm_rc);
        printf("\x1b[9;2HThread:  %08X",thread_rc);
        printf("\x1b[11;2HServer: %s (%d)",!started?"not started":atomic_load(&finished)?"stopped":"running",atomic_load(&server_error));
        printf("\x1b[14;2HPackets: %u",count);
        printf("\x1b[16;2HButtons: %04X",state.buttons);
        printf("\x1b[18;2HLeft:  %6d %6d",state.lx,state.ly);
        printf("\x1b[20;2HRight: %6d %6d",state.rx,state.ry);
        if(count) printf("\x1b[22;2HLast input: %llu ms ago",(unsigned long long)(platform_ms()-seen));
        printf("\x1b[26;2HNETWORK TEST ONLY");
        printf("\x1b[28;2HDoes not control games.");
        printf("\x1b[30;2HKeep this screen open.");
        printf("\x1b[32;2H+ Stop and exit");
        if(ip&&started&&!atomic_load(&finished)) draw_qr(url);
        consoleUpdate(NULL); svcSleepThread(50000000);
    }
    atomic_store(&running,false);
    if(started) threadWaitForExit(&server_thread);
    if(created) threadClose(&server_thread);
    if(R_SUCCEEDED(nifm_rc)) nifmExit();
    if(R_SUCCEEDED(socket_rc)) socketExit();
    log_event("diagnostic exit",0);
    consoleExit(NULL); return 0;
}
