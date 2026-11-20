#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "pad.h"

u32 __nx_applet_type=AppletType_None;
u32 __nx_fs_num_sessions=1;
static PadSession session;
static HiddbgHdlsHandle handle;
static HiddbgHdlsSessionId hdls;
static bool attached, hdls_ready, dbg_ready;
static bool listening;
static uint64_t deadline, next_stop_check;
static Result last_result=0xffffffff;
static unsigned char work_buffer[0x1000] __attribute__((aligned(0x1000)));

void __libnx_initheap(void) {
    // Keep the background module's heap bounded on Horizon 23.x.
    static unsigned char heap[0x100000] __attribute__((aligned(0x1000)));
    extern void *fake_heap_start,*fake_heap_end;
    fake_heap_start=heap; fake_heap_end=heap+sizeof(heap);
}
void __appInit(void) {
    Result rc=smInitialize(); if(R_FAILED(rc)) diagAbortWithResult(rc);
    if(R_SUCCEEDED(setsysInitialize())) {
        SetSysFirmwareVersion fw;
        if(R_SUCCEEDED(setsysGetFirmwareVersion(&fw))) hosversionSet(MAKEHOSVERSION(fw.major,fw.minor,fw.micro));
        setsysExit();
    }
    rc=fsInitialize(); if(R_FAILED(rc)) diagAbortWithResult(rc);
    fsdevMountSdmc();
    // SM stays open: hid:dbg and BSD may only become available after boot.
}
void __appExit(void) {
    if(attached) hiddbgDetachHdlsVirtualDevice(handle);
    if(hdls_ready) hiddbgReleaseHdlsWorkBuffer(hdls);
    if(dbg_ready) hiddbgExit();
    fsdevUnmountAll(); fsExit(); smExit();
}
uint64_t platform_ms(void) { return armTicksToNs(armGetSystemTick())/1000000; }
static void status(Result rc) {
    if(last_result==rc) return;
    last_result=rc;
    FILE *f=fopen("sdmc:/config/phonepad/status.txt","w");
    if(f) { fprintf(f,"%s\n8080\n%08X\n",session.pin,rc); fclose(f); }
}
void platform_listening(void) { status(0); }

bool platform_apply(const PhonePadState *s) {
    Result rc=0;
    if(!dbg_ready) { rc=hiddbgInitialize(); if(R_FAILED(rc)) goto done; dbg_ready=true; }
    if(!hdls_ready) {
        rc=hiddbgAttachHdlsWorkBuffer(&hdls,work_buffer,sizeof(work_buffer));
        if(R_FAILED(rc)) goto done;
        hdls_ready=true;
    }
    if(!attached) {
        HiddbgHdlsDeviceInfo info={0};
        info.deviceType=HidDeviceType_FullKey3;
        info.npadInterfaceType=HidNpadInterfaceType_Bluetooth;
        info.singleColorBody=0x273449; info.singleColorButtons=0xf5f7fa;
        rc=hiddbgAttachHdlsVirtualDevice(&handle,&info);
        if(R_FAILED(rc)) goto done;
        attached=true;
    }
    HiddbgHdlsState state={0};
    state.battery_level=4; state.flags=1; state.buttons=s->buttons & 0xffffu;
    if (s->buttons & PHONEPAD_HOME) state.buttons |= HiddbgNpadButton_Home;
    state.analog_stick_l=(HidAnalogStickState){s->lx,s->ly};
    state.analog_stick_r=(HidAnalogStickState){s->rx,s->ry};
    rc=hiddbgSetHdlsState(handle,&state);
    if(R_FAILED(rc)) { hiddbgDetachHdlsVirtualDevice(handle); attached=false; }
done:
    status(rc); return R_SUCCEEDED(rc);
}
bool platform_running(void) {
    uint64_t now=platform_ms();
    if (!listening || now>=deadline) return false;
    if (now<next_stop_check) return true;
    next_stop_check=now+150;
    return access("sdmc:/config/phonepad/stop",F_OK)!=0;
}
void platform_tick(void) {
    if(attached&&!session.owner[0]) { hiddbgDetachHdlsVirtualDevice(handle); attached=false; }
    if(!session.owner[0] && hdls_ready) {
        hiddbgReleaseHdlsWorkBuffer(hdls);
        hdls_ready=false; hiddbgExit(); dbg_ready=false;
    }
}
int main(void) {
    mkdir("sdmc:/config",0777); mkdir("sdmc:/config/phonepad",0777);
    char pin[7]={0};
    FILE *f=fopen("sdmc:/config/phonepad/pin.txt","r");
    if(f) { if(fscanf(f,"%6[0-9]",pin)!=1) pin[0]=0; fclose(f); }
    if(strlen(pin)!=6) {
        unsigned value; randomGet(&value,sizeof(value));
        snprintf(pin,sizeof(pin),"%06u",value%1000000);
        f=fopen("sdmc:/config/phonepad/pin.txt","w");
        if(f) { fprintf(f,"%s\n",pin); fclose(f); }
    }
    pad_init(&session,pin);
    SocketInitConfig config=*socketGetDefaultInitConfig();
    config.tcp_tx_buf_size=config.tcp_rx_buf_size=0x4000;
    config.tcp_tx_buf_max_size=config.tcp_rx_buf_max_size=0;
    config.sb_efficiency=2; config.num_bsd_sessions=1;
    Result rc;

    unlink("sdmc:/config/phonepad/start");
    status(0xf0000010);
    for (;;) {
        if (unlink("sdmc:/config/phonepad/start")==0) {
            unlink("sdmc:/config/phonepad/stop");
            pad_init(&session,pin);
            deadline=platform_ms()+30*60*1000;
            rc=socketInitialize(&config);
            if (R_FAILED(rc)) { status(rc); continue; }
            status(0xf0000011);
            next_stop_check=0;
            listening=true;
            int err=phonepad_run(&session,8080);
            listening=false;
            socketExit();
            pad_init(&session,pin);
            platform_tick();
            status(err ? 0xf0000000u+(unsigned)err : 0xf0000010);
        }
        svcSleepThread(100000000);
    }
}
