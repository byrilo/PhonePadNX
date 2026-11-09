#include "pad.h"
#include "web_page.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define CLOSE closesocket
#define INVALID INVALID_SOCKET
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
typedef int Socket;
#define CLOSE close
#define INVALID (-1)
#endif

typedef struct {
    Socket fd; char input[2048], header[512]; size_t used, sent, hlen, plen;
    const char *payload; uint64_t started; bool writing,draining;
} Client;
static void nonblock(Socket fd) {
#ifdef _WIN32
    u_long mode=1; ioctlsocket(fd,FIONBIO,&mode);
#else
    fcntl(fd,F_SETFL,O_NONBLOCK);
#endif
}
static void respond(Client *c,int code,const char *type,const char *body) {
    c->plen=strlen(body); c->payload=body; c->sent=0; c->writing=true;
    c->hlen=(size_t)snprintf(c->header,sizeof(c->header),
        "HTTP/1.1 %d Result\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n"
        "Referrer-Policy: no-referrer\r\n\r\n",code,type,c->plen);
    // Send small API responses in one write; do not split headers from "OK".
    if(c->hlen+c->plen<sizeof(c->header)) {
        memcpy(c->header+c->hlen,body,c->plen);
        c->hlen+=c->plen; c->plen=0;
    }
}
static bool would_block(void) {
#ifdef _WIN32
    int error=WSAGetLastError(); return error==WSAEWOULDBLOCK||error==WSAEINTR;
#else
    return errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR;
#endif
}
static bool authorized_header(const char *request) {
    const char *p=strstr(request,"\r\n");
    while(p && p[2]) {
        p+=2; const char *end=strstr(p,"\r\n"); if(!end) return false;
        const char *key="x-phonepad:"; size_t n=strlen(key);
        bool match=(size_t)(end-p)>=n;
        for(size_t i=0;match&&i<n;i++) {
            char c=p[i]; if(c>='A'&&c<='Z') c=(char)(c+32);
            if(c!=key[i]) match=false;
        }
        if(match) {
            const char *v=p+n; while(v<end&&(*v==' '||*v=='\t')) v++;
            return end-v==1&&*v=='1';
        }
        p=end;
    }
    return false;
}
static void route(Client *c,PadSession *session) {
    char method[8],path[1200],version[16];
    if(sscanf(c->input,"%7s %1199s %15s",method,path,version)!=3) {
        respond(c,400,"text/plain","Bad request"); return;
    }
    if(!strcmp(method,"GET")&&(!strcmp(path,"/")||!strncmp(path,"/?",2))) {
        respond(c,200,"text/html; charset=utf-8",web_page); return;
    }
    // A custom header makes cross-origin browser requests require a CORS preflight.
    // No CORS/OPTIONS support is provided. API calls never mutate through GET.
    if(strcmp(method,"POST")||!authorized_header(c->input)) {
        respond(c,403,"text/plain","Forbidden"); return;
    }
    int result=pad_request(session,path,platform_ms());
    if(result==200 && !platform_apply(&session->state)) result=503;
    respond(c,result,"text/plain",result==200?"OK":"Controller unavailable or request rejected");
}
int phonepad_run(PadSession *session,unsigned port) {
#ifdef _WIN32
    WSADATA data; if(WSAStartup(MAKEWORD(2,2),&data)) return 1;
#endif
    Socket listener=socket(AF_INET,SOCK_STREAM,0); if(listener==INVALID) return 2;
    int yes=1; setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,(const char*)&yes,sizeof(yes));
    struct sockaddr_in addr={0}; addr.sin_family=AF_INET; addr.sin_port=htons((unsigned short)port);
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(listener,(struct sockaddr*)&addr,sizeof(addr))||listen(listener,8)) { CLOSE(listener); return 3; }
    nonblock(listener);
    Client clients[8]={0}; for(int i=0;i<8;i++) clients[i].fd=INVALID;
    while(platform_running()) {
        uint64_t now=platform_ms();
        if(pad_expire(session,now)) platform_apply(&session->state);
        platform_tick();
        fd_set reads,writes; FD_ZERO(&reads); FD_ZERO(&writes); FD_SET(listener,&reads);
        Socket maxfd=listener;
        for(int i=0;i<8;i++) if(clients[i].fd!=INVALID) {
            if(now-clients[i].started>(clients[i].draining?100u:1000u)) { CLOSE(clients[i].fd); clients[i].fd=INVALID; continue; }
            if(clients[i].writing) FD_SET(clients[i].fd,&writes); else FD_SET(clients[i].fd,&reads);
            if(clients[i].fd>maxfd) maxfd=clients[i].fd;
        }
        struct timeval timeout={0,5000};
        if(select((int)maxfd+1,&reads,&writes,NULL,&timeout)<0) continue;
        if(FD_ISSET(listener,&reads)) {
            Socket fd=accept(listener,NULL,NULL);
            if(fd!=INVALID) {
                int slot=-1; for(int i=0;i<8;i++) if(clients[i].fd==INVALID) { slot=i; break; }
                if(slot<0) CLOSE(fd);
                else {
                    setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(const char*)&yes,sizeof(yes));
                    nonblock(fd); clients[slot]=(Client){.fd=fd,.started=now};
                }
            }
        }
        for(int i=0;i<8;i++) {
            Client *c=&clients[i]; if(c->fd==INVALID) continue;
            if(!c->writing && FD_ISSET(c->fd,&reads)) {
                if(c->draining) {
                    char discard[2048]; int n=recv(c->fd,discard,sizeof(discard),0);
                    if(n<0&&would_block()) continue;
                    if(n<=0) { CLOSE(c->fd); c->fd=INVALID; }
                    continue;
                }
                int n=recv(c->fd,c->input+c->used,(int)(sizeof(c->input)-c->used-1),0);
                if(n<0&&would_block()) continue;
                if(n<=0) { CLOSE(c->fd); c->fd=INVALID; continue; }
                c->used+=(size_t)n; c->input[c->used]=0;
                if(strstr(c->input,"\r\n\r\n")) route(c,session);
                else if(c->used==sizeof(c->input)-1) respond(c,413,"text/plain","Too large");
            }
            if(c->writing && FD_ISSET(c->fd,&writes)) {
                const char *p; size_t left;
                if(c->sent<c->hlen) { p=c->header+c->sent; left=c->hlen-c->sent; }
                else { p=c->payload+c->sent-c->hlen; left=c->plen-(c->sent-c->hlen); }
                int n=send(c->fd,p,(int)left,0);
                if(n<0&&would_block()) continue;
                if(n>0) c->sent+=(size_t)n;
                if(n<=0) { CLOSE(c->fd); c->fd=INVALID; }
                else if(c->sent==c->hlen+c->plen) {
                    // Half-close and drain unread request data. Closing immediately with
                    // an oversized request still queued can reset TCP and truncate the reply.
#ifdef _WIN32
                    shutdown(c->fd,SD_SEND);
#else
                    shutdown(c->fd,SHUT_WR);
#endif
                    c->writing=false; c->draining=true;
                    c->started=platform_ms();
                }
            }
        }
    }
    for(int i=0;i<8;i++) if(clients[i].fd!=INVALID) CLOSE(clients[i].fd);
    CLOSE(listener);
    return 0;
}
