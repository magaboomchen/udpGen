#define _GNU_SOURCE // sendmmsg

#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <linux/ip.h>
#include <string.h>
#include <asm/byteorder.h>
#include <time.h>
#include <pthread.h>
#include "common.h"
////#define MEASURE
////#define ENABLE_CONTROL
////#define DISPLAY_DELAY
#define POSSION_DELAY
#define DISPLAY_VELOCITY
////#define DELAY_TEST

#define PACKETBUFF 1 // Don not change this value!
#define PACKSIZE 1500
#define PART1 200
#define PART2 500
#define PART3 1000
#define kp1 0.1
#define ki1 0.0
#define kd1 0.0
#define kp2 0.5
#define ki2 0.0
#define kd2 0.0
#define kp3 1.0
#define ki3 0.0
#define kd3 0.0
#define kp4 1.5
#define ki4 0.0
#define kd4 0.0
#define interval 1000UL //1000ms
//time to send 1024 pakcets per round ! This value must be accuracy! You could tune this value to access best velocity control performance!
//#define tc 16600000 // value in vmware
#define tc 18700 // value in host of laptop
long nd=0; //nanodelay
unsigned long aim_pps=30000;
unsigned long payload_sz = 4;
signed long workingtime=-1;
double lamda=1000;

#ifdef ENABLE_CONTROL
struct timespec td={
    .tv_sec=0,    //time_t tv_sec; // seconds
    .tv_nsec=0    //long tv_nsec; // and nanoseconds
};
#endif

struct state {
    struct net_addr *target_addr;
    volatile uint64_t bps;
    volatile uint64_t pps;
    int packets_in_buf;
    char (*payload)[PACKSIZE+1];
    int payload_sz;
    int src_port;
    char thread_seq;
};

struct ipoptions{
    char type;
    char length;
    char pointer;
    char thread_num;
    uint32_t temp;
};//4-bytes-align problems!!!

int ndelay(const struct timespec delay){
    struct timespec tc1;
    struct timespec tc2;
    clock_gettime(CLOCK_REALTIME, &tc1);
    long delays=0,delayns=0;
    while(1){
        //get the time of now
        clock_gettime(CLOCK_REALTIME, &tc2);
        //calculate the real delay time
        if(tc2.tv_sec>tc1.tv_sec){
            delayns=tc2.tv_nsec-tc1.tv_nsec+1000000000*(tc2.tv_sec-tc1.tv_sec);
            delays=delayns/1000000000;
            delayns=delayns-delays*1000000000;
        }else if(tc2.tv_sec==tc1.tv_sec){
            delayns=tc2.tv_nsec-tc1.tv_nsec;
            delays=0;
        }else{return 1;}
        //judge the delay
        if(delays==delay.tv_sec){
            if(delayns>=delay.tv_nsec){
              break;
          }
      }else if(delays>delay.tv_sec){break;}
  }
  return 0;
}

int printdelay(const struct timespec t1,const struct timespec t2){
    long delays=0,delayns=0;
    if(t2.tv_sec==t1.tv_sec){
        printf("delay:0s+%luns\n",t2.tv_nsec-t1.tv_nsec);
    }else if(t2.tv_sec>t1.tv_sec){
        delayns=t2.tv_nsec-t1.tv_nsec+1000000000*(t2.tv_sec-t1.tv_sec);
        delays=delayns/1000000000;
        if(delays>0){delayns=delayns-delays*1000000000;}
        printf("delay:%lus+%luns\n",delays,delayns);
   }else{printf("2rd parameter must larger than 1st.\n");return 1;}
   return 0;
}

void thread_loop(void *userdata) {
    struct state *state = userdata;
    struct mmsghdr *messages = calloc(state->packets_in_buf, sizeof(struct mmsghdr));
    struct iovec *iovecs = calloc(state->packets_in_buf, sizeof(struct iovec));
    int fd = net_connect_udp(state->target_addr, state->src_port);
    int i;
    for(i = 0; i < state->packets_in_buf; i++){
        struct iovec *iovec = &iovecs[i];
        struct mmsghdr *msg = &messages[i];
        msg->msg_hdr.msg_iov = iovec;
        msg->msg_hdr.msg_iovlen = 1;
        iovec->iov_base = (void*)state->payload[i];
        iovec->iov_len = state->payload_sz;
    }
    
    struct ipoptions count;
    memset(&count,0,sizeof(count));
    //printf("sizeof(count):%lu\r\n",sizeof(count));
    count.type=1 |IPOPT_MEASUREMENT;count.length=0x08;count.pointer=0x05;count.thread_num=state->thread_seq;
    //printf("threas_seq:%c\r\n",(state->thread_seq)+'0');
    count.temp=htonl(0x00000000);

#ifndef ENABLE_CONTROL
#ifdef POSSION_DELAY
    struct timespec td={
                .tv_sec=0,    //time_t tv_sec; // seconds
                .tv_nsec=0    //long tv_nsec; // and nanoseconds
            };
    double z;
    double lamda_1;
    double pdelay;
    if(lamda>0){lamda_1=1/lamda;}else{printf("lamada must bigger than 0\n");lamda_1=1;}
    srand((unsigned)time(NULL));
#endif
#endif

#ifdef MEASURE
    struct timespec tc1;
    struct timespec tc2;
#endif

    while(1){
#ifdef ENABLE_CONTROL
        struct timespec tdt;
        if(-1==nanosleep(&td,&tdt)){perror("Didn't delay well! nanosleep()");}
#endif
#ifndef ENABLE_CONTROL
#ifdef POSSION_DELAY
        // possion delay
        do{
            z = ((double)rand()/RAND_MAX);
        }while((z == 0) || (z == 1));
        pdelay=-lamda_1*log(z);
        td.tv_sec=((long)pdelay);
        td.tv_nsec=(pdelay-td.tv_sec)*1000000000;
        if(td.tv_nsec>=4500){td.tv_nsec-=4500;}
        else if(td.tv_nsec>=0 && td.tv_nsec<4500){td.tv_nsec=0;}
        //printf("lamda_1:%.9f\tpdelay:%.9f\ttd.tv_sec:%lus td.tv_nsec:%luns\n",lamda_1,pdelay,td.tv_sec,td.tv_nsec);
        ndelay(td);
#endif
#endif
#ifdef MEASURE
        clock_gettime(CLOCK_REALTIME, &tc1);
#endif
#ifdef DELAY_TEST
        struct timespec tc1;
        struct timespec tc2;
        struct timespec delayn={
            .tv_sec=0,    //time_t tv_sec; // seconds
            .tv_nsec=0    //long tv_nsec; // and nanoseconds
        };
        clock_gettime(CLOCK_REALTIME, &tc1);
        ndelay(delayn);
        clock_gettime(CLOCK_REALTIME, &tc2);
        printdelay(tc1,tc2);
#endif
        //send the packets
        if(-1==setsockopt(fd,IPPROTO_IP,IP_OPTIONS, (char*)&count, sizeof(count))){perror("Plead use root authority to run! setsockopt()"); }
        count.temp=htonl(ntohl(count.temp)+1);
        int r = sendmmsg(fd, messages, state->packets_in_buf, 0);
        if (r <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }

            if (errno == ECONNREFUSED) {
                continue;
            }
            PFATAL("sendmmsg()");
        }
        int i, bytes = 0;
        for (i = 0; i < r; i++) {
           struct mmsghdr *msg = &messages[i];
           int len = msg->msg_len;
           msg->msg_hdr.msg_flags = 0;
           msg->msg_len = 0;
           bytes += len;
        }
        __atomic_fetch_add(&state->pps, r, 0);
        __atomic_fetch_add(&state->bps, bytes, 0);
#ifdef MEASURE
        clock_gettime(CLOCK_REALTIME, &tc2);
        printdelay(tc1,tc2); 
#endif
    }
}

int addcheck(const char * c,const int n){
    int t=0;
    int dot=0,colon=0;
    for(t=0;t<n;t++){
        if(c[t]=='.'){dot++;}
        if(c[t]==':'){colon++;}
    }
    if(dot==3&&colon==1){return 1;}
    else{return 0;}
}

int paracheck(const char *c,const int n,const char t[]){
    int i=0,count=0;
    if(0==strcmp("int",t)){
        for(i=0;i<n;i++){
            if(c[i]<'0' || c[i]>'9'){
                return 0;
            }
        }
    }else if(0==strcmp("double",t)){
        for(i=0;i<n;i++){
            if(!((c[i]>='0' && c[i]<='9') || c[i]=='.' )){
                return 0;
            }else if(c[i]=='.'){
                count++;
                if(count>1){return 0;}
            }
        }
    }else{return 1;}
    return 1;
}

int main(int argc, const char *argv[])
{
    if (argc == 1){
        FATAL("Usage: %s [target ip:port] [target ...]", argv[0]);
    }

    int t; int thread_num = 0;
    for(t=1;t<argc;t++){
        if(addcheck(argv[t],strlen(argv[t]))){
           thread_num++;
        }else if(0==strcmp(argv[t],"-pps")){
            t++;if(t>=argc){FATAL("Lost parameter after -pps!");}
            if(paracheck(argv[t],strlen(argv[t]),"int")){aim_pps=atoi(argv[t]);}
            else{FATAL("The type of the parameter after -pps must be unsigned int!");}
        }else if(0==strcmp(argv[t],"-psize")){
            t++;if(t>=argc){FATAL("Lost parameter after -psize!");}
            if(paracheck(argv[t],strlen(argv[t]),"int")){payload_sz=atoi(argv[t]);}
            else{FATAL("The type of the parameter after -psize must be unsigned int!");}
        }else if(0==strcmp(argv[t],"-t")){
            t++;if(t>=argc){FATAL("Lost parameter after -t!");}
            if(paracheck(argv[t],strlen(argv[t]),"int")){workingtime=atoi(argv[t]);}
            else{FATAL("The type of the parameter after -t must be unsigned int!");}
        }else if(0==strcmp(argv[t],"-lamda")){
            t++;if(t>=argc){FATAL("Lost parameter after -lamda!");}
            if(paracheck(argv[t],strlen(argv[t]),"double")){lamda=atof(argv[t]);}
            else{FATAL("The type of the parameter after -lamda must be double!");}
        }else if(0==strcmp(argv[t],"-h") || 0==strcmp(argv[t],"--help")){
            printf("-pps :packets per second, example: -pps 30000\n-psize : packets size, example: -psize 32\n-t : working time, example: -t 10\n");
            return 0;
       }else{printf("%s\r\n",argv[t]);FATAL("input ./udpsender -h or ./udpsender --help for more information.");}
    }
    //display basic sender information
    printf("lamda: %f\r\n",lamda);
    printf("packetsize: %lu bytes\r\n",payload_sz);
    if(workingtime>0){printf("workingtime: %ld \r\n",workingtime);}
    else{printf("workingtime: forever \r\n");}
    printf("thread_num:%d\r\n",thread_num);
    int packets_in_buf = PACKETBUFF;
    char payload[PACKETBUFF][PACKSIZE+1];
#ifdef ENABLE_CONTROL
    //initialize the nd.
    if(1000000000.0/(aim_pps/PACKETBUFF)>=tc){nd=(1000000000.0/(aim_pps/PACKETBUFF)-tc)*thread_num;}
    else{nd=0;printf("Exceed the MAX velocity!\r\n");}
    printf("nd:%lu\r\n",nd);
    td.tv_nsec=nd;
#endif
    //initialize the payload zone
    memset(payload,0,sizeof(payload));
    if(payload_sz>=4){
        for(t=0;t<packets_in_buf;t++){
            *(unsigned int*)payload[t]=htonl(t);//set the number.
        }
    }

    struct net_addr *target_addrs = calloc(argc-1, sizeof(struct net_addr));

    for (t = 0; t < thread_num; t++) {
        const char *target_addr_str = argv[t+1];
        parse_addr(&target_addrs[t], target_addr_str);

        fprintf(stderr, "[*] Sending to %s, send buffer %i packets\n",
        addr_to_str(&target_addrs[t]), packets_in_buf);
    }

    struct state *array_of_states = calloc(thread_num, sizeof(struct state));

    for (t = 0; t < thread_num; t++) {
        struct state *state = &array_of_states[t];
        state->target_addr = &target_addrs[t];
        state->packets_in_buf = packets_in_buf;
        state->payload = payload;
        state->payload_sz = payload_sz;
        state->src_port = 11404;
        state->pps=0;
        state->bps=0;
        state->thread_seq=t;
        thread_spawn(thread_loop, state);
    }

    // velocity display
    uint64_t last_pps = 0;
    uint64_t last_bps = 0;
    uint64_t sum=0;
    double now_delta_pps=0;
    double now_delta_bps=0;
    // velocity control
#ifdef ENABLE_CONTROL
    double last_delta_pps=0;
    double last_delta_bps=0;
    double now_errorpps=-1;
    double last_errorpps=-1;
    double delta_d=-1;
    int symbol;
#endif
while (workingtime!=0) {
    if(workingtime>0){workingtime--;}
    struct timeval timeout = NSEC_TIMEVAL(MSEC_NSEC(interval));
    while (1) {
        int r = select(0, NULL, NULL, NULL, &timeout);
        if (r != 0) {continue;}
        if (TIMEVAL_NSEC(&timeout) == 0) {break;}
    }
    // pass
    uint64_t now_pps = 0, now_bps = 0;
    for (t = 0; t < thread_num; t++) {
        struct state *state = &array_of_states[t];
        now_pps += __atomic_load_n(&state->pps, 0);
        now_bps += __atomic_load_n(&state->bps, 0);
    }
    //calculate the now_delta_pps and now_delta_bps
    now_delta_pps = now_pps - last_pps;
    now_delta_bps = now_bps - last_bps;
    last_pps = now_pps;
    last_bps = now_bps;
#ifdef ENABLE_CONTROL
    //calculate the now_error and last_error
    now_errorpps=now_delta_pps-(aim_pps/(1000.0/interval));
    last_errorpps=last_delta_pps-(aim_pps/(1000.0/interval));
    last_delta_pps=now_delta_pps;//update the last_delta_pps
    delta_d=0;
    symbol=now_errorpps>0?1:-1;
    if(abs(now_errorpps)<=PART1){
        delta_d=kp1*now_errorpps+ki1*(now_errorpps+last_errorpps)+kd1*(now_errorpps-last_errorpps);
    }else if(abs(now_errorpps)<=PART2){
        delta_d=kp2*now_errorpps+ki2*(now_errorpps+last_errorpps)+kd2*(now_errorpps-last_errorpps)+ symbol*PART1*(kp2-kp1);
    }else if(abs(now_errorpps)<=PART3){
        delta_d=kp3*now_errorpps+ki3*(now_errorpps+last_errorpps)+kd3*(now_errorpps-last_errorpps)+symbol*(PART2*(kp3-kp2)+PART1*(kp2-kp1));
    }else{
        delta_d=kp4*now_errorpps+ki4*(now_errorpps+last_errorpps)+kd4*(now_errorpps-last_errorpps)+symbol*(PART3*(kp4-kp3)+PART2*(kp3-kp2)+PART1*(kp2-kp1));
    }
    //calculate the control var
    if(nd+(long)delta_d>=0 && nd+(long)delta_d<=999999999){nd+=(long)delta_d;td.tv_nsec=nd;}
    else if(nd+(long)delta_d<0){nd=0;td.tv_nsec=nd;}
    else if(nd+(long)delta_d>999999999){nd=999999999;td.tv_nsec=nd;}
#ifdef DISPLAY_DELAY
    printf("delta_d:%7.3f, ",delta_d);
    printf("nanodelay:%ld, ",nd);
    printf("now_errorpps:%7.3f pps.\r\n",now_errorpps);
#endif
#endif
#ifdef DISPLAY_VELOCITY
    sum+=now_delta_pps;
    if(workingtime%1==0){
        printf("%7.7fK pps %7.3fMiB / %7.3fMb\n",
        now_delta_pps / 1000.0    * (1000.0/interval),
        now_delta_bps / 1024.0 / 1024.0 * (1000.0/interval),
        now_delta_bps * 8.0 / 1000.0 / 1000.0 *(1000.0/interval) );
    }
#endif
    }
    printf("Send %lu Packets.\a\n",sum);
    return 0;
}
