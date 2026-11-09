#error "Legacy sysmodule withdrawn after console reboots. Do not build or install."
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pad.h"

u32 __nx_applet_type=AppletType_None;
u32 __nx_fs_num_sessions=1;
static PadSession session;
static HiddbgHdlsHandle handle;
static HiddbgHdlsSessionId hdls;
static bool attached, hdls_ready, dbg_ready;
static Result last_result=0xffffffff;
static unsigned char work_buffer[0x1000] __attribute__((aligned(0x1000)));

void __libnx_initheap(void) {
    static unsigned char heap[0x800000];
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
    state.battery_level=4; state.flags=1; state.buttons=s->buttons;
    state.analog_stick_l=(HidAnalogStickState){s->lx,s->ly};
    state.analog_stick_r=(HidAnalogStickState){s->rx,s->ry};
    rc=hiddbgSetHdlsState(handle,&state);
    if(R_FAILED(rc)) { hiddbgDetachHdlsVirtualDevice(handle); attached=false; }
done:
    status(rc); return R_SUCCEEDED(rc);
}
void platform_tick(void) {
    if(attached&&!session.owner[0]) { hiddbgDetachHdlsVirtualDevice(handle); attached=false; }
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
    while(R_FAILED(rc=socketInitialize(&config))) { status(rc); svcSleepThread(1000000000); }
    status(0);
    for(;;) {
        int err=phonepad_run(&session,8080);
        status(0xf0000000u+(unsigned)err); svcSleepThread(1000000000);
    }
}
