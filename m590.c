#define M590_STATE_READY 0
#define M590_STATE_UNKNOWN 2
#define M590_STATE_RING 3
#define M590_STATE_CONNECT 4
#define M590_STATE_SLEEP 5
#define M590_STATE_NO 100

void m590_responseSkipBreak(int fd, int count) {
    uint8_t x;
    size_t i = 0;
    while (read(fd, &x, 1) == 1) {
        if (x == 13) {
            i++;
        }
        if (i == count) {
            return;
        }
    }
}

void m590_responseReadAll(int fd) {
    uint8_t x;
    size_t n = 0;
    while (read(fd, &x, 1) == 1) {
        putchar(x);
        n++;
    }
}

int m590_responseIsOK(int fd) {
    uint8_t x;
    char state = 0;
    while (read(fd, &x, 1) == 1) {
        switch (state) {
            case 0:
                if (x == 'O') {
                    state = 1;
                }
                break;
            case 1:
                if (x == 'K') {
                    state = 2;
                } else {
                    state = 0;
                }
                break;
            case 2:
                if (x == 13) {
                    state = 3;
                } else {
                    state = 0;
                }
                break;
            case 3:
                if (x == 10) {
#ifdef MODE_DEBUG
                    puts("m590_responseIsOK: is OK\n");
#endif
                    return 1;
                } else {
                    state = 0;
                }
                break;
        }
    }
#ifdef MODE_DEBUG
    puts("m590_responseIsOK: is not OK\n");
#endif
    return 0;
}

int m590_getStatus(int fd) {
    serialPuts(fd, "AT+CPAS\r");
    /*
        char buf[LINE_SIZE];
        memset(buf, 0, LINE_SIZE);
        serialRead(fd, (void *) buf, 26);
        size_t i;
        char *chr = strstr(buf, "+CPAS:");
        if (chr == NULL) {
    #ifdef MODE_DEBUG
            fputs("m590_getStatus: +CPAS: not found\n", stderr);
    #endif
            serialFlush(fd);
            return M590_STATE_NO;
        }
        serialFlush(fd);
     */
    char buf[2];
    memset(buf, 0, sizeof buf);
    uint8_t x = 0;
    size_t i = 0;
    char *str = "+CPAS: ";
    size_t sl = strlen(str);
    int f = 0;
    while (read(fd, &x, 1) == 1) {
        if (i >= sl) {
            f = 1;
            break;
        }
        if (x == str[i]) {
            i++;
        } else {
            i = 0;
        }
    }
    if (!f) {
#ifdef MODE_DEBUG
        fputs("m590_getStatus: status not found in response\n", stderr);
#endif
        return M590_STATE_NO;
    }
    buf[0] = x;
    unsigned int out;
    if (sscanf(buf, "%u", &out) == 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "m590_getStatus: status: %d\n", out);
#endif
        return out;
    } else {
#ifdef MODE_DEBUG
        fputs("m590_getStatus: failed to parse status\n", stderr);
#endif
        return M590_STATE_NO;
    }
}

int m590_waitReady(int fd, struct timespec max_time) {
    Ton_ts tmr = {.ready = 0};
    while (1) {
        int status = m590_getStatus(fd);
        if (status == M590_STATE_READY) {
            return 1;
        }
        if (ton_ts(max_time, &tmr)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "m590_waitReady: can't wait: %d\n", status);
#endif
            return 0;
        }
        delayUsIdle(1000000);
    }
}

int m590_init(int fd) {
    struct timespec tm1 = {.tv_sec = 60, .tv_nsec = 0};
    if (!m590_waitReady(fd, tm1)) {
#ifdef MODE_DEBUG
        fputs("m590_init: m590_waitReady failed\n", stderr);
#endif
        return 0;
    }
    serialPuts(fd, "AT+CMGF=1\r");
    delayUsIdle(1000000);
    serialPuts(fd, "AT+CSCS=\"GSM\"\r");
    m590_responseReadAll(fd);
    return 1;
}

int m590_requireReady(int fd) {
    int status = m590_getStatus(fd);
    if (status == M590_STATE_UNKNOWN || status == M590_STATE_SLEEP || status == M590_STATE_NO) {
#ifdef MODE_DEBUG
        fprintf(stderr, "m590_requireReady: bad status: %d\n", status);
#endif
        return 0;
    }
    if (status == M590_STATE_RING || status == M590_STATE_CONNECT) {
        serialPuts(fd, "ATH\r");
        delayUsIdle(100);
        status = m590_getStatus(fd);
    }
    if (status != M590_STATE_READY) {
#ifdef MODE_DEBUG
        fprintf(stderr, "m590_requireReady: bad status: %d\n", status);
#endif
        return 0;
    }
    return 1;
}

int m590_sendSMS(int fd, char * phone_number, char * message) {
    struct timespec tm1 = {10, 0};
    if (!m590_waitReady(fd, tm1)) {
#ifdef MODE_DEBUG
        fputs("m590_sendSMS: m590_waitReady failed\n", stderr);
#endif
        return 0;
    }
    char buf[LINE_SIZE];
    char cmd[LINE_SIZE];
    memset(cmd, 0, LINE_SIZE);
    snprintf(cmd, sizeof cmd, "AT+CMGS=\"%s\"\r", phone_number);
    serialPuts(fd, cmd);
    memset(buf, 0, LINE_SIZE);
    m590_responseReadAll(fd);
    snprintf(cmd, sizeof cmd, "%s%c\r", message, 0x1a);
    serialPuts(fd, cmd);
    if (!m590_responseIsOK(fd)) {
#ifdef MODE_DEBUG
        fputs("m590_sendSMS: m590_responseIsOK failed\n", stderr);
#endif
        return 0;
    }
    return 1;
}

int m590_ring(int fd, char * phone_number) {
    char cmd[LINE_SIZE];
    memset(cmd, 0, LINE_SIZE);
    snprintf(cmd, sizeof cmd, "ATD%s;\r", phone_number);
#ifdef MODE_DEBUG
    printf("m590_ring: cmd: %s\n", cmd);
#endif
    serialPuts(fd, cmd);
    char buf[LINE_SIZE];
    memset(buf, 0, LINE_SIZE);
    if (!m590_responseIsOK(fd)) {
#ifdef MODE_DEBUG
        fputs("m590_ring: m590_responseIsOK\n", stderr);
#endif
        return 0;
    }
    return 1;
}

int m590_makeCall(int fd, char * phone_number) {
    struct timespec tm1 = {10, 0};
    if (!m590_waitReady(fd, tm1)) {
#ifdef MODE_DEBUG
        fputs("m590_makeCall: m590_waitReady failed\n", stderr);
#endif
        return 0;
    }
    if (!m590_ring(fd, phone_number)) {
#ifdef MODE_DEBUG
        fputs("m590_makeCall: m590_ring failed\n", stderr);
#endif
        return 0;
    }
    Ton_ts tmr = {.ready = 0};
    struct timespec max_time = {30, 0};
    while (1) {
        int status = m590_getStatus(fd);
        if (status == M590_STATE_READY) {
            m590_responseReadAll(fd);
#ifdef MODE_DEBUG
            puts("m590_makeCall: success");
#endif
            return 1;
        }
        if (ton_ts(max_time, &tmr)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "m590_waitReady: can't wait: %d\n", status);
#endif
            serialPuts(fd, "ATH\r");
            m590_responseReadAll(fd);
            return 0;
        }
        delayUsIdle(1000000);
    }
    serialPuts(fd, "ATH\r");
    return 1;
}

void m590_skipRing(int fd) {
    char buf[LINE_SIZE];
    memset(buf, 0, LINE_SIZE);
    serialRead(fd, (void *) buf, 10);
    char *chr = strstr(buf, "RING");
    if (chr != NULL) {
#ifdef MODE_DEBUG
        fputs("m590_skipRing: sending ATH\n", stderr);
#endif
        serialPuts(fd, "ATH\r");
    }
    serialFlush(fd);
}