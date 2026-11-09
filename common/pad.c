#include "pad.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

void pad_init(PadSession *s, const char *pin) {
    memset(s,0,sizeof(*s)); snprintf(s->pin,sizeof(s->pin),"%.6s",pin);
}
static bool param(const char *path,const char *key,char *out,size_t size) {
    const char *p=strchr(path,'?'); if(!p) return false;
    bool found=false; size_t kl=strlen(key);
    for(p++;*p;) {
        const char *end=strchr(p,'&'); if(!end) end=p+strlen(p);
        if((size_t)(end-p)>kl && !strncmp(p,key,kl) && p[kl]=='=') {
            size_t n=(size_t)(end-p)-kl-1;
            if(found || n==0 || n>=size) return false;
            memcpy(out,p+kl+1,n); out[n]=0; found=true;
        }
        p=*end?end+1:end;
    }
    return found;
}
static bool number(const char *path,const char *key,long long lo,long long hi,long long *out) {
    char tmp[32],*end; if(!param(path,key,tmp,sizeof(tmp))) return false;
    const char *p=tmp; if(*p=='-') p++;
    if(!*p) return false;
    for(;*p;p++) if(*p<'0'||*p>'9') return false;
    errno=0; long long v=strtoll(tmp,&end,10);
    if(errno||*end||v<lo||v>hi) return false;
    *out=v; return true;
}
bool pad_expire(PadSession *s,uint64_t now) {
    if(s->owner[0] && now-s->seen>350) {
        bool changed=s->state.buttons||s->state.lx||s->state.ly||s->state.rx||s->state.ry;
        memset(&s->state,0,sizeof(s->state));
        if(now-s->seen>1500) { s->owner[0]=0; s->has_sequence=false; }
        return changed;
    }
    return false;
}
int pad_request(PadSession *s,const char *path,uint64_t now) {
    char id[33]; if(!param(path,"id",id,sizeof(id))||strlen(id)!=32) return 400;
    for(int i=0;i<32;i++) if(!((id[i]>='0'&&id[i]<='9')||(id[i]>='a'&&id[i]<='f'))) return 400;
    if(!strncmp(path,"/api/claim?",11)) {
        if(now<s->claim_after) return 429;
        s->claim_after=now+300;
        char pin[7]; if(!param(path,"pin",pin,sizeof(pin))||strcmp(pin,s->pin)) return 403;
        if(s->owner[0] && strcmp(s->owner,id)) return 409;
        if(!s->owner[0]) { memcpy(s->owner,id,33); s->has_sequence=false; }
        s->seen=now; return 200;
    }
    if(strncmp(path,"/api/state?",11)) return 404;
    if(!s->owner[0]||strcmp(s->owner,id)) return 403;
    long long b,lx,ly,rx,ry,seq;
    if(!number(path,"b",0,0xffff,&b)||!number(path,"lx",-32767,32767,&lx)||
       !number(path,"ly",-32767,32767,&ly)||!number(path,"rx",-32767,32767,&rx)||
       !number(path,"ry",-32767,32767,&ry)||!number(path,"seq",0,UINT32_MAX,&seq)) return 400;
    // Reject replayed/reordered states, while allowing the sequence counter to wrap.
    uint32_t delta=(uint32_t)seq-s->sequence;
    if(s->has_sequence && (delta==0||delta>=0x80000000u)) return 409;
    s->sequence=(uint32_t)seq; s->has_sequence=true;
    s->state=(PhonePadState){(uint32_t)b,(int)lx,(int)ly,(int)rx,(int)ry};
    s->seen=now; return 200;
}
