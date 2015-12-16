#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <iconv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../include/connect.h"
#include "../include/qt_extended.hh"

#define INCRE 16
#define UNAMELEN 20
#define POSTFIELDOFFSET 10

extern struct flow flow_current;
extern int gfflag;

extern char result[];
extern char uname_ret[];
extern char messages[];
extern char userIndex[];
extern char queryString[];
extern char infoString[];
extern char jsessionid[];
extern char info_text[];

static const char* HTTP_HEADER_LOGOUT = "POST /eportal/InterFace.do?method=logout HTTP/1.1\r\nHost: 210.77.16.21\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\
Content-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
static const char* HTTP_HEADER_INFO = "POST /eportal/InterFace.do?method=getOnlineUserInfo HTTP/1.1\r\nHost: 210.77.16.21\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\
Content-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
static const char* HTTP_HEADER_REQID = "GET /eportal/gologout.jsp HTTP/1.1\r\nHost: 210.77.16.21\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\r\n";
static const char* HTTP_HEADER_LOGIN = "POST /eportal/InterFace.do?method=login&time=null HTTP/1.1\r\nHost: 210.77.16.21\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\
Content-Type: application/x-www-form-urlencoded\r\nContent-Length: ";
static const char* HTTP_HEADER_SUCCESS_1 = "GET /eportal/interface/GKC/success.html?userIndex=";
static const char* HTTP_HEADER_SUCCESS_2 = " HTTP/1.1\r\nHost: 210.77.16.21\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\r\n";
static const char* HTTP_HEADER_CONFIRM_1 = "GET /selfservice/module/userself/web/portal_business_detail.jsf?";
static const char* HTTP_HEADER_CONFIRM_2 = " HTTP/1.1\r\nHost: 121.195.186.149\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\r\n";
static const char* HTTP_HEADER_INFO_FIELD_1 = "GET /selfservice/module/userself/web/portal_packagemoney.jsf HTTP/1.1\r\nHost: 121.195.186.149\r\n\
Upgrade-Insecure-Requests: 1\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\
Cookie: JSESSIONID=";
static const char* HTTP_HEADER_INFO_FIELD_2 = "GET /selfservice/module/userself/web/portal_lasttraffic.jsf HTTP/1.1\r\nHost: 121.195.186.149\r\n\
Upgrade-Insecure-Requests: 1\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\
Cookie: JSESSIONID=";
static const char* HTTP_HEADER_INFO_FIELD_3 = "GET /selfservice/module/webcontent/web/portal_onlinedevice_list.jsf HTTP/1.1\r\nHost: 121.195.186.149\r\n\
Upgrade-Insecure-Requests: 1\r\nUser-Agent: \
Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.85 Safari/537.36\r\n\
Cookie: JSESSIONID=";
static const char* HTTP_HEADER_KEEPALIVE = "GET / HTTP/1.1\r\nHost: 184.87.132.234\r\n\r\n";
static const size_t LENGTH_HEADER_LOGOUT = 254;
static const size_t LENGTH_HEADER_INFO = 265;
static const size_t LENGTH_HEADER_REQID = 176;
static const size_t LENGTH_HEADER_LOGIN = 263;
static const size_t LENGTH_HEADER_SUCCESS_1 = 50;
static const size_t LENGTH_HEADER_SUCCESS_2 = 151;
static const size_t LENGTH_HEADER_CONFIRM_1 = 66;
static const size_t LENGTH_HEADER_CONFIRM_2 = 154;
static const size_t LENGTH_HEADER_INFO_FIELD_1 = 261;
static const size_t LENGTH_HEADER_INFO_FIELD_2 = 260;
static const size_t LENGTH_HEADER_INFO_FIELD_3 = 268;
static const size_t LENGTH_HEADER_KEEPALIVE = 40;

static const char* success = "success";
static const char* part1 = "\r\n\r\nuserId=";
static const char* part2 = "&password=";
static const char* part3 = "&service=&queryString=";
static const unsigned int len1 = 11;
static const unsigned int len2 = 10;
static const unsigned int len3 = 22;
static const struct timespec wait_time = { 0, 100000000 };
static char postfield[571];
static pthread_mutex_t post_lock;

static const char* AUTH_SERVER = "210.77.16.21";
static const uint16_t AUTH_PORT = 80;
static const char* INFO_SERVER = "121.195.186.149";
static const uint16_t INFO_PORT = 80;
static const char* BAIDU_SERVER = "184.87.132.234";
static const uint16_t BAIDU_PORT = 80;
static char receiveline[MAXLINE+1];
static pthread_mutex_t recv_lock;
static char receiveline_keep[MAXLINE+1];

static char to_hex(char code) 
{
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}

static void urlencode(const char* input, char* output)
{
    const char *rdptr = input;
    char* wrptr = output;
    while (*rdptr != '\0')
    {
        if (isalnum(*rdptr))
            *wrptr ++ = *rdptr ++;
        else
        {
            *wrptr ++ = '%';
            *wrptr ++ = '2';
            *wrptr ++ = '5';
            *wrptr ++ = to_hex((*rdptr) >> 4);
            *wrptr ++ = to_hex((*rdptr));
            ++ rdptr;
        }
    }
    *wrptr = '\0';
}

/* Connect to the Auth Server */
static int connect_err(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen)
{
    if (connect(sockfd, servaddr, addrlen) == 0) return 0;
    else 
        switch (errno){
            case ETIMEDOUT:
                printf("Connection timed out!\n");
                return -1;
            case ECONNREFUSED:
                printf("Connection refused!\n");
                return -1;
            case EHOSTUNREACH:
                printf("Server unreachable!\n");
                return -1;
            case ENETUNREACH:
                printf("Server unreachable!\n");
                return -1;
            default:
                return -1;
        }
}

static int addr_init(struct sockaddr_in *servaddr, const char *addr_host, const uint16_t addr_port)
{
    (void)memset(servaddr, 0, sizeof(struct sockaddr_in));
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(addr_port);
    if (inet_pton(AF_INET, addr_host, &(servaddr->sin_addr)) <= 0) {
        printf("Failed to initialize server address!\n");
        return -1;
    }
    return 0;
}

static int http_req(const char* http_header, size_t size_header, char* receiveline, size_t max_receive, int f_close)
{
    struct sockaddr_in servaddr;
    int socketd;
    ssize_t processed;
    size_t nleft = size_header;
    const char* wrptr = http_header;
    char* rdptr = receiveline;
    if (addr_init(&servaddr, AUTH_SERVER, AUTH_PORT) < 0)
        return -1;
    if ((socketd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Failed to initialize socket descriptor!\n");
        return -1;
    }
    if (connect_err(socketd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0)
        return -1;
    /* Write to the Socket */
    while (nleft > 0) 
    {
        if ((processed = write(socketd, wrptr, nleft)) < 0)
        {
            if (errno == EINTR) processed = 0;
            else {
                printf("Write error!\n");
                close(socketd);
                return -2;
            }
        }
        nleft -= processed;
        wrptr += processed;
    }
    (void)shutdown(socketd, SHUT_WR);
    nleft = max_receive;
    /* Read from the Socket */
    while ( nleft > 0 && (processed = read(socketd, rdptr, nleft)) > 0 )
    {
        rdptr += processed;
        nleft -= processed;
    }
    *rdptr = '\0';
    if (f_close)
        (void)shutdown(socketd, SHUT_RD);
    return 0;
}

static int conf_req(const char* http_header, size_t size_header, char* receiveline, size_t max_receive, int f_close)
{
    struct sockaddr_in servaddr;
    int socketd;
    ssize_t processed;
    size_t nleft = size_header;
    const char* wrptr = http_header;
    char* rdptr = receiveline;
    if (addr_init(&servaddr, INFO_SERVER, INFO_PORT) < 0)
        return -1;
    if ((socketd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Failed to initialize socket descriptor!\n");
        return -1;
    }
    if (connect_err(socketd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0)
        return -1;
    /* Write to the Socket */
    while (nleft > 0) 
    {
        if ((processed = write(socketd, wrptr, nleft)) < 0)
        {
            if (errno == EINTR) processed = 0;
            else {
                printf("Write error!\n");
                close(socketd);
                return -2;
            }
        }
        nleft -= processed;
        wrptr += processed;
    }
    (void)shutdown(socketd, SHUT_WR);
    nleft = max_receive;
    /* Read from the Socket */
    while ( nleft > 0 && (processed = read(socketd, rdptr, nleft)) > 0 )
    {
        rdptr += processed;
        nleft -= processed;
    }
    *rdptr = '\0';
    if (f_close)
        (void)shutdown(socketd, SHUT_RD);
    return 0;
}

static int baidu_req(const char* http_header, size_t size_header, char* receiveline, size_t max_receive, int f_close)
{
    struct sockaddr_in servaddr;
    int socketd;
    ssize_t processed;
    size_t nleft = size_header;
    const char* wrptr = http_header;
    char* rdptr = receiveline;
    if (addr_init(&servaddr, BAIDU_SERVER, BAIDU_PORT) < 0)
        return -1;
    if ((socketd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Failed to initialize socket descriptor!\n");
        return -1;
    }
    if (connect_err(socketd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0)
        return -1;
    /* Write to the Socket */
    while (nleft > 0) 
    {
        if ((processed = write(socketd, wrptr, nleft)) < 0)
        {
            if (errno == EINTR) processed = 0;
            else {
                printf("Write error!\n");
                close(socketd);
                return -2;
            }
        }
        nleft -= processed;
        wrptr += processed;
    }
    (void)shutdown(socketd, SHUT_WR);
    nleft = max_receive;
    /* Read from the Socket */
    while ( nleft > 0 && (processed = read(socketd, rdptr, nleft)) > 0 )
    {
        rdptr += processed;
        nleft -= processed;
    }
    *rdptr = '\0';
    if (f_close)
        (void)shutdown(socketd, SHUT_RD);
    return 0;
}

static int get_success()
{
    unsigned int total_len;
    int retcode;

    /* Prepare Post Field for Success */
    total_len = LENGTH_HEADER_SUCCESS_1 + LENGTH_HEADER_SUCCESS_2 + strlen(userIndex);
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%s%s", HTTP_HEADER_SUCCESS_1,\
            userIndex, HTTP_HEADER_SUCCESS_2);

    /* Send HTTP Request and Process the Response */
    nanosleep(&wait_time, NULL);
    pthread_mutex_lock(&recv_lock);
    retcode = http_req(postfield, total_len, receiveline, 0, 1);
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    return retcode;
}

void *QMain::keep_alive(void *arg)
{
    QMain *fake_this = (QMain*)arg;
    int p;

    /* Prepare Post Field for Keepalive */
    while ( fake_this->isOffline == 0 )
    {
        if (baidu_req(HTTP_HEADER_KEEPALIVE, LENGTH_HEADER_KEEPALIVE, receiveline_keep, MAXLINE, 0) != 0)
        {
            fake_this->isOffline = 2;
            continue;
        }
        p = get_ret_code(receiveline_keep);
        if (p == 1 || p < -256)
        {
            fake_this->isOffline = 2;
            continue;
        }
        sleep(30);
    }

    if (fake_this->isOffline == 2)
    {
        fake_this->message_server = QString("Connection Lost");
        fake_this->send();
        fake_this->send_logoff_success();
    }
    return NULL;
}

static int get_confirm(void *arg)
{
    unsigned int total_len;

    /* Prepare Post Field for Confirm */
    total_len = LENGTH_HEADER_CONFIRM_1 + LENGTH_HEADER_CONFIRM_2 + strlen(infoString);
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%s%s%s", HTTP_HEADER_CONFIRM_1,\
            infoString, HTTP_HEADER_CONFIRM_2, userIndex);

    /* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
    if (conf_req(postfield, total_len, receiveline, MAXLINE, 0) != 0)
    {
        pthread_mutex_unlock(&recv_lock);
        pthread_mutex_unlock(&post_lock);
        return -1;
    }
    pthread_mutex_unlock(&post_lock);
    if (readJid((const char*)receiveline) != 0)
    {
        pthread_mutex_unlock(&recv_lock);
        return -2;
    }
    pthread_mutex_unlock(&recv_lock);
    //get_info_test(arg);
    return 0;
}

void *QMain::get_info_1(void *arg)
{
    unsigned int total_len;
    QMain *fake_this = (QMain*)arg;

    while (fake_this->get_confirmed == 0)
        //nanosleep(&wait_time, NULL);
        get_confirm(arg);

    /* Prepare Post Field for Info Request */
    total_len = LENGTH_HEADER_INFO_FIELD_1 + strlen(jsessionid) + 4;
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%s\r\n\r\n", HTTP_HEADER_INFO_FIELD_1, jsessionid);

    /* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
    if (conf_req(postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        read_info_1(receiveline, info_text, MAXINFOLINE);
        fake_this->send_info();
    }
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    //fake_this->show_info(info_text);
    return NULL;
}

void *QMain::get_info_2(void *arg)
{
    unsigned int total_len;
    QMain *fake_this = (QMain*)arg;

    while (fake_this->get_confirmed == 0)
        //nanosleep(&wait_time, NULL);
        get_confirm(arg);

    /* Prepare Post Field for Info Request */
    total_len = LENGTH_HEADER_INFO_FIELD_2 + strlen(jsessionid) + 4;
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%s\r\n\r\n", HTTP_HEADER_INFO_FIELD_2, jsessionid);

    /* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
    if (conf_req(postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        read_info_2(receiveline, info_text, MAXINFOLINE);
        fake_this->send_info();
    }
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    //fake_this->show_info(info_text);
    return NULL;
}

void *QMain::get_info_3(void *arg)
{
    unsigned int total_len;
    QMain *fake_this = (QMain*)arg;

    while (fake_this->get_confirmed == 0)
        //nanosleep(&wait_time, NULL);
        get_confirm(arg);

    /* Prepare Post Field for Info Request */
    total_len = LENGTH_HEADER_INFO_FIELD_3 + strlen(jsessionid) + 4;
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%s\r\n\r\n", HTTP_HEADER_INFO_FIELD_3, jsessionid);

    /* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
    if (conf_req(postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        read_info_3(receiveline, info_text, MAXINFOLINE);
        fake_this->send_info();
    }
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    //fake_this->show_info(info_text);
    return NULL;
}

void *QMain::login(void *arg)
{
    QMain *fake_this = (QMain*) arg;
    unsigned int lenuname;
    unsigned int lenpword;
    char* username_raw = fake_this->username.data();
    char* username_t = (char*)malloc((strlen(username_raw)*5 + 1)*sizeof(char));
    if (username_t == NULL)
    {
        fake_this->retcode = -10;
        printf("Failed to malloc memory!\n");
    }
    const char* password_raw = fake_this->password.data();
    char* password_t = (char*)malloc((strlen(password_raw)*5 + 1)*sizeof(char));
    if (password_t == NULL)
    {
        fake_this->retcode = -10;
        printf("Failed to malloc memory!\n");
    }
    char *loginpost;
    //int need_success = 1;
    urlencode(username_raw, username_t);
    urlencode(password_raw, password_t);
    lenuname = strlen(username_t);
    lenpword = strlen(password_t);
    unsigned int total_len, total_len_temp, post_len;

    /* Request queryString From Server */
    pthread_mutex_lock(&recv_lock);
    if (http_req(HTTP_HEADER_REQID, LENGTH_HEADER_REQID, receiveline, MAXLINE, 1) != 0) 
    {
        pthread_mutex_unlock(&recv_lock);
        fake_this->retcode = -1;
        fake_this->send_fail();
        return NULL;
    }
    if (readQuery((const char*)receiveline) != 0)
        snprintf(queryString, 5, "null");
    
    pthread_mutex_unlock(&recv_lock);

	/* Prepare Login Post Field */
	total_len = len1 + len2 + len3 + lenuname + lenpword - 4 + strlen(queryString);
	total_len_temp = total_len;
	post_len = total_len;
	while (total_len_temp >= 1){
	    total_len_temp /= 10;
	    ++total_len;
	}
	total_len += (LENGTH_HEADER_LOGIN+4+strlen(queryString));
	loginpost = (char*)malloc(sizeof(char)*(total_len+1));
	if (loginpost == NULL) {
        fake_this->retcode = -1;
        fake_this->send_fail();
        return NULL;
	}
	(void)snprintf(loginpost, total_len+1, "%s%d%s%s%s%s%s%s", HTTP_HEADER_LOGIN,\
	        post_len, part1, username_t, part2, password_t, part3, queryString);

    memset(password_t, 1, lenpword);
	
	result[0] = '\0';
	messages[0] = '\0';
	userIndex[0] = '\0';
	
	/* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
	if (http_req(loginpost, total_len, receiveline, MAXLINE, 0) == 0)
	{
	    if (readMessages((const char*)receiveline) == 0)
	    {
            pthread_mutex_unlock(&recv_lock);
	        if (strcmp(result, success) != 0) 
            {
                fake_this->message_server = QString((messages));
                fake_this->send();
                fake_this->send_fail();
            }
            else 
            {
                fake_this->isOffline = 0;
	            if (get_success() != 0)
                    gfflag = 0;
                /*
                    fake_this->del_off_layout();
                    fake_this->set_on_layout();
                    */
                 fake_this->send_success();
            }
	    }
        else 
            fake_this->send_fail();
        pthread_mutex_unlock(&recv_lock);
	    memset(loginpost, 0, total_len);
	    free(loginpost);
        fake_this->retcode = 0;
        return NULL;
	}
    fake_this->send_fail();
    pthread_mutex_unlock(&recv_lock);
	memset(loginpost, 0, total_len);
	free(loginpost);
    fake_this->retcode = -2;
    return NULL;
    //else {
        /* Request userIndex From Server */
    /*
        if (getIndex((const char*)receiveline) != 0) {
            return -3;
        }
        return 1;
    }
    */
}

void *QMain::logout(void *arg)
{
    QMain *fake_this = (QMain*) arg;
    unsigned int total_len, post_len;
    unsigned int post_len_temp;
    messages[0] = '\0';

    fake_this->rem_flow.setText("");
    /* Prepare Post Field for Logout */
    post_len = strlen(userIndex) + 10;
    post_len_temp = post_len;
    total_len = LENGTH_HEADER_LOGOUT + 4 + post_len;
    while (post_len_temp >= 1)
    {
        post_len_temp /= 10;
        ++total_len;
    }
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%d\r\n\r\nuserIndex=%s", HTTP_HEADER_LOGOUT,\
            post_len, userIndex);

    /* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
    if (http_req(postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        fake_this->isOffline = 1;
        if ( readMessages((const char*)receiveline) == 0)
        {
            fake_this->message_server = QString((messages));
            fake_this->send();
        }
        pthread_mutex_unlock(&recv_lock);
        pthread_mutex_unlock(&post_lock);
        fake_this->send_logoff_success();
        /*
        fake_this->del_on_layout();
        fake_this->set_off_layout();
        */
        fake_this->retcode = 0;
        fake_this->get_confirmed = 0;
        return NULL;
    }
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    fake_this->retcode = -3;
    fake_this->get_confirmed = 0;
    return NULL;
}

void *QMain::getflow(void *arg)
{
    QMain *fake_this = (QMain*) arg;
    if (gfflag == 0) return NULL;
    unsigned int total_len, post_len;
    unsigned int post_len_temp;
    char flow_text[25];
    int tmp;
    int ret;
    messages[0] = '\0';
    flow_text[0] = '\0';

    /* Prepare Post Field for GetOnlineUserInfo */
    post_len = strlen(userIndex) + 10;
    post_len_temp = post_len;
    total_len = LENGTH_HEADER_INFO + 4 + post_len;
    while (post_len_temp >= 1)
    {
        post_len_temp /= 10;
        ++total_len;
    }
    pthread_mutex_lock(&post_lock);
    (void)snprintf(postfield, total_len+1, "%s%d\r\n\r\nuserIndex=%s", HTTP_HEADER_INFO,\
            post_len, userIndex);

    /* Send HTTP Request and Process the Response */
    pthread_mutex_lock(&recv_lock);
    if (http_req(postfield, total_len, receiveline, MAXLINE, 1) == 0)
    {
        pthread_mutex_unlock(&post_lock);
        if ( (ret = readFlow((const char*)receiveline)) == 0)
        {
            pthread_mutex_unlock(&recv_lock);
            tmp = snprintf(flow_text, 10, "%.2f", flow_current.flow_value);
            if (flow_current.unit)
                snprintf(flow_text+tmp, 14, "MB Remaining.");
            else
                snprintf(flow_text+tmp, 14, "GB Remaining.");
            if (get_confirm(fake_this) == 0)
            {
                fake_this->rem_flow.setText(QString("Current user: ").append(uname_ret).append("\n").append(flow_text));
                fake_this->get_confirmed = 1;
            }
        }
        pthread_mutex_unlock(&recv_lock);
    }
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    return NULL;
}

int QMain::check_state()
{
    /* Request queryString From Server */
    pthread_mutex_init(&post_lock, NULL);
    pthread_mutex_init(&recv_lock, NULL);
    pthread_mutex_lock(&recv_lock);
    if (http_req(HTTP_HEADER_REQID, LENGTH_HEADER_REQID, receiveline, MAXLINE, 1) != 0) 
    {
        pthread_mutex_unlock(&recv_lock);
        isOffline = 1;
        return -2;
    }

    /* Check if Offline */
    if (readQuery((const char*)receiveline) == 0) 
    {
        pthread_mutex_unlock(&recv_lock);
        isOffline = 1;
        return 0;
    }

    snprintf(queryString, 5, "null");

    /* Check if Online */
    if (getIndex((const char*)receiveline) == 0) 
    {
        pthread_mutex_unlock(&recv_lock);
        isOffline = 0;
        return 1;
    }

    /* Unknown Error */
    pthread_mutex_unlock(&recv_lock);
    isOffline = 1;
    return -1;
}
