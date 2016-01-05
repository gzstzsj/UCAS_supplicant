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
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../include/connect.h"
#include "../include/qt_extended.hh"

#define INCRE 32
#define POSTFIELDOFFSET 10
#define LEN_PATH_POSTFIX 13
#define TRYLOCK_INTERVAL_SECOND 5
#define TRYLOCK_TOTAL_ATTEMPS 6
#define CONNECT_TIMEOUT_SECOND 60

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

static char flow_text[25];
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
Cookie: JSESSIONID=";
static const char* HTTP_HEADER_INFO_FIELD_2 = "GET /selfservice/module/userself/web/portal_chargetraffic.jsf HTTP/1.1\r\nHost: 121.195.186.149\r\n\
Cookie: JSESSIONID=";
static const char* HTTP_HEADER_INFO_FIELD_3 = "GET /selfservice/module/webcontent/web/portal_onlinedevice_list.jsf HTTP/1.1\r\nHost: 121.195.186.149\r\n\
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
static const size_t LENGTH_HEADER_INFO_FIELD_1 = 113;
static const size_t LENGTH_HEADER_INFO_FIELD_2 = 114;
static const size_t LENGTH_HEADER_INFO_FIELD_3 = 120;
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

static const char* PATH_POSTFIX = "/.ucas_uname";
static char* env_home;
static char* read_uname;
static char* rec_path;

static const char* AUTH_SERVER = "210.77.16.21";
static const char* INFO_SERVER = "121.195.186.149";
static const char* BAIDU_SERVER = "184.87.132.234";
static const uint16_t HTTP_PORT = 80;
static char receiveline[MAXLINE+1];
static char receiveline_keep[MAXLINE+1];
static int active_keep;
static pthread_mutex_t post_lock;
static pthread_mutex_t recv_lock;
static pthread_mutex_t keep_lock;

char error_message[LEN_ERROR];

static char to_hex(char code) 
{
    static char hex[] = "0123456789abcdef";
    return hex[code & 0xf];
}

static void urlencode(const char* input, char* output, size_t max_len)
{
    if (max_len == 0) return;
    const char *rdptr = input;
    char* wrptr = output;
    size_t tmpcnt = 0;
    while (*rdptr != '\0')
    {
        if (isalnum(*rdptr))
        {
            *wrptr ++ = *rdptr ++;
            ++ tmpcnt;
            if (tmpcnt == max_len - 1) break;
        }
        else
        {
            *wrptr ++ = '%';
            ++ tmpcnt;
            if (tmpcnt == max_len - 1) break;
            *wrptr ++ = '2';
            ++ tmpcnt;
            if (tmpcnt == max_len - 1) break;
            *wrptr ++ = '5';
            ++ tmpcnt;
            if (tmpcnt == max_len - 1) break;
            *wrptr ++ = to_hex((*rdptr) >> 4);
            ++ tmpcnt;
            if (tmpcnt == max_len - 1) break;
            *wrptr ++ = to_hex((*rdptr));
            ++ tmpcnt;
            if (tmpcnt == max_len - 1) break;
            ++ rdptr;
        }
    }
    *wrptr = '\0';
}

/* Connect to the Auth Server */
int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
	int				flags, n, error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;

	flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;

	error = 0;
	if ( (n = connect(sockfd, saptr, salen)) < 0)
		if (errno != EINPROGRESS)
			return -1;

	if (n == 0)
    {
        fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
        return 0;
    }
		//goto done;	/* connect completed immediately */

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd+1, &rset, &wset, NULL, nsec ? &tval : NULL)) == 0) 
    {
		close(sockfd);		/* timeout */
		errno = ETIMEDOUT;
		return -1;
	}

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			return -1;			/* Solaris pending error */
	} else
        return -2;

    fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */

	if (error) {
		close(sockfd);		/* just in case */
		errno = error;
		return -1;
	}
	return 0;
}

static int connect_err(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen)
{
    if (connect_nonb(sockfd, servaddr, addrlen, CONNECT_TIMEOUT_SECOND) == 0) return 0;
    else 
        switch (errno){
            case ETIMEDOUT:
                snprintf(error_message, LEN_ERROR, "Connection timed out!\n");
                return -1;
            case ECONNREFUSED:
                snprintf(error_message, LEN_ERROR, "Connection refused!\n");
                return -1;
            case EHOSTUNREACH:
                snprintf(error_message, LEN_ERROR, "Server unreachable!\n");
                return -1;
            case ENETUNREACH:
                snprintf(error_message, LEN_ERROR, "Server unreachable!\n");
                return -1;
            default:
                snprintf(error_message, LEN_ERROR, "Socket error!\n");
                return -1;
        }
}

static int addr_init(struct sockaddr_in *servaddr, const char *addr_host, const uint16_t addr_port)
{
    (void)memset(servaddr, 0, sizeof(struct sockaddr_in));
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(addr_port);
    if (inet_pton(AF_INET, addr_host, &(servaddr->sin_addr)) <= 0) {
        snprintf(error_message, LEN_ERROR, "Failed to initialize server address!\n");
        return -1;
    }
    return 0;
}

static int http_req(const char* server_addr, const uint16_t port, const char* http_header, size_t size_header, char* receiveline, size_t max_receive, int f_close)
{
    struct sockaddr_in servaddr;
    int socketd;
    ssize_t processed;
    size_t nleft = size_header;
    const char* wrptr = http_header;
    char* rdptr = receiveline;
    if (addr_init(&servaddr, server_addr, port) < 0)
        return -1;
    if ((socketd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        snprintf(error_message, LEN_ERROR, "Failed to initialize socket descriptor!\n");
        return -1;
    }
    if (connect_err(socketd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0)
        return -1;
    /* Write to the Socket */
    printf("here %x\n", pthread_self());
    while (nleft > 0) 
    {
        if ((processed = write(socketd, wrptr, nleft)) < 0)
        {
            if (errno == EINTR) processed = 0;
            else {
                snprintf(error_message, LEN_ERROR, "Write error!\n");
                close(socketd);
                return -2;
            }
        }
        nleft -= processed;
        wrptr += processed;
    }
    printf("here %x\n", pthread_self());
    (void)shutdown(socketd, SHUT_WR);
    nleft = max_receive;
    /* Read from the Socket */
    while ( nleft > 0 && (processed = read(socketd, rdptr, nleft)) > 0 )
    {
        rdptr += processed;
        nleft -= processed;
    }
    *rdptr = '\0';
    printf("here %x\n", pthread_self());
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
    retcode = http_req(AUTH_SERVER,HTTP_PORT, postfield, total_len, receiveline, 0, 1);
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    return retcode;
}

static int write_uname(QMain* m_pro, const char* uname)
{
    FILE* fp;
    int p;
    env_home = getenv("HOME");
    p = strlen(env_home) + LEN_PATH_POSTFIX;
    rec_path = new char[p];
    snprintf(rec_path, p, "%s%s", env_home, PATH_POSTFIX);
    fp = fopen(rec_path, "w");
    if (fp != NULL)
    {
        if (m_pro->get_remu())
            fprintf(fp, "1\n%s", uname);
        else 
            fprintf(fp, "0\n");
        fclose(fp);
        delete rec_path;
        return 0;
    }
    delete rec_path;
    p = 1 + LEN_PATH_POSTFIX;
    rec_path = new char[p];
    snprintf(rec_path, p, ".%s", PATH_POSTFIX);
    fp = fopen(rec_path, "w");
    if (fp != NULL)
    {
        if (m_pro->get_remu())
            fprintf(fp, "1\n%s", uname);
        else 
            fprintf(fp, "0\n");
        fclose(fp);
        delete rec_path;
        return 0;
    }
    delete rec_path;
    return -1;
}

int read_username()
{
    int fd;
    int p;
    int locsize;
    int unsize = INCRE;
    char bo;
    char* rdptr;
    env_home = getenv("HOME");
    p = strlen(env_home) + LEN_PATH_POSTFIX;
    rec_path = new char[p];
    snprintf(rec_path, p, "%s%s", env_home, PATH_POSTFIX);
    fd = open(rec_path, O_RDONLY);
    if (fd >= 0)
    {
        if (read(fd, &bo, 1) > 0 && bo == '1')
        {
            while (read(fd, &bo, 1) && bo != '\n');
            if (bo == '\n')
            {
                read_uname = (char*)malloc(unsize*sizeof(char));
                if (read_uname == NULL)
                {
                    snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
                    delete rec_path;
                    close(fd);
                    return -1;
                }
                rdptr = read_uname;
                while ( (locsize = read(fd, rdptr, INCRE)) == INCRE)
                {   
                    unsize += INCRE;
                    rdptr = (char*)realloc(read_uname, unsize*sizeof(char));
                    if (rdptr == NULL)
                    {
                        if (read_uname != NULL) free(read_uname);
                        snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
                        delete rec_path;
                        close(fd);
                        return -1;
                    }
                    read_uname = rdptr;
                    rdptr = read_uname + unsize - INCRE;
                }
                *(rdptr + locsize) = '\0';
                delete rec_path;
                close(fd);
                return 0;
            }
        }
        else 
        {
            delete rec_path;
            close(fd);
            return -2;
        }
    }
    p = 1 + LEN_PATH_POSTFIX;
    rec_path = new char[p];
    snprintf(rec_path, p, ".%s", PATH_POSTFIX);
    fd = open(rec_path, O_RDONLY);
    if (fd >= 0)
    {
        if (read(fd, &bo, 1) > 0 && bo == '1')
        {
            while (read(fd, &bo, 1) && bo != '\n');
            if (bo == '\n')
            {
                read_uname = (char*)malloc(unsize*sizeof(char));
                if (read_uname == NULL)
                {
                    snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
                    delete rec_path;
                    close(fd);
                    return -1;
                }
                rdptr = read_uname;
                while ( (locsize = read(fd, rdptr, INCRE)) == INCRE)
                {   
                    unsize += INCRE;
                    rdptr = (char*)realloc(read_uname, unsize*sizeof(char));
                    if (rdptr == NULL)
                    {
                        if (read_uname != NULL) free(read_uname);
                        snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
                        delete rec_path;
                        close(fd);
                        return -1;
                    }
                    read_uname = rdptr;
                    rdptr = read_uname + unsize - INCRE;
                }
                *(rdptr + locsize) = '\0';
                delete rec_path;
                close(fd);
                return 0;
            }
        }
        else 
        {
            delete rec_path;
            close(fd);
            return -2;
        }
    }
    delete rec_path;
    return -1;
}

void *QMain::keep_alive(void *arg)
{
    QMain *fake_this = (QMain*)arg;
    int p;

    pthread_mutex_lock(&keep_lock);
    ++ active_keep;
    pthread_mutex_unlock(&keep_lock);
    /* Prepare Post Field for Keepalive */
    while ( fake_this->isOffline == 0 )
    {
        if (http_req(BAIDU_SERVER, HTTP_PORT, HTTP_HEADER_KEEPALIVE, LENGTH_HEADER_KEEPALIVE, receiveline_keep, MAXLINE, 0) != 0)
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
        pthread_mutex_lock(&keep_lock);
        if (active_keep > 1)
        {
            -- active_keep;
            pthread_mutex_unlock(&keep_lock);
            return NULL;
        }
        pthread_mutex_unlock(&keep_lock);
    }

    if (fake_this->isOffline == 2)
    {
        snprintf(fake_this->message_server, 16, "Connection lost");
        fake_this->send();
        fake_this->send_logoff_success();
    }
    pthread_mutex_lock(&keep_lock);
    -- active_keep;
    pthread_mutex_unlock(&keep_lock);
    return NULL;
}

static int get_confirm(void *arg)
{
    unsigned int total_len;
    int att, ret;

    /* Prepare Post Field for Confirm */
    total_len = LENGTH_HEADER_CONFIRM_1 + LENGTH_HEADER_CONFIRM_2 + strlen(infoString);
    for (att = 0; (ret = pthread_mutex_trylock(&post_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) return -3;
    (void)snprintf(postfield, total_len+1, "%s%s%s%s", HTTP_HEADER_CONFIRM_1,\
            infoString, HTTP_HEADER_CONFIRM_2, userIndex);

    /* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        pthread_mutex_unlock(&post_lock);
        return -3;
    }
    if (http_req(INFO_SERVER, HTTP_PORT, postfield, total_len, receiveline, MAXLINE, 0) != 0)
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
    return 0;
}

void *QMain::get_info_1(void *arg)
{
    unsigned int total_len;
    int att, ret;
    QMain *fake_this = (QMain*)arg;

    while (fake_this->get_confirmed == 0 && !fake_this->isOffline)
        sleep(1);
    if (fake_this->isOffline) return NULL;

    /* Prepare Post Field for Info Request */
    total_len = LENGTH_HEADER_INFO_FIELD_1 + strlen(jsessionid) + 4;
    for (att = 0; (ret = pthread_mutex_trylock(&post_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) return NULL;
    (void)snprintf(postfield, total_len+1, "%s%s\r\n\r\n", HTTP_HEADER_INFO_FIELD_1, jsessionid);

    /* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        pthread_mutex_unlock(&post_lock);
        return NULL;
    }
    if (http_req(INFO_SERVER, HTTP_PORT, postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        read_info_1(receiveline, info_text, MAXINFOLINE);
        fake_this->send_info();
    }
    else fake_this->send_error();
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    return NULL;
}

void *QMain::get_info_2(void *arg)
{
    unsigned int total_len;
    int att, ret;
    QMain *fake_this = (QMain*)arg;

    while (fake_this->get_confirmed == 0 && !fake_this->isOffline)
        sleep(1);
    if (fake_this->isOffline) return NULL;

    /* Prepare Post Field for Info Request */
    total_len = LENGTH_HEADER_INFO_FIELD_2 + strlen(jsessionid) + 4;
    for (att = 0; (ret = pthread_mutex_trylock(&post_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) return NULL;
    (void)snprintf(postfield, total_len+1, "%s%s\r\n\r\n", HTTP_HEADER_INFO_FIELD_2, jsessionid);

    /* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        pthread_mutex_unlock(&post_lock);
        return NULL;
    }
    if (http_req(INFO_SERVER, HTTP_PORT, postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        read_info_2(receiveline, info_text, MAXINFOLINE);
        fake_this->send_info();
    }
    else fake_this->send_error();
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    return NULL;
}

void *QMain::get_info_3(void *arg)
{
    unsigned int total_len;
    int att, ret;
    QMain *fake_this = (QMain*)arg;

    while (fake_this->get_confirmed == 0 && !fake_this->isOffline)
        sleep(1);
    if (fake_this->isOffline) return NULL;

    /* Prepare Post Field for Info Request */
    total_len = LENGTH_HEADER_INFO_FIELD_3 + strlen(jsessionid) + 4;
    for (att = 0; (ret = pthread_mutex_trylock(&post_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) return NULL;
    (void)snprintf(postfield, total_len+1, "%s%s\r\n\r\n", HTTP_HEADER_INFO_FIELD_3, jsessionid);

    /* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        pthread_mutex_unlock(&post_lock);
        return NULL;
    }
    if (http_req(INFO_SERVER, HTTP_PORT, postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        read_info_3(receiveline, info_text, MAXINFOLINE);
        fake_this->send_info();
    }
    else fake_this->send_error();
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    return NULL;
}

void *QMain::login(void *arg)
{
    QMain *fake_this = (QMain*) arg;
    unsigned int lenuname;
    unsigned int lenpword;
    unsigned int total_len, total_len_temp, post_len;
    int p;
    int att, ret;
    char *username_t, *password_t;
    char *loginpost;
    char *username_raw, *password_raw; 

    if ( fake_this->userName.text().isEmpty() )
    {
        username_t = (char*)malloc(1);
        if (username_t == NULL)
        {
            snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
            fake_this->send_error();
            return NULL;
        }
        *username_t = '\0';
        username_raw = username_t;
    }
    else 
    {
        username_raw = fake_this->username.data();
        p = strlen(username_raw)*5 + 1;
        username_t = (char*)malloc(p*sizeof(char));
        if (username_t == NULL)
        {
            snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
            fake_this->send_error();
            return NULL;
        }
        urlencode(username_raw, username_t, p);
    }

    if ( fake_this->userName.text().isEmpty() )
    {
        password_t = (char*)malloc(1);
        if (password_t == NULL)
        {
            snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
            fake_this->send_error();
            free(username_t);
            return NULL;
        }
        *password_t = '\0';
    }
    else 
    {
        password_raw = fake_this->password.data();
        p = strlen(password_raw)*5 + 1;
        password_t = (char*)malloc((strlen(password_raw)*5 + 1)*sizeof(char));
        if (password_t == NULL)
        {
            snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
            fake_this->send_error();
            free(username_t);
            return NULL;
        }
        urlencode(password_raw, password_t, p);
    }

    lenuname = strlen(username_t);
    lenpword = strlen(password_t);

    /* Record Username */
    write_uname(fake_this, username_raw);

    /* Request queryString From Server */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        fake_this->send_error();
        fake_this->send_fail();
        memset(password_t, 1, lenpword);
        free(username_t);
        free(password_t);
        return NULL;
    }
    if (http_req(AUTH_SERVER, HTTP_PORT, HTTP_HEADER_REQID, LENGTH_HEADER_REQID, receiveline, MAXLINE, 0) != 0) 
    {
        pthread_mutex_unlock(&recv_lock);
        fake_this->send_error();
        fake_this->send_fail();
        memset(password_t, 1, lenpword);
        free(username_t);
        free(password_t);
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
        snprintf(error_message, LEN_ERROR, "Failed to malloc memory!\n");
        fake_this->send_error();
        fake_this->send_fail();
        memset(password_t, 1, lenpword);
        free(username_t);
        free(password_t);
        return NULL;
	}
	total_len = snprintf(loginpost, total_len+1, "%s%d%s%s%s%s%s%s", HTTP_HEADER_LOGIN,\
	        post_len, part1, username_t, part2, password_t, part3, queryString);

    memset(password_t, 1, lenpword);
    free(username_t);
    free(password_t);
	
	result[0] = '\0';
	messages[0] = '\0';
	userIndex[0] = '\0';
	
	/* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
	    memset(loginpost, 0, total_len);
	    free(loginpost);
        return NULL;
    }
	if (http_req(AUTH_SERVER, HTTP_PORT, loginpost, total_len, receiveline, MAXLINE, 0) == 0)
	{
	    if (readMessages((const char*)receiveline) == 0)
	    {
            pthread_mutex_unlock(&recv_lock);
	        if (strcmp(result, success) != 0) 
            {
                strncpy(fake_this->message_server, messages, 100);
                fake_this->send();
                fake_this->send_fail();
            }
            else 
            {
                fake_this->isOffline = 0;
	            if (get_success() != 0)
                    gfflag = 0;
                 fake_this->send_success();
            }
	    }
        else 
        {
            pthread_mutex_unlock(&recv_lock);
            fake_this->send_fail();
            snprintf(error_message, LEN_ERROR, "Failed to get returned message!\n");
            fake_this->send_error();
        }
	    memset(loginpost, 0, total_len);
	    free(loginpost);
        return NULL;
	}
    fake_this->send_fail();
    pthread_mutex_unlock(&recv_lock);
	memset(loginpost, 0, total_len);
	free(loginpost);
    fake_this->send_error();
    return NULL;
}

void *QMain::logout(void *arg)
{
    QMain *fake_this = (QMain*) arg;
    unsigned int total_len, post_len;
    unsigned int post_len_temp;
    int att, ret;
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
    for (att = 0; (ret = pthread_mutex_trylock(&post_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) return NULL;
    (void)snprintf(postfield, total_len+1, "%s%d\r\n\r\nuserIndex=%s", HTTP_HEADER_LOGOUT,\
            post_len, userIndex);

    /* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        pthread_mutex_unlock(&post_lock);
        return NULL;
    }
    if (http_req(AUTH_SERVER, HTTP_PORT, postfield, total_len, receiveline, MAXLINE, 0) == 0)
    {
        fake_this->isOffline = 1;
        if ( readMessages((const char*)receiveline) == 0)
        {
            strncpy(fake_this->message_server, messages, 100);
            fake_this->send();
        }
        pthread_mutex_unlock(&recv_lock);
        pthread_mutex_unlock(&post_lock);
        fake_this->send_logoff_success();
        fake_this->get_confirmed = 0;
        return NULL;
    }
    fake_this->isOffline = 1;
    pthread_mutex_unlock(&recv_lock);
    pthread_mutex_unlock(&post_lock);
    fake_this->send_error();
    fake_this->send_logoff_success();
    fake_this->get_confirmed = 0;
    return NULL;
}

void *QMain::getflow(void *arg)
{
    QMain *fake_this = (QMain*) arg;
    if (gfflag == 0) return NULL;
    unsigned int total_len, post_len;
    unsigned int post_len_temp;
    int att, ret;
    int tmp;
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
    for (att = 0; (ret = pthread_mutex_trylock(&post_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) return NULL;
    (void)snprintf(postfield, total_len+1, "%s%d\r\n\r\nuserIndex=%s", HTTP_HEADER_INFO,\
            post_len, userIndex);

    /* Send HTTP Request and Process the Response */
    for (att = 0; (ret = pthread_mutex_trylock(&recv_lock)) != 0 && att < TRYLOCK_TOTAL_ATTEMPS; ++att) sleep(TRYLOCK_INTERVAL_SECOND);
    if (ret != 0) 
    {
        pthread_mutex_unlock(&post_lock);
        return NULL;
    }
    if (http_req(AUTH_SERVER, HTTP_PORT, postfield, total_len, receiveline, MAXLINE, 0) == 0)
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
        else
            pthread_mutex_unlock(&recv_lock);
    }
    else 
    {
        pthread_mutex_unlock(&recv_lock);
        pthread_mutex_unlock(&post_lock);
    }
    return NULL;
}

int QMain::check_state()
{
    active_keep = 0;
    /* Request queryString From Server */
    pthread_mutex_init(&post_lock, NULL);
    pthread_mutex_init(&recv_lock, NULL);
    pthread_mutex_lock(&recv_lock);
    if (read_username() == 0)
    {
        restore_uname(read_uname);
        get_username(userName.text());
        storeUname.setCheckState(Qt::Checked);
    }
    if (http_req(AUTH_SERVER, HTTP_PORT, HTTP_HEADER_REQID, LENGTH_HEADER_REQID, receiveline, MAXLINE, 0) != 0) 
    {
        pthread_mutex_unlock(&recv_lock);
        isOffline = 1;
        send_error();
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
