
#ifndef GWU59_H
#define GWU59_H


#include "lib/util.h"
#include "lib/crc.h"

#include "lib/app.h"
#include "lib/timef.h"
#include "lib/udp.h"
#include "lib/serial.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/acp/mobile.h"

#define APP_NAME gwu59
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./"
#endif

#define CONFIG_FILE "" CONF_DIR "config.tsv"

#define WAIT_RESP_TIMEOUT 1
#define MESSAGE_SIZE 140
#define PHONE_NUMBER_SIZE 16


#define PROG_LIST_LOOP_ST  {Prog *curr = prog_list.top; while (curr != NULL) {
#define PROG_LIST_LOOP_SP curr = curr->next; }} 


typedef struct {
    char phone_number[PHONE_NUMBER_SIZE];
    char message[MESSAGE_SIZE];
} SMS;

typedef struct {
    char phone_number[PHONE_NUMBER_SIZE];
} PN;

DEC_FIFO_LIST(SMS)

DEC_FIFO_LIST(PN)

extern int readSettings();

extern int initSMS(FIFOItemList_SMS *list, size_t count);

extern int initData();

extern void initApp();

DEC_FUN_FIFO_PUSH(SMS)

DEC_FUN_FIFO_POP(SMS)

DEC_FUN_FIFO_PUSH(PN)

DEC_FUN_FIFO_POP(PN)

extern int sendSMS(SMS item, int fd);

extern void serverRun(int *state, int init_state);

extern void *threadFunction(void *arg);

extern int createThread_ctl();

extern void freeData();

extern void freeApp();

extern void exit_nicely();

extern void exit_nicely_e(char *s);


#endif 

