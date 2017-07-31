/*
 * gwu59
 */

#include "main.h"

char pid_path[LINE_SIZE];
char app_class[NAME_SIZE];
char serial_path[LINE_SIZE];

int app_state = APP_INIT;

int pid_file = -1;
int proc_id;
int sock_port = -1;
size_t sock_buf_size = 0;
size_t b_count = 0; //number of items in buffers
int sock_fd = -1;
int serial_fd = -1;
int serial_baud_rate = -1;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
pthread_t thread;
char thread_cmd;
S2List s2l = {NULL, 0};
S1List s1l = {NULL, 0};


FIFOItemList_SMS sms_list = {.item = NULL, .length = 0, .pop_item = NULL, .push_item = NULL};
FIFOItemList_PN pn_list = {.item = NULL, .length = 0, .pop_item = NULL, .push_item = NULL};

#include "util.c"
#include "m590.c"

int readSettings() {
    FILE* stream = fopen(CONFIG_FILE, "r");
    if (stream == NULL) {
#ifdef MODE_DEBUG
        fputs("ERROR: readSettings: fopen\n", stderr);
#endif
        return 0;
    }
    skipLine(stream);
    int n;
    n = fscanf(stream, "%d\t%255s\t%d\t%ld\t%ld\t%d\t%255s\t%d\n",
            &sock_port,
            pid_path,
            &sock_buf_size,
            &cycle_duration.tv_sec,
            &cycle_duration.tv_nsec,
            &b_count,
            serial_path,
            &serial_baud_rate
            );
    if (n != 8) {
        fclose(stream);
        #ifdef MODE_DEBUG
        fputs("ERROR: readSettings: bad row format\n", stderr);
#endif
        return 0;
    }
    fclose(stream);
    #ifdef MODE_DEBUG
    printf("readSettings: \n\tsock_port: %d, \n\tpid_path: %s, \n\tsock_buf_size: %d, \n\tcycle_duration: %ld sec %ld nsec, \n\tqueue_size: %d, \n\tserial_path: %s, \n\tserial_baud_rate: %d\n", sock_port, pid_path, sock_buf_size,cycle_duration.tv_sec,cycle_duration.tv_nsec, b_count, serial_path, serial_baud_rate);
#endif
    return 1;
}

int initSMS(FIFOItemList_SMS *list, size_t count) {
    list->length = count;
    list->item = (FIFOItem_SMS *) malloc(list->length * sizeof *(list->item));
    if (list->item == NULL) {
        list->length = 0;
#ifdef MODE_DEBUG
        fputs("ERROR: initSMS: memory allocation for sms_list failed\n", stderr);
#endif
        return 0;
    }
    size_t i;
    FORL{
        memset(LIi.data.phone_number, 0, sizeof LIi.data.phone_number);
        memset(LIi.data.message, 0, sizeof LIi.data.message);
        if (i == Lil) {
            LIi.next = &list->item[0];
        } else {
            LIi.next = &list->item[i + 1];
        }
        if (i == 0) {
            LIi.prev = &list->item[Lil];
        } else {
            LIi.prev = &list->item[i - 1];
        }
        LIi.free = 1;
    }
    list->pop_item = NULL;
    list->push_item = &list->item[0];
    initMutex(&list->mutex);
    return 1;
}

int initPN(FIFOItemList_PN *list, size_t count) {
    list->length = count;
    list->item = (FIFOItem_PN *) malloc(list->length * sizeof *(list->item));
    if (list->item == NULL) {
        list->length = 0;
#ifdef MODE_DEBUG
        fputs("ERROR: initPN: memory allocation for pn_list failed\n", stderr);
#endif
        return 0;
    }
    size_t i;
    FORL{
        memset(LIi.data.phone_number, 0, sizeof LIi.data.phone_number);
        if (i == Lil) {
            LIi.next = &list->item[0];
        } else {
            LIi.next = &list->item[i + 1];
        }
        if (i == 0) {
            LIi.prev = &list->item[Lil];
        } else {
            LIi.prev = &list->item[i - 1];
        }
        LIi.free = 1;
    }
    list->pop_item = NULL;
    list->push_item = &list->item[0];
    initMutex(&list->mutex);
    return 1;
}

int initData() {
    s1l.item = (char *) malloc(sock_buf_size * sizeof *(s1l.item) * LINE_SIZE);
    if (s1l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("ERROR: initData: memory allocation for s1l failed\n", stderr);
#endif
        return 0;
    }
    s2l.item = (S2 *) malloc(sock_buf_size * sizeof *(s2l.item));
    if (s2l.item == NULL) {
#ifdef MODE_DEBUG
        fputs("ERROR: initData: memory allocation for s2l failed\n", stderr);
#endif
        FREE_LIST(&s1l);
        return 0;
    }
    if (!initSMS(&sms_list, b_count)) {
        FREE_FIFO(&sms_list);
        FREE_LIST(&s1l);
        FREE_LIST(&s2l);
        return 0;
    }
    if (!initPN(&pn_list, b_count)) {
        FREE_FIFO(&pn_list);
        FREE_FIFO(&sms_list);
        FREE_LIST(&s1l);
        FREE_LIST(&s2l);
        return 0;
    }
    if (!m590_init(serial_fd)) {
        FREE_FIFO(&pn_list);
        FREE_FIFO(&sms_list);
        FREE_LIST(&s1l);
        FREE_LIST(&s2l);
        return 0;
    }
    if (!createThread_ctl()) {
        FREE_FIFO(&pn_list);
        FREE_FIFO(&sms_list);
        FREE_LIST(&s1l);
        FREE_LIST(&s2l);
        return 0;
    }
    return 1;
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    peer_client.sock_buf_size = sock_buf_size;
#ifdef MODE_DEBUG
    puts("initApp: readSettings: done");
#endif
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
#ifdef MODE_DEBUG
    puts("initApp: initPid: done");
#endif
    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }
#ifdef MODE_DEBUG
    puts("initApp: initServer: done");
#endif
    if (!initSerial(&serial_fd, serial_path, serial_baud_rate)) {
        exit_nicely_e("initApp: failed to initialize serial\n");
    }
#ifdef MODE_DEBUG
    puts("initApp: initSerial: done");
#endif

}

void serverRun(int *state, int init_state) {
    char buf_in[sock_buf_size];
    char buf_out[sock_buf_size];
    memset(buf_in, 0, sizeof buf_in);
    acp_initBuf(buf_out, sizeof buf_out);
    if (recvfrom(sock_fd, buf_in, sizeof buf_in, 0, (struct sockaddr*) (&(peer_client.addr)), &(peer_client.addr_size)) < 0) {
#ifdef MODE_DEBUG
        perror("serverRun: recvfrom() error");
#endif
    }
#ifdef MODE_DEBUG
    acp_dumpBuf(buf_in, sizeof buf_in);
#endif    
    if (!acp_crc_check(buf_in, sizeof buf_in)) {
#ifdef MODE_DEBUG
        fputs("ERROR: serverRun: crc check failed\n", stderr);
#endif
        return;
    }
     int i;
    switch (buf_in[1]) {
        case ACP_CMD_APP_START:
            if (!init_state) {
                *state = APP_INIT_DATA;
            }
            return;
        case ACP_CMD_APP_STOP:
            if (init_state) {
                *state = APP_STOP;
            }
            return;
        case ACP_CMD_APP_RESET:
            *state = APP_RESET;
            return;
        case ACP_CMD_APP_EXIT:
            *state = APP_EXIT;
            return;
        case ACP_CMD_APP_PING:
            if (init_state) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_BUSY);
            } else {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_IDLE);
            }
            return;
        case ACP_CMD_APP_PRINT:
            printAll(&sms_list);
            return;
        case ACP_CMD_APP_HELP:
            printHelp();
            return;
        case ACP_CMD_GWU59_SEND_SMS:
            if (!init_state) {
                return;
            }
            acp_parsePackS2(buf_in, &s2l, sock_buf_size);
            if (s2l.length <= 0) {
                return;
            }
            for (i = 0; i < s2l.length; i++) {
                SMS item;
                memset(&item, 0, sizeof item);
                strcpy(item.phone_number, s2l.item[i].p0);
                strcpy(item.message, s2l.item[i].p1);
                SMS_fifo_push(item, &sms_list);
            }
            return;
        case ACP_CMD_GWU59_RING:
            if (!init_state) {
                return;
            }
            acp_parsePackS1(buf_in, &s1l, sock_buf_size);
            if (s1l.length <= 0) {
                return;
            }
            for (i = 0; i < s1l.length; i++) {
                PN item;
                memset(&item, 0, sizeof item);
                memcpy(item.phone_number, &s1l.item[LINE_SIZE * i], sizeof item.phone_number);
                PN_fifo_push(item, &pn_list);
            }

            if (!sendBufPack(buf_out, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED)) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                return;
            }
            return;
        default:
            if (!init_state) {
                return;
            }
            break;
    }

}

void *threadFunction(void *arg) {
    char *cmd = (char *) arg;
#ifdef MODE_DEBUG
    puts("threadFunction: running...");
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();
        size_t i;
        for (i = 0; i < sms_list.length; i++) {
            SMS item;
            if (SMS_fifo_pop(&item, &sms_list)) {
                m590_sendSMS(serial_fd, item.phone_number, item.message);
            }
        }
        for (i = 0; i < sms_list.length; i++) {
            PN item;
            if (PN_fifo_pop(&item, &pn_list)) {
                m590_makeCall(serial_fd, item.phone_number);
            }
        }
        switch (*cmd) {
            case ACP_CMD_APP_STOP:
            case ACP_CMD_APP_RESET:
            case ACP_CMD_APP_EXIT:
                *cmd = ACP_CMD_APP_NO;
                return (EXIT_SUCCESS);
        }
        sleepRest(cycle_duration, t1);
    }
}

int createThread_ctl() {
    if (pthread_create(&thread, NULL, &threadFunction, (void *) &thread_cmd) != 0) {
        perror("createThreads: pthread_create");
        return 0;
    }
    return 1;
}

void freeData() {
    waitThread_ctl(ACP_CMD_APP_EXIT);
#ifdef MODE_DEBUG
    puts("freeData: freeThread done");
#endif    
    FREE_FIFO(&pn_list);
    FREE_FIFO(&sms_list);
    FREE_LIST(&s2l);
    FREE_LIST(&s1l);
#ifdef MODE_DEBUG
    puts("freeData: lists a free");
#endif  
}

void freeApp() {
    freeData();
#ifdef MODE_DEBUG
    puts("freeData: done");
#endif
    freeSocketFd(&sock_fd);
#ifdef MODE_DEBUG
    puts("free sock_fd: done");
#endif
    freePid(&pid_file, &proc_id, pid_path);
#ifdef MODE_DEBUG
    puts("freePid: done");
#endif
    serialClose(serial_fd);
#ifdef MODE_DEBUG
    puts("serialClose: done");
#endif
#ifdef MODE_DEBUG
    puts("freeApp: done");
#endif
}

void exit_nicely() {
    freeApp();
#ifdef MODE_DEBUG
    puts("\nBye...");
#endif
    exit(EXIT_SUCCESS);
}

void exit_nicely_e(char *s) {
    fprintf(stderr, "%s", s);
    freeApp();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
        if (geteuid() != 0) {
#ifdef MODE_DEBUG
        fprintf(stderr,"%s: root user expected\n", APP_NAME_STR);
#endif
        return (EXIT_FAILURE);
    }
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
        switch (app_state) {
            case APP_INIT:
#ifdef MODE_DEBUG
                puts("MAIN: init");
#endif
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
#ifdef MODE_DEBUG
                puts("MAIN: init data");
#endif
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
#ifdef MODE_DEBUG
                puts("MAIN: run");
#endif
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
#ifdef MODE_DEBUG
                puts("MAIN: stop");
#endif
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
#ifdef MODE_DEBUG
                puts("MAIN: reset");
#endif
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
#ifdef MODE_DEBUG
                puts("MAIN: exit");
#endif
                exit_nicely();
                break;
            default:
                exit_nicely_e("main: unknown application state");
                break;
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}