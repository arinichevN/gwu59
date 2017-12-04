/*
 * gwu59
 */

#include "main.h"

FUN_FIFO_PUSH(SMS)

FUN_FIFO_POP(SMS)

FUN_FIFO_PUSH(PN)

FUN_FIFO_POP(PN)


void printData(ACPResponse *response) {
    extern FIFOItemList_SMS sms_list;
    extern FIFOItemList_PN pn_list;
    char q[LINE_SIZE];
    size_t i;
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    SEND_STR(q)
    snprintf(q, sizeof q, "b_count: %d\n", b_count);
    SEND_STR(q)
    snprintf(q, sizeof q, "serial_path: %s\n", serial_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "serial_baud_rate: %d\n", serial_baud_rate);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    SEND_STR(q)
    snprintf(q, sizeof q, "SMS buffer size: %d\n push item pointer: %p\n pop item pointer: %p\n", sms_list.length, (void *) sms_list.push_item, (void *) sms_list.pop_item);
    SEND_STR(q)
    SEND_STR("+---------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                              SMS list                                       |\n")
    SEND_STR("+------------+--------------------------------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|phone_number|            message             |  is_free  |    ptr    | next_ptr  | prev_ptr  |\n")
    SEND_STR("+------------+--------------------------------+-----------+-----------+-----------+-----------+\n")
    for (i = 0; i < sms_list.length; i++) {
        snprintf(q, sizeof q, "|%12.12s|%32.32s|%11d|%11p|%11p|%11p|\n",
                sms_list.item[i].data.phone_number,
                sms_list.item[i].data.message,
                sms_list.item[i].free,
                (void *) &sms_list.item[i],
                (void *) sms_list.item[i].next,
                (void *) sms_list.item[i].prev
                );
        SEND_STR(q)
    }
    SEND_STR("+------------+--------------------------------+-----------+-----------+-----------+-----------+\n")

    snprintf(q, sizeof q, "phone number buffer size: %d\n push item pointer: %p\n pop item pointer: %p\n", pn_list.length, (void *) pn_list.push_item, (void *) pn_list.pop_item);
    SEND_STR(q)
    SEND_STR("+------------------------------------------------------------+\n")
    SEND_STR("|                       phone number list                    |\n")
    SEND_STR("+------------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|phone_number|  is_free  |    ptr    | next_ptr  | prev_ptr  |\n")
    SEND_STR("+------------+-----------+-----------+-----------+-----------+\n")
    for (i = 0; i < pn_list.length; i++) {
        snprintf(q, sizeof q, "|%12.12s|%11d|%11p|%11p|%11p|\n",
                pn_list.item[i].data.phone_number,
                pn_list.item[i].free,
                (void *) &pn_list.item[i],
                (void *) pn_list.item[i].next,
                (void *) pn_list.item[i].prev
                );
        SEND_STR(q)
    }
    SEND_STR_L("+------------+-----------+-----------+-----------+-----------+\n")
}

void printHelp(ACPResponse *response) {
    char q[LINE_SIZE];
    SEND_STR("COMMAND LIST\n")
    snprintf(q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tfirst put process in standby and then in active mode\n", ACP_CMD_APP_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tsend SMS; phone number and message are expected\n", ACP_CMD_MOBILE_SEND_SMS);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tring; phone number expected\n", ACP_CMD_MOBILE_RING);
    SEND_STR_L(q)
}
