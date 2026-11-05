// 本地真实HTTP服务器与共享令牌的并发回归，不依赖公网、Python或外部服务。
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <psapi.h>
typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;
#include <lib2.h>
#include <lang.h>

static PLIB_INFO info;
static volatile LONG failures=0, runtimeErrors=0;
static void Check(bool ok,const char* what) { if(!ok) { InterlockedIncrement(&failures); printf("[FAIL] %s\n",what); fflush(stdout); } }
static DWORD WINAPI Notify(INT msg,DWORD x,DWORD)
{
    if(msg==NRS_MALLOC)return (DWORD)malloc(x?x:1);
    if(msg==NRS_MFREE) {free((void*)x);return 0;}
    if(msg==NRS_RUNTIME_ERR) {InterlockedIncrement(&runtimeErrors);printf("[ERROR] %s\n",(char*)x);}
    return 0;
}
static int Type(const char* name)
{
    for(int i=0;i<info->m_nDataTypeCount;i++)if(!strcmp((char*)info->m_pDataType[i].m_szName,name))return i;
    Check(false,"type not found");ExitProcess(2);return -1;
}
static int Cmd(int type,const char* name)
{
    LIB_DATA_TYPE_INFO& t=info->m_pDataType[type];
    for(int i=0;i<t.m_nCmdCount;i++) {int idx=t.m_pnCmdsIndex[i];if(!strcmp((char*)info->m_pBeginCmdInfo[idx].m_szName,name))return idx;}
    Check(false,name);ExitProcess(2);return -1;
}
static int Life(int type,WORD flag)
{
    LIB_DATA_TYPE_INFO& t=info->m_pDataType[type];
    for(int i=0;i<t.m_nCmdCount;i++)if(info->m_pBeginCmdInfo[t.m_pnCmdsIndex[i]].m_wState&flag)return t.m_pnCmdsIndex[i];
    Check(false,"missing lifecycle");ExitProcess(2);return -1;
}
static MDATA_INF Arg(int value) {MDATA_INF a={0};a.m_dtDataType=SDT_INT;a.m_int=value;return a;}
struct Obj {INT slot;int type;};
static MDATA_INF Self(Obj* o) {MDATA_INF a={0};a.m_dtDataType=MAKELONG(o->type+1,0);a.m_pCompoundData=&o->slot;return a;}
static MDATA_INF Reference(Obj* o,void** data) {MDATA_INF a=Self(o);*data=&o->slot;a.m_ppCompoundData=data;return a;}
static MDATA_INF Call(int cmd,MDATA_INF* a,int n) {MDATA_INF r={0};info->m_pCmdsFunc[cmd](&r,n,a);return r;}
static Obj New(int type) {Obj o={0,type};MDATA_INF r=Call(Life(type,CT_IS_OBJ_CONSTURCT_CMD),NULL,0);o.slot=r.m_int;Check(o.slot!=0,"constructor");return o;}
static void Drop(Obj* o) {MDATA_INF a=Self(o);Call(Life(o->type,CT_IS_OBJ_FREE_CMD),&a,1);}
static Obj Copy(Obj* src) {Obj dst={0x5a5a5a5a,src->type};MDATA_INF a[]={Self(&dst),Self(src)};Call(Life(src->type,CT_IS_OBJ_COPY_CMD),a,2);return dst;}
static int Op(Obj* o,const char* name) {MDATA_INF a=Self(o);return Call(Cmd(o->type,name),&a,1).m_int;}
static int Wait(Obj* o,const char* name,int ms) {MDATA_INF a[]={Self(o),Arg(ms)};return Call(Cmd(o->type,name),a,2).m_int;}
static void Bind(Obj* request,Obj* token) {void* data;MDATA_INF a[]={Self(request),Reference(token,&data)};Check(Call(Cmd(request->type,"绑定取消令牌"),a,2).m_bool==BL_TRUE,"bind token");}
static void Join(HANDLE thread,DWORD ms=6000) {if(WaitForSingleObject(thread,ms)!=WAIT_OBJECT_0) {printf("[FAIL] worker timed out\n");ExitProcess(3);}CloseHandle(thread);}

struct WaitTask {Obj* obj;const char* method;int result;HANDLE entered;};
static DWORD WINAPI WaitWorker(void* raw) {WaitTask* t=(WaitTask*)raw;SetEvent(t->entered);t->result=Wait(t->obj,t->method,-1);return 0;}
static void TestWait(Obj* t,const char* method,const char* trigger,bool expected)
{
    WaitTask task={t,method,-99,CreateEvent(NULL,TRUE,FALSE,NULL)};
    HANDLE th=CreateThread(NULL,0,WaitWorker,&task,0,NULL);
    Check(WaitForSingleObject(task.entered,1000)==WAIT_OBJECT_0,"wait thread started");
    Check(WaitForSingleObject(th,40)==WAIT_TIMEOUT,"wait blocks before signal");
    Op(t,trigger);Join(th);Check((task.result!=0)==expected,"wait wake result");CloseHandle(task.entered);
}
struct Stress {Obj obj;int mode;};
static DWORD WINAPI StressWorker(void* raw)
{
    Stress* t=(Stress*)raw;
    for(int i=0;i<3000;i++)
    {
        if(t->mode==0) {Op(&t->obj,"暂停");Op(&t->obj,"恢复");}
        else {Wait(&t->obj,"等待继续",1);Wait(&t->obj,"等待取消",0);Op(&t->obj,"是否已取消");Op(&t->obj,"是否已暂停");}
    }
    return 0;
}
struct RaceTask {Obj* token;HANDLE gate;int action;};
static DWORD WINAPI RaceWorker(void* raw)
{
    RaceTask* task=(RaceTask*)raw;WaitForSingleObject(task->gate,INFINITE);
    if(task->action==0)Op(task->token,"暂停");
    else if(task->action==1)Op(task->token,"恢复");
    else if(task->action==2)Op(task->token,"取消");
    else Wait(task->token,"等待继续",-1);
    return 0;
}
static void TestToken()
{
    int type=Type("取消令牌");Obj t=New(type);
    Check(Op(&t,"是否已创建")!=0,"created initially");Check(Op(&t,"是否已取消")==0,"not cancelled initially");
    Check(Wait(&t,"等待继续",0)!=0,"running immediately");Check(Wait(&t,"等待取消",15)==0,"cancel timeout");
    Obj copy=Copy(&t);Check(copy.slot==t.slot,"copy shares state");
    Op(&t,"暂停");Check(Op(&copy,"是否已暂停")!=0,"shared pause");Check(Wait(&copy,"等待继续",10)==0,"paused timeout");
    TestWait(&copy,"等待继续","恢复",true);
    Op(&copy,"暂停");TestWait(&t,"等待继续","取消",false);
    Check(Op(&copy,"是否已取消")!=0,"shared cancellation");Check(Op(&copy,"是否已暂停")==0,"cancel beats pause");
    Check(Op(&copy,"恢复")==0 && Op(&copy,"暂停")==0,"cancel irreversible");
    Check(Op(&copy,"取消")!=0 && Op(&copy,"取消")!=0,"cancel idempotent");
    Check(Op(&t,"创建")!=0,"create cancelled token again");Check(t.slot!=copy.slot,"new round independent");Check(Op(&copy,"是否已取消")!=0,"old generation stays cancelled");
    Check(Op(&t,"是否已取消")==0,"new generation running");TestWait(&t,"等待取消","取消",true);
    Drop(&copy);Op(&t,"创建");copy=Copy(&t);Drop(&t);
    Check(Op(&copy,"是否已创建")!=0,"copy survives original destruction");Drop(&copy);
    Check(Op(&t,"是否已创建")==0,"destroyed query");Op(&t,"创建");Op(&t,"暂停");Obj paused=Copy(&t);
    Check(Op(&t,"创建")!=0,"recreate paused token");Check(t.slot!=paused.slot && Op(&t,"是否已暂停")==0 && Op(&t,"是否已取消")==0,"create resets both states");
    Check(Op(&paused,"是否已暂停")!=0,"create preserves old paused snapshot");Op(&paused,"取消");Drop(&paused);
    Stress tasks[16];HANDLE threads[16];
    for(int i=0;i<16;i++) {tasks[i].obj=Copy(&t);tasks[i].mode=i%2;threads[i]=CreateThread(NULL,0,StressWorker,&tasks[i],0,NULL);}
    for(int i=0;i<16;i++) {Join(threads[i],30000);Drop(&tasks[i].obj);}
    Op(&t,"暂停");
    WaitTask waiters[16];
    for(int i=0;i<16;i++) {waiters[i].obj=&t;waiters[i].method=i%2?"等待继续":"等待取消";waiters[i].result=-99;waiters[i].entered=CreateEvent(NULL,TRUE,FALSE,NULL);threads[i]=CreateThread(NULL,0,WaitWorker,&waiters[i],0,NULL);}
    Sleep(30);Op(&t,"取消");
    for(int i=0;i<16;i++) {Join(threads[i]);Check((waiters[i].result!=0)==(i%2==0),"broadcast cancels all waiters");CloseHandle(waiters[i].entered);}
    Drop(&t);
    for(int round=0;round<200;round++)
    {
        t=New(type);HANDLE gate=CreateEvent(NULL,TRUE,FALSE,NULL);RaceTask race[4];HANDLE workers[4];
        for(int i=0;i<4;i++) {race[i].token=&t;race[i].gate=gate;race[i].action=i;workers[i]=CreateThread(NULL,0,RaceWorker,&race[i],0,NULL);}
        SetEvent(gate);for(int i=0;i<4;i++)Join(workers[i]);
        Check(Op(&t,"是否已取消")!=0 && Op(&t,"是否已暂停")==0,"cancel wins pause/resume race");CloseHandle(gate);Drop(&t);
    }
    printf("[OK] token state/copy/generations/waits; 16 workers x 3000 contention loops\n");
    printf("[OK] 200 simultaneous pause/resume/cancel/wait races\n");
}

static HANDLE serverStop;
static SOCKET listener=INVALID_SOCKET;
static volatile LONG clients=0, hits=0, tlsHits=0;
static int port=0;
static bool SendAll(SOCKET sock,const char* p,int n) {while(n>0) {int x=send(sock,p,n,0);if(x<=0)return false;p+=x;n-=x;}return true;}
static DWORD WINAPI Client(void* raw)
{
    SOCKET sock=(SOCKET)raw;char buffer[16384];int used=0;
    DWORD timeout=500;setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(timeout));
    while(used<(int)sizeof(buffer)-1) {int n=recv(sock,buffer+used,sizeof(buffer)-1-used,0);if(n<=0)break;used+=n;buffer[used]=0;if((BYTE)buffer[0]==0x16 || strstr(buffer,"\r\n\r\n"))break;}
    if(used>0 && (BYTE)buffer[0]==0x16) {InterlockedIncrement(&tlsHits);WaitForSingleObject(serverStop,1500);}
    else if(used>0 && strstr(buffer,"\r\n\r\n"))
    {
        InterlockedIncrement(&hits);
        if(strstr(buffer," /hold ")) WaitForSingleObject(serverStop,1500);
        const bool stream=strstr(buffer," /stream ")!=NULL;
        const bool upload=strstr(buffer," /upload ")!=NULL;
        if(upload) WaitForSingleObject(serverStop,1500); // 不读上传正文，迫使客户端异步上传等待。
        const int size=stream?131072:2;char head[128];int n=sprintf(head,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",size);
        if(SendAll(sock,head,n))
        {
            if(stream) {char data[1024];memset(data,'x',sizeof(data));for(int i=0;i<size/1024;i++) {if(WaitForSingleObject(serverStop,10)==WAIT_OBJECT_0 || !SendAll(sock,data,sizeof(data)))break;}}
            else SendAll(sock,"OK",2);
        }
    }
    shutdown(sock,SD_BOTH);closesocket(sock);InterlockedDecrement(&clients);return 0;
}
static DWORD WINAPI Server(void*)
{
    while(WaitForSingleObject(serverStop,0)==WAIT_TIMEOUT)
    {
        fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval tv={0,100000};
        if(select(0,&set,NULL,NULL,&tv)<=0)continue;
        SOCKET sock=accept(listener,NULL,NULL);if(sock==INVALID_SOCKET)continue;
        InterlockedIncrement(&clients);HANDLE th=CreateThread(NULL,0,Client,(void*)sock,0,NULL);
        if(th)CloseHandle(th);else {closesocket(sock);InterlockedDecrement(&clients);Check(false,"server client thread");}
    }
    return 0;
}
static HANDLE StartServer()
{
    WSADATA data;Check(WSAStartup(MAKEWORD(2,2),&data)==0,"WSAStartup");
    serverStop=CreateEvent(NULL,TRUE,FALSE,NULL);listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    sockaddr_in address={0};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    Check(bind(listener,(sockaddr*)&address,sizeof(address))==0,"server bind");Check(listen(listener,SOMAXCONN)==0,"server listen");
    int len=sizeof(address);getsockname(listener,(sockaddr*)&address,&len);port=ntohs(address.sin_port);
    return CreateThread(NULL,0,Server,NULL,0,NULL);
}
struct RequestTask {Obj obj;char url[160];int bytes;int error;int upload;};
static DWORD WINAPI Request(void* raw)
{
    RequestTask* t=(RequestTask*)raw;const char* method=t->obj.type==Type("HTTP访问_Curl访问类")?"访问网页_cURL":t->obj.type==Type("HTTP访问_WinHTTP访问类")?"访问网页_WinHTTP":"访问网页_WinInet";
    int cmd=Cmd(t->obj.type,method);MDATA_INF args[32]={0};args[0]=Self(&t->obj);args[1].m_dtDataType=SDT_TEXT;args[1].m_pText=t->url;
    char* post=NULL;
    if(t->upload) {post=(char*)malloc(16*1024*1024+8);((int*)post)[0]=1;((int*)post)[1]=16*1024*1024;memset(post+8,'u',16*1024*1024);args[2].m_dtDataType=SDT_TEXT;args[2].m_pText=(LPSTR)"POST";args[4].m_dtDataType=SDT_BIN;args[4].m_pBin=(LPBYTE)post;}
    args[5]=Arg(5);
    MDATA_INF result=Call(cmd,args,info->m_pBeginCmdInfo[cmd].m_nArgCount+1);
    t->bytes=result.m_pBin?((int*)result.m_pBin)[1]:0;if(result.m_pBin)free(result.m_pBin);if(post)free(post);
    t->error=Op(&t->obj,"取上次请求错误码");return 0;
}
static RequestTask MakeRequest(int type,Obj* token,const char* path)
{
    RequestTask t={0};t.obj=New(type);t.bytes=-1;t.error=-1;sprintf(t.url,"http://127.0.0.1:%d/%s",port,path);if(token)Bind(&t.obj,token);return t;
}
static void WaitHit(LONG before,int n=1)
{
    DWORD begin=GetTickCount();while(InterlockedCompareExchange(&hits,0,0)<before+n && GetTickCount()-begin<4000)Sleep(2);
    Check(InterlockedCompareExchange(&hits,0,0)>=before+n,"server observed requests");
}
static const char* globalNames[]={"网页_访问_Curl","网页_访问_WinHTTP","网页_访问_WinInet"};
static int GlobalCmd(int backend)
{
    for(int i=0;i<info->m_nCmdCount;i++)if(info->m_pBeginCmdInfo[i].m_shtCategory>0 && !strcmp((char*)info->m_pBeginCmdInfo[i].m_szName,globalNames[backend]))return i;
    Check(false,"global command missing");ExitProcess(2);return -1;
}
struct GlobalTask {int backend;int mode;Obj* token;char url[160];int bytes;int upload;bool headEmpty;bool cancelText;};
static GlobalTask MakeGlobal(int backend,Obj* token,const char* path,int mode=1)
{
    GlobalTask t={0};t.backend=backend;t.token=token;t.mode=mode;t.bytes=-1;sprintf(t.url,"http://127.0.0.1:%d/%s",port?port:1,path);return t;
}
static DWORD WINAPI GlobalRequest(void* raw)
{
    GlobalTask* t=(GlobalTask*)raw;int cmd=GlobalCmd(t->backend),count=info->m_pBeginCmdInfo[cmd].m_nArgCount;MDATA_INF args[32]={0};
    args[0].m_dtDataType=SDT_TEXT;args[0].m_pText=t->url;args[6]=Arg(5);
    char* head=NULL;char* error=NULL;args[8].m_dtDataType=SDT_TEXT;args[8].m_ppText=&head;
    if(t->backend==0) {args[16].m_dtDataType=SDT_TEXT;args[16].m_ppText=&error;}
    void* tokenData=NULL;
    if(t->mode==1)args[count-1]=Reference(t->token,&tokenData);
    else if(t->mode==2)args[count-1].m_int=0x12345678; // 类型仍为空，必须忽略其联合体内容。
    char* post=NULL;
    if(t->upload) {post=(char*)malloc(16*1024*1024+8);((int*)post)[0]=1;((int*)post)[1]=16*1024*1024;memset(post+8,'u',16*1024*1024);args[1].m_dtDataType=SDT_TEXT;args[1].m_pText=(LPSTR)"POST";args[3].m_dtDataType=SDT_BIN;args[3].m_pBin=(LPBYTE)post;}
    MDATA_INF result=Call(cmd,args,t->mode==0?count-1:count);
    t->bytes=result.m_pBin?((int*)result.m_pBin)[1]:0;t->headEmpty=!head || !head[0];t->cancelText=error && strstr(error,"1223")!=NULL;
    if(result.m_pBin)free(result.m_pBin);if(post)free(post);if(head)free(head);if(error)free(error);return 0;
}
static void TestGlobalHttp()
{
    const int counts[]={20,17,15};int type=Type("取消令牌");
    for(int k=0;k<3;k++)
    {
        DWORD maxCancelMs=0;
        CMD_INFO& command=info->m_pBeginCmdInfo[GlobalCmd(k)];Check(command.m_nArgCount==counts[k],"new global parameter count");
        ARG_INFO& arg=command.m_pBeginArgInfo[counts[k]-1];
        Check(!strcmp((char*)arg.m_szName,"取消令牌") && arg.m_dtType==MAKELONG(type+1,0),"last parameter is cancellation token");
        Check((arg.m_dwState&AS_DEFAULT_VALUE_IS_EMPTY)!=0 && (arg.m_dwState&AS_RECEIVE_VAR)!=0,"token optional reference flags");
        for(int mode=0;mode<=2;mode++)
        {
            Obj token=New(type);GlobalTask t=MakeGlobal(k,&token,"fast",mode);GlobalRequest(&t);
            Check(t.bytes==2 && !t.headEmpty,"global omitted/null/running token completes");Drop(&token);
        }
        Obj token=New(type);Op(&token,"取消");LONG before=hits;GlobalTask t=MakeGlobal(k,&token,"fast");GlobalRequest(&t);
        Check(t.bytes==0 && t.headEmpty && hits==before,"global precancel prevents request");if(k==0)Check(t.cancelText,"global curl cancellation text");Drop(&token);
        token=New(type);Op(&token,"暂停");t=MakeGlobal(k,&token,"fast");before=hits;HANDLE thread=CreateThread(NULL,0,GlobalRequest,&t,0,NULL);
        Check(WaitForSingleObject(thread,100)==WAIT_TIMEOUT && hits==before,"global pause before send");Op(&token,"恢复");Join(thread);Check(t.bytes==2,"global resume");Drop(&token);
        for(int phase=0;phase<5;phase++)
        {
            token=New(type);Obj controller=Copy(&token);t=MakeGlobal(k,&token,phase==0?"hold":phase==2?"upload":"stream");t.upload=phase==2;before=hits;LONG tlsBefore=tlsHits;
            if(phase==4)sprintf(t.url,"https://127.0.0.1:%d/hold",port);
            thread=CreateThread(NULL,0,GlobalRequest,&t,0,NULL);
            if(phase==4) {DWORD begin=GetTickCount();while(tlsHits==tlsBefore && GetTickCount()-begin<3000)Sleep(2);Check(tlsHits>tlsBefore,"global TLS reached server");}
            else WaitHit(before);
            if(phase==0)Drop(&token); // 请求已固定状态引用，原变量销毁不影响另一副本控制请求。
            if(phase==1 || phase==3)Sleep(50);
            if(phase==3) {Op(&controller,"暂停");Check(WaitForSingleObject(thread,120)==WAIT_TIMEOUT,"global inflight pause");Op(&controller,"恢复");Join(thread);Check(t.bytes==131072,"global inflight resume body intact");}
            else {Op(&controller,"暂停");DWORD begin=GetTickCount();Op(&controller,"取消");Join(thread);DWORD elapsed=GetTickCount()-begin;if(elapsed>maxCancelMs)maxCancelMs=elapsed;Check(elapsed<1000,"global cancellation did not return promptly");Check(t.bytes==0 && t.headEmpty,"global cancel clears partial body and headers");if(k==0)Check(t.cancelText,"global curl active cancellation text");}
            Drop(&token);Drop(&controller);
        }
        printf("[OK] %s optional/null token, pause/resume, headers/body/upload/TLS cancellation, source disposal\n",globalNames[k]);
        printf("[CANCEL] %s max cancel-to-return=%lu ms\n",globalNames[k],maxCancelMs);
    }
    Obj token=New(type);GlobalTask tasks[9];HANDLE threads[9];LONG before=hits;
    for(int i=0;i<9;i++) {tasks[i]=MakeGlobal(i%3,&token,"hold");threads[i]=CreateThread(NULL,0,GlobalRequest,&tasks[i],0,NULL);}
    WaitHit(before);Sleep(50);Op(&token,"取消");for(int i=0;i<9;i++) {Join(threads[i]);Check(tasks[i].bytes==0 && tasks[i].headEmpty,"global shared broadcast");}Drop(&token);
    for(int i=0;i<90;i++)
    {
        token=New(type);GlobalTask t=MakeGlobal(i%3,&token,"fast");HANDLE thread=CreateThread(NULL,0,GlobalRequest,&t,0,NULL);if(i%2)Sleep(1);Op(&token,"取消");Join(thread);Check(t.bytes==0 || t.bytes==2,"global completion/cancel race");Drop(&token);
    }
    printf("[OK] 9 mixed global requests cancelled together; 90 global completion/cancel races\n");
}
static void TestHttp()
{
    const char* names[]={"HTTP访问_Curl访问类","HTTP访问_WinHTTP访问类","HTTP访问_WinInet访问类"};
    int types[]={Type(names[0]),Type(names[1]),Type(names[2])};int tokenType=Type("取消令牌");
    // 先热身所有传输和系统异步线程，句柄检查只比较稳定后的轮次。
    for(int k=0;k<3;k++) {RequestTask t=MakeRequest(types[k],NULL,"fast");Request(&t);Check(t.bytes==2 && t.error==0,"unbound normal request");Drop(&t.obj);}
    for(int k=0;k<3;k++)
    {
        Obj token=New(tokenType);Op(&token,"暂停");RequestTask t=MakeRequest(types[k],&token,"fast");LONG before=hits;
        HANDLE th=CreateThread(NULL,0,Request,&t,0,NULL);Check(WaitForSingleObject(th,100)==WAIT_TIMEOUT,"paused request blocks before send");Check(hits==before,"paused request not sent");
        Op(&token,"恢复");Join(th);Check(t.bytes==2 && t.error==0,"resume completes request");
        RequestTask cp=t;cp.obj=Copy(&t.obj);Op(&token,"取消");Drop(&token);Drop(&t.obj);Request(&cp);
        Check(cp.error==ERROR_CANCELLED && cp.bytes==0,"copied binding survives token and original request destruction");Drop(&cp.obj);
        token=New(tokenType);t=MakeRequest(types[k],&token,"fast");Op(&token,"取消");Check(Op(&t.obj,"解绑取消令牌")!=0,"unbind");Request(&t);Check(t.bytes==2 && t.error==0,"unbind normal request");Drop(&t.obj);Drop(&token);
        // 绑定持有轮次引用；切换源令牌后旧副本仍能控制已绑定请求。
        token=New(tokenType);Obj old=Copy(&token);t=MakeRequest(types[k],&token,"fast");Op(&token,"创建");Op(&old,"取消");Request(&t);Check(t.error==ERROR_CANCELLED,"binding preserves original generation");Check(Op(&token,"是否已取消")==0,"new generation unaffected");
        Bind(&t.obj,&token);Request(&t);Check(t.error==0 && t.bytes==2,"rebind to recreated token resumes normal access");Drop(&t.obj);Drop(&old);Drop(&token);
        for(int phase=0;phase<4;phase++)
        {
            token=New(tokenType);t=MakeRequest(types[k],&token,phase==0?"hold":phase==2?"upload":"stream");t.upload=phase==2;before=hits;
            th=CreateThread(NULL,0,Request,&t,0,NULL);WaitHit(before);if(phase==1 || phase==3)Sleep(50);
            if(phase==3) {Op(&token,"暂停");Check(WaitForSingleObject(th,120)==WAIT_TIMEOUT,"inflight pause blocks completion");Op(&token,"恢复");Join(th);Check(t.bytes==131072 && t.error==0,"inflight resume intact body");}
            else {Op(&token,"暂停");Op(&token,"取消");Join(th);Check(t.error==ERROR_CANCELLED && t.bytes==0,"cancel headers/body/upload while paused");}
            Drop(&t.obj);Drop(&token);
        }
        token=New(tokenType);t=MakeRequest(types[k],&token,"hold");sprintf(t.url,"https://127.0.0.1:%d/hold",port);
        LONG tlsBefore=tlsHits;th=CreateThread(NULL,0,Request,&t,0,NULL);DWORD begin=GetTickCount();while(tlsHits==tlsBefore && GetTickCount()-begin<3000)Sleep(2);
        Check(tlsHits>tlsBefore,"TLS handshake reached server");Op(&token,"取消");Join(th);Check(t.error==ERROR_CANCELLED && t.bytes==0,"cancel during TLS handshake");Drop(&t.obj);Drop(&token);
        printf("[OK] %s: pause/resume/cancel/copy/dispose/generation/unbind/headers/body/upload/TLS\n",names[k]);
    }
    Obj shared=New(tokenType);RequestTask many[12];HANDLE threads[12];LONG before=hits;
    for(int i=0;i<12;i++) {many[i]=MakeRequest(types[i%3],&shared,"hold");threads[i]=CreateThread(NULL,0,Request,&many[i],0,NULL);}
    // WinInet会限制同一服务器的并发连接，不能等所有请求到达服务端才取消；排队中的请求也必须被取消。
    WaitHit(before);Sleep(60);Op(&shared,"取消");
    for(int i=0;i<12;i++) {Join(threads[i]);Check(many[i].error==ERROR_CANCELLED && many[i].bytes==0,"one cancellation broadcasts to all backends");Drop(&many[i].obj);}
    Drop(&shared);
    for(int i=0;i<120;i++)
    {
        Obj token=New(tokenType);RequestTask t=MakeRequest(types[i%3],&token,"fast");HANDLE th=CreateThread(NULL,0,Request,&t,0,NULL);
        if(i%2)Sleep(1);Op(&token,"取消");Join(th);Check((t.error==0 && t.bytes==2)||(t.error==ERROR_CANCELLED && t.bytes==0),"completion/cancel race result");Drop(&t.obj);Drop(&token);
    }
    printf("[OK] broadcast to 12 requests, 120 completion/cancel races\n");
}
struct SlotTask {Obj* source;};
static DWORD WINAPI SlotReader(void* raw)
{
    Obj* source=((SlotTask*)raw)->source;
    for(int i=0;i<3000;i++)
    {
        Op(source,"是否已创建");Op(source,"是否已取消");Op(source,"是否已暂停");
        Obj copy=Copy(source);Check((Op(&copy,"是否已创建")!=0)==(copy.slot!=0),"copied snapshot remains valid");Drop(&copy);
    }
    return 0;
}
static void TestLifetime()
{
    DWORD before=0,after=0;GetProcessHandleCount(GetCurrentProcess(),&before);
    for(int i=0;i<2000;i++) {Obj t=New(Type("取消令牌"));Obj cp=Copy(&t);Op(&t,"暂停");Op(&cp,"取消");Drop(&t);Drop(&cp);}
    const char* names[]={"HTTP访问_Curl访问类","HTTP访问_WinHTTP访问类","HTTP访问_WinInet访问类"};
    for(int k=0;k<3;k++)for(int i=0;i<500;i++)
    {
        Obj t=New(Type("取消令牌"));Obj r=New(Type(names[k]));Bind(&r,&t);Bind(&r,&t);Obj cp=Copy(&r);Drop(&t);Drop(&r);Op(&cp,"解绑取消令牌");Drop(&cp);
    }
    Obj shared=New(Type("取消令牌"));SlotTask task={&shared};HANDLE readers[8];
    for(int i=0;i<8;i++)readers[i]=CreateThread(NULL,0,SlotReader,&task,0,NULL);
    for(int i=0;i<2000;i++) {Op(&shared,"销毁");Op(&shared,"创建");}
    for(int i=0;i<8;i++)Join(readers[i]);Drop(&shared);
    GetProcessHandleCount(GetCurrentProcess(),&after);Check(after<=before,"token event handle leak");
    printf("[OK] 2000 token lifetimes and 1500 request bindings/copies; handles %lu -> %lu\n",before,after);
    printf("[OK] 8 readers x 3000 queries/copies race 2000 slot destroy/create cycles\n");
}
struct MemorySample {SIZE_T busyBytes;SIZE_T busyBlocks;SIZE_T privateBytes;DWORD handles;};
static MemorySample SampleMemory()
{
    MemorySample sample={0};HANDLE heaps[128];DWORD count=GetProcessHeaps(128,heaps);Check(count>0 && count<=128,"enumerate heaps");
    if(count>128)count=128;
    for(DWORD i=0;i<count;i++)
    {
        if(!HeapLock(heaps[i])) {Check(false,"heap lock");continue;}
        PROCESS_HEAP_ENTRY entry={0};
        while(HeapWalk(heaps[i],&entry))if(entry.wFlags&PROCESS_HEAP_ENTRY_BUSY) {sample.busyBytes+=entry.cbData;sample.busyBlocks++;}
        DWORD error=GetLastError();HeapUnlock(heaps[i]);Check(error==ERROR_NO_MORE_ITEMS,"heap walk completed");
    }
    PROCESS_MEMORY_COUNTERS_EX memory={0};memory.cb=sizeof(memory);
    Check(GetProcessMemoryInfo(GetCurrentProcess(),(PROCESS_MEMORY_COUNTERS*)&memory,sizeof(memory))!=0,"private memory sample");
    sample.privateBytes=memory.PrivateUsage;GetProcessHandleCount(GetCurrentProcess(),&sample.handles);return sample;
}
static void MemoryBatch(int count)
{
    const int tokenType=Type("取消令牌");
    int types[]={Type("HTTP访问_Curl访问类"),Type("HTTP访问_WinHTTP访问类"),Type("HTTP访问_WinInet访问类")};
    for(int i=0;i<count;i++)
    {
        Obj token=New(tokenType);Obj copy=Copy(&token);Obj request=New(types[i%3]);Bind(&request,&token);Bind(&request,&copy);
        Obj requestCopy=Copy(&request);Op(&token,"暂停");Op(&copy,"恢复");Op(&token,"取消");
        Op(&token,"创建");Bind(&request,&token);
        if(i%1000==0)for(int k=0;k<3;k++) {GlobalTask call=MakeGlobal(k,&copy,"fast");GlobalRequest(&call);Check(call.bytes==0 && call.headEmpty,"global temporary object releases cancelled state");}
        Drop(&copy);Drop(&token);Drop(&request);Drop(&requestCopy);
    }
}
static void TestHeapPlateau()
{
    MemoryBatch(3000);SampleMemory(); // 排除首次堆遍历和分配器预热。
    MemorySample initial=SampleMemory(),first={0},last={0};
    for(int batch=0;batch<6;batch++)
    {
        MemoryBatch(10000);last=SampleMemory();if(batch==0)first=last;
        printf("[MEMORY] batch=%d busyBytes=%lu busyBlocks=%lu privateBytes=%lu handles=%lu\n",batch+1,(DWORD)last.busyBytes,(DWORD)last.busyBlocks,(DWORD)last.privateBytes,last.handles);
    }
    // 比较已分配的活块，而非可能保留在分配器中的空闲页；后者同时记录供排查。
    Check(last.busyBytes<=first.busyBytes+1024 && last.busyBlocks<=first.busyBlocks+4,"no growing live heap allocations");
    Check(last.handles<=initial.handles,"no growing token/request handles");
    printf("[OK] 60000 mixed token/copy/rebind/new-round/dispose lifetimes checked across process heaps\n");
}
static void TestPollingCost()
{
    Obj token=New(Type("取消令牌"));MDATA_INF args[]={Self(&token),Arg(0)};int cmd=Cmd(token.type,"等待继续");
    LARGE_INTEGER frequency,begin,end;QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&begin);
    for(int i=0;i<500000;i++)Check(Call(cmd,args,2).m_bool==BL_TRUE,"running polling result");
    QueryPerformanceCounter(&end);double ns=(end.QuadPart-begin.QuadPart)*1000000000.0/frequency.QuadPart/500000;
    printf("[PERF] standalone wait-continue including lifetime pin: %.1f ns/call (500000 calls)\n",ns);Drop(&token);
}
static void TestHttpCost()
{
    int types[]={Type("HTTP访问_Curl访问类"),Type("HTTP访问_WinHTTP访问类"),Type("HTTP访问_WinInet访问类")};
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    for(int k=0;k<3;k++)
    {
        Obj token=New(Type("取消令牌"));RequestTask integer=MakeRequest(types[k],NULL,"fast"),object=MakeRequest(types[k],&token,"fast");
        volatile LONG stop=0;MDATA_INF args[]={Self(&integer.obj),Arg(0)};args[1].m_pInt=(PINT)&stop;
        Check(Call(Cmd(types[k],"绑定停止变量"),args,2).m_bool==BL_TRUE,"bind benchmark integer");
        double ticks[2]={0,0};RequestTask* tests[]={&integer,&object};
        for(int i=0;i<120;i++)for(int pass=0;pass<2;pass++)
        {
            int which=(pass+i)%2;LARGE_INTEGER begin,end;QueryPerformanceCounter(&begin);Request(tests[which]);QueryPerformanceCounter(&end);
            Check(tests[which]->error==0 && tests[which]->bytes==2,"benchmark response intact");if(i>=20)ticks[which]+=(double)(end.QuadPart-begin.QuadPart);
        }
        printf("[PERF] backend=%d integer=%.3f ms/request token=%.3f ms/request (100 warmed samples each)\n",k,ticks[0]*1000/frequency.QuadPart/100,ticks[1]*1000/frequency.QuadPart/100);
        Drop(&integer.obj);Drop(&object.obj);Drop(&token);
    }
}
int main(int argc,char** argv)
{
    HMODULE module=LoadLibraryA(argc>1?argv[1]:"out\\cxsoft.fne");if(!module){printf("load error %lu\n",GetLastError());return 2;}
    typedef PLIB_INFO(WINAPI* GetInfo)();info=((GetInfo)GetProcAddress(module,"GetNewInf"))();info->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION,(DWORD)Notify,0);
    Check(info->m_nDataTypeCount==25 && info->m_nCmdCount==657,"metadata counts");
    LIB_DATA_TYPE_INFO& tokenType=info->m_pDataType[Type("取消令牌")];
    for(int i=0;i<tokenType.m_nCmdCount;i++)Check(strcmp((char*)info->m_pBeginCmdInfo[tokenType.m_pnCmdsIndex[i]].m_szName,"开始新一轮")!=0,"removed method must not be exposed");
    TestToken();TestLifetime();TestHeapPlateau();TestPollingCost();HANDLE server=StartServer();TestHttp();TestGlobalHttp();TestHttpCost();SetEvent(serverStop);Join(server);closesocket(listener);
    DWORD start=GetTickCount();while(clients && GetTickCount()-start<5000)Sleep(10);Check(clients==0,"server workers drained");CloseHandle(serverStop);WSACleanup();
    Check(runtimeErrors==0,"unexpected runtime errors");printf("[RESULT] failures=%ld runtimeErrors=%ld\n",failures,runtimeErrors);FreeLibrary(module);return failures?1:0;
}
