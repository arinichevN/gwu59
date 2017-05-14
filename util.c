/*
 * gwu59
 */

#include "main.h"

FUN_FIFO_PUSH(SMS)

FUN_FIFO_POP(SMS)

FUN_FIFO_PUSH(PN)

FUN_FIFO_POP(PN)


int sendStrPack(char qnf, char *cmd) {
    extern Peer peer_client;
    return acp_sendStrPack(qnf, cmd,  &peer_client);
}

int sendBufPack(char *buf, char qnf, char *cmd_str) {
    extern Peer peer_client;
    return acp_sendBufPack(buf, qnf, cmd_str,  &peer_client);
}

void waitThread_ctl(char cmd) {
    thread_cmd = cmd;
    pthread_join(thread, NULL);
}

void sendStr(const char *s, uint8_t *crc) {
    acp_sendStr(s, crc, &peer_client);
}

void sendFooter(int8_t crc) {
    acp_sendFooter(crc, &peer_client);
}

void printAll() {
    extern FIFOItemList_SMS sms_list;
    extern FIFOItemList_PN pn_list;
    char q[LINE_SIZE];
    uint8_t crc = 0;
    size_t i;
      snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "sock_buf_size: %d\n", sock_buf_size);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "b_count: %d\n", b_count);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "serial_path: %s\n", serial_path);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "serial_baud_rate: %d\n", serial_baud_rate);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    sendStr(q, &crc);
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "SMS buffer size: %d\n push item pointer: %p\n pop item pointer: %p\n", sms_list.length, sms_list.push_item, sms_list.pop_item);
    sendStr(q, &crc);
    sendStr("+---------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                              SMS list                                       |\n", &crc);
    sendStr("+------------+--------------------------------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|phone_number|            message             |  is_free  |    ptr    | next_ptr  | prev_ptr  |\n", &crc);
    sendStr("+------------+--------------------------------+-----------+-----------+-----------+-----------+\n", &crc);
    for (i = 0; i < sms_list.length; i++) {
        snprintf(q, sizeof q, "|%12.12s|%32.32s|%11d|%11p|%11p|%11p|\n",
                sms_list.item[i].data.phone_number,
                sms_list.item[i].data.message,
                sms_list.item[i].free,
                &sms_list.item[i],
                sms_list.item[i].next,
                sms_list.item[i].prev
                );
        sendStr(q, &crc);
    }
    sendStr("+------------+--------------------------------+-----------+-----------+-----------+-----------+\n", &crc);

    snprintf(q, sizeof q, "phone number buffer size: %d\n push item pointer: %p\n pop item pointer: %p\n", pn_list.length, pn_list.push_item, pn_list.pop_item);
    sendStr(q, &crc);
    sendStr("+------------------------------------------------------------+\n", &crc);
    sendStr("|                       phone number list                    |\n", &crc);
    sendStr("+------------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|phone_number|  is_free  |    ptr    | next_ptr  | prev_ptr  |\n", &crc);
    sendStr("+------------+-----------+-----------+-----------+-----------+\n", &crc);
    for (i = 0; i < pn_list.length; i++) {
        snprintf(q, sizeof q, "|%12.12s|%11d|%11p|%11p|%11p|\n",
                pn_list.item[i].data.phone_number,
                pn_list.item[i].free,
                &pn_list.item[i],
                pn_list.item[i].next,
                pn_list.item[i].prev
                );
        sendStr(q, &crc);
    }
    sendStr("+------------+-----------+-----------+-----------+-----------+\n", &crc);
    sendFooter(crc);
}

void printHelp() {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    sendStr("COMMAND LIST\n", &crc);
    snprintf(q, sizeof q, "%c\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tfirst put process in standby and then in active mode\n", ACP_CMD_APP_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tterminate process\n", ACP_CMD_APP_EXIT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tsend SMS; phone number and message are expected; quantifier does not matter\n", ACP_CMD_GWU59_SEND_SMS);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tring; phone number expected; quantifier does not matter\n", ACP_CMD_GWU59_RING);
    sendStr(q, &crc);
    sendFooter(crc);
}
