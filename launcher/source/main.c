#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "qrcodegen.h"

static void draw_qr(const char *url) {
    uint8_t temp[qrcodegen_BUFFER_LEN_FOR_VERSION(4)], qr[qrcodegen_BUFFER_LEN_FOR_VERSION(4)];
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
    socketInitializeDefault(); nifmInitialize(NifmServiceType_User);
    while(appletMainLoop()) {
        padUpdate(&pad); if(padGetButtonsDown(&pad)&HidNpadButton_Plus) break;
        char pin[7]="------"; unsigned port=8080,result=0xffffffff;
        FILE *f=fopen("sdmc:/config/phonepad/status.txt","r");
        if(f) { if(fscanf(f,"%6s\n%u\n%x",pin,&port,&result)!=3) result=0xffffffff; fclose(f); }
        u32 ip=0; nifmGetCurrentIpAddress(&ip);
        struct in_addr a={.s_addr=ip}; char url[128];
        snprintf(url,sizeof(url),"http://%s:%u/?pin=%s",inet_ntoa(a),port,pin);
        printf("\x1b[2J\x1b[1;2HPhonePadNX 0.1 - experimental");
        printf("\x1b[2;2H%s",url);
        printf("\x1b[5;2HPIN: %s",pin);
        printf("\x1b[7;2HBackend result: %08X",result);
        printf("\x1b[10;2HSame Wi-Fi on both devices.");
        printf("\x1b[12;2HScan QR or open the address.");
        printf("\x1b[14;2HTap Connect on your phone.");
        printf("\x1b[16;2HExit this app, open a game.");
        printf("\x1b[19;2HControllers > Change Grip/Order:");
        printf("\x1b[21;2Hpress L + R on the phone.");
        printf("\x1b[25;2H+ Exit");
        printf("\x1b[29;2HNo PIN? Install module, reboot.");
        printf("\x1b[31;2HStatus file can be stale.");
        if(ip && pin[0]!='-') draw_qr(url);
        consoleUpdate(NULL);
        svcSleepThread(100000000);
    }
    nifmExit(); socketExit(); consoleExit(NULL); return 0;
}
