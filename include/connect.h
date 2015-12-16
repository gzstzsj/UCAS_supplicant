#ifndef __UCAS_CONNECT_H__
#define __UCAS_CONNECT_H__
#define MAXLINE 1048576
#define MAXINFOLINE 1024
#include <QString>

struct flow {
    int unit;
    float flow_value;
};

void trim(char*);

int readMessages(const char*);
int readResult(const char*);
int readFlow(const char*);
int getIndex(const char*);
int readQuery(const char*);
int readJid(const char*);
int read_info_1(const char*, char*, int);
int read_info_2(const char*, char*, int);
int read_info_3(const char*, char*, int);
int get_ret_code(const char*);
//QString read_info_gumbo_1(char*);
//int read_info_bare_1(char*);

//int get_success(void);
int login(void);
int logout(void);
int getflow(void);

#endif
