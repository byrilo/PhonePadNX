#include <switch.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "qrcodegen.h"

static bool signal_module(const char *path) {
    FILE *file=fopen(path,"w");
    if (!file) return false;
    bool wrote=fputs("1",file)>=0;
    return fclose(file)==0 && wrote;
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
    PadState pad; padInitializeDefault(&pad);
    Result network=nifmInitialize(NifmServiceType_User);
    const char *command="";
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 buttons=padGetButtonsDown(&pad);
        if (buttons & HidNpadButton_Plus) break;
        if (buttons & HidNpadButton_A) command=signal_module("sdmc:/config/phonepad/start")?"Start requested":"Cannot write start flag";
        if (buttons & HidNpadButton_X) signal_module("sdmc:/config/phonepad/stop");
        char pin[7]={0};
        unsigned port=8080,result=0;
        FILE *status=fopen("sdmc:/config/phonepad/status.txt","r");
        if (status) {
            if (fscanf(status,"%6[0-9]%u%x",pin,&port,&result)!=3) pin[0]=0;
            fclose(status);
        }
        u32 ip=0;
        if (R_SUCCEEDED(network)) nifmGetCurrentIpAddress(&ip);
        struct in_addr address={.s_addr=ip};
        char url[128];
        snprintf(url,sizeof(url),"http://%s:%u/?pin=%s",inet_ntoa(address),port,pin);
        printf("\x1b[2J\x1b[1;2HPhonePadNX 0.3.2 test");
        if (ip && pin[0] && result==0) {
            printf("\x1b[2;2H%s",url);
            draw_qr(url);
        }
        printf("\x1b[6;2HBackend: %08X",result);
        printf("\x1b[7;2H%s",command);
        printf("\x1b[8;2H%s",!pin[0]?"No module status":result==0?"Server listening":result==0xf0000010?"Server idle":result==0xf0000011?"Starting server":"Server/controller error");
        printf("\x1b[9;2HA  Start server (30 minutes)");
        printf("\x1b[11;2HX  Stop server");
        printf("\x1b[13;2H+  Exit to launch a game");
        printf("\x1b[16;2HConnect phone after pressing A.");
        if (!pin[0]) printf("\x1b[20;2HModule status unavailable.");
        consoleUpdate(NULL);
        svcSleepThread(100000000);
    }
    if (R_SUCCEEDED(network)) nifmExit();
    consoleExit(NULL);
    return 0;
}
