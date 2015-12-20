#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <QTextCodec>
#include "../include/connect.h"
#define GB 1073741824
#define MB 1048576
#define SEPARATOR ','
#define EDFLAG '}'
#define STFLAG ':'
#define QUOTE '"'

extern char info_text[];
extern char error_message[];

struct flow flow_current;
int gfflag;

char result[15];
char uname_ret[20];
char jsessionid[33];
char messages[100];
char userIndex[200];
char infoString[280];
char queryString[300];
char info_ret_title[512];
char info_ret_utf8_title[512];
char info_ret_field[512];
char info_ret_utf8_field[512];

static const char* RES = "result\"";
static const char* MES = "message\"";
static const char* UID = "userIndex\"";
static const char* XQINFO = "套餐详情";
static const char* COMM = "<!--";

static char seg_start[20];
static char seg_end[20];
static char seg_rate[20];
static char seg_drate[20];

static const double max = 999999999999;

typedef struct seg
{
    double f_start;
    double f_end;
    double f_rate;
    double f_drate;
    struct seg* next;
} 
seg_info;

typedef enum 
{
    charge,free1,free2,free3,other
} 
f_state;

void trim(char* totrim)
{
    const char* rdptr = totrim;
    char* wrptr = totrim;
    while (*rdptr != '\0')
    {
        if (*rdptr == '<') 
        {
            rdptr = strstr(rdptr, ">");
            if (rdptr == NULL) break;
            ++ rdptr;
        }
        if (*rdptr == '\t' || *rdptr == ' ' || *rdptr == '\n' || *rdptr == '\r')
            ++ rdptr;
        else 
            *wrptr ++ = *rdptr ++;
    }
    *wrptr = '\0';
}

void trim_only_space_comm(char* totrim)
{
    const char* rdptr = totrim;
    char* wrptr = totrim;
    char comm_head[5];
    int inDesc = 0;
    while (*rdptr != '\0')
    {
        if (*rdptr == '<') 
        {
            inDesc = 1;
            snprintf(comm_head, 5, rdptr);
            if ( strcmp(COMM, comm_head) == 0 )
            {
                rdptr = strstr(rdptr, "-->");
                if (rdptr == NULL) break;
                rdptr += 3;
                inDesc = 0;
            }
        }
        if (*rdptr == '>') inDesc = 0;
        if (inDesc == 0 && (*rdptr == '\t' || *rdptr == ' ' || *rdptr == '\n' || *rdptr == '\r'))
            ++ rdptr;
        else 
            *wrptr ++ = *rdptr ++;
    }
    *wrptr = '\0';
}

void trim_nonaggresive(char* totrim)
{
    const char* rdptr = totrim;
    char* wrptr = totrim;
    while (*rdptr == '\t' || *rdptr == ' ' || *rdptr == '\n' || *rdptr == '\r')
        ++ rdptr;
    while (*rdptr != '\0')
        *wrptr ++ = *rdptr ++;
    if (wrptr != totrim)
    {
        while (*wrptr == '\t' || *wrptr == ' ' || *wrptr == '\n' || *wrptr == '\r')
            -- wrptr;
        ++ wrptr;
    }
    *wrptr = '\0';
}

int readMessages(const char* input)
{
    const char* tmpptr = input;
    const char* tmpcmp;
    char* tmpwrt;
    int state = 0;
    int tmpcnt;
    int ignore_separator = 0;
    while(*tmpptr != '\0')
    {
        switch (state) {
            case -1:
                /* Invalid Input */
                if (*tmpptr == SEPARATOR) state = 0;
                break;
            case 0: 
                if (*tmpptr == QUOTE) state = 1;
                break;
            case 1:
                switch(*tmpptr){
                    case 'r':
                        state = 10;
                        break;
                    case 'm':
                        state = 20;
                        break;
                    case 'u':
                        state = 30;
                        break;
                    default:
                        state = -1;
                }
                break;
            case 10:
                /* Entering result */
                tmpcmp = RES+1;
                while (*tmpptr != '\0' && *tmpcmp != '\0') {
                    if (*tmpptr != *tmpcmp) {
                        state = -1;
                        break;
                    }
                    ++tmpptr;
                    ++tmpcmp;
                }
                --tmpptr;
                if (state == 10) state = 15;
                break;
            case 15:
                if (*tmpptr == STFLAG) state = 16;
                break;
            case 16:
                /* Reading result */
                tmpcnt = 0;
                tmpwrt = result;
                while (ignore_separator || (*tmpptr != SEPARATOR && *tmpptr != EDFLAG)) {
                    if (*tmpptr == '\0') return -1;
                    if (tmpcnt > 14) return -2;
                    if (*tmpptr == QUOTE) 
                    {
                        ignore_separator = !ignore_separator;
                        ++ tmpptr;
                        continue;
                    }
                    *tmpwrt = *tmpptr;
                    ++tmpptr;
                    ++tmpwrt;
                    ++tmpcnt;
                }
                *tmpwrt = '\0';
                state = 0;
                break;
            case 20:
                /* Entering message */
                tmpcmp = MES+1;
                while (*tmpptr != '\0' && *tmpcmp != '\0') {
                    if (*tmpptr != *tmpcmp) {
                        state = -1;
                        break;
                    }
                    ++tmpptr;
                    ++tmpcmp;
                }
                --tmpptr;
                if (state == 20) state = 25;
                break;
            case 25:
                if (*tmpptr == STFLAG) state = 26;
                break;
            case 26:
                /* Reading message */
                tmpwrt = messages;
                tmpcnt = 0;
                while (ignore_separator || (*tmpptr != SEPARATOR && *tmpptr != EDFLAG)) {
                    if (*tmpptr == '\0') return -1;
                    if (tmpcnt > 99) return -2;
                    if (*tmpptr == QUOTE) 
                    {
                        ignore_separator = !ignore_separator;
                        ++ tmpptr;
                        continue;
                    }
                    *tmpwrt = *tmpptr;
                    ++tmpptr;
                    ++tmpwrt;
                    ++tmpcnt;
                }
                *tmpwrt = '\0';
                state = 0;
                break;
            case 30:
                /* Entering userIndex */
                tmpcmp = UID+1;
                while (*tmpptr != '\0' && *tmpcmp != '\0') {
                    if (*tmpptr != *tmpcmp) {
                        state = -1;
                        break;
                    }
                    ++tmpptr;
                    ++tmpcmp;
                }
                --tmpptr;
                if (state == 30) state = 35;
                break;
            case 35:
                if (*tmpptr == STFLAG) state = 36;
                break;
            case 36:
                /* Reading userIndex */
                tmpwrt = userIndex;
                tmpcnt = 0;
                while (ignore_separator || (*tmpptr != SEPARATOR && *tmpptr != EDFLAG)) {
                    if (*tmpptr == '\0') return -1;
                    if (tmpcnt > 199) return -2;
                    if (*tmpptr == QUOTE)
                    {
                        ignore_separator = !ignore_separator;
                        ++ tmpptr;
                        continue;
                    }
                    *tmpwrt = *tmpptr;
                    ++tmpptr;
                    ++tmpwrt;
                    ++tmpcnt;
                }
                *tmpwrt = '\0';
                state = 0;
                break;
            default:
                ;
        }
        ++tmpptr;
    }
    return 0;
}

int readResult(const char* input)
{
    char suc[8];
    const char* rdptr = strstr(input, "result\":\"");
    if (rdptr == NULL) return -1;
    rdptr += 9;
    snprintf(suc, 8, rdptr);
    return strcmp(suc, "success");
}

int readFlow(const char* input)
{
    const char *ptr = input;
    char *wrptr = uname_ret;
    int tmpcnt = 0;
    double flowinbyte;
    ptr = strstr(ptr, "userName\":\"");
    if (ptr == NULL) 
    {
        snprintf(error_message, LEN_ERROR, "Failed to get user information!\n");
        return -1;
    }
    ptr += 11;
    while (tmpcnt < 19 && *ptr != '"' && *ptr != '\0')
    {
        *wrptr ++ = *ptr ++;
        ++ tmpcnt;
    }
    *wrptr = '\0';
    ptr = strstr(ptr, "flow");
    if (ptr == NULL) 
    {
        snprintf(error_message, LEN_ERROR, "Failed to get user information!\n");
        return -1;
    }
    ptr = strstr(ptr, "value\\\":\\\"");
    if (ptr == NULL) 
    {
        snprintf(error_message, LEN_ERROR, "Failed to get user information!\n");
        return -2;
    }
    ptr += 10;
    flowinbyte = atof(ptr);
    if (flow_current.unit = (flowinbyte < GB))
        flow_current.flow_value = flowinbyte/(float)MB;
    else 
        flow_current.flow_value = flowinbyte/(float)GB;
    ptr = strstr(ptr, "url");
    if (ptr == NULL) 
    {
        snprintf(error_message, LEN_ERROR, "Failed to get user information!\n");
        return -3;
    }
    ptr = strstr(ptr, "?");
    if (ptr == NULL) 
    {
        snprintf(error_message, LEN_ERROR, "Failed to get user information!\n");
        return -3;
    }
    ++ ptr;
    tmpcnt = 0;
    wrptr = infoString;
    while (tmpcnt < 279 && *ptr != '\\' && *ptr != '\0')
    {
        *wrptr ++ = *ptr ++;
        ++ tmpcnt;
    }
    *wrptr = '\0';
    return 0;
}
    
int getIndex(const char* retstr)
{
    const char* rdptr;
    const char* tpptr = UID;
    char* wtptr = userIndex;
    int state = 0;
    int cnt;
    if (retstr == NULL) return -1;
    for (rdptr = retstr; *rdptr != '\0'; ++rdptr)
    {
        switch (state){
            case 0:
                if (*rdptr == 'u') {
                    state = 1;
                    tpptr = UID+1;
                }
                break;
            case 1:
                while(*rdptr != '\0')
                {
                    if (*rdptr == 'u') tpptr = UID;
                    else if (*rdptr != *tpptr) {
                        state = 0;
                        break;
                    }
                    ++rdptr;
                    ++tpptr;
                    if (*tpptr == 'x') {
                        state = 2;
                        break;
                    }
                }
                break;
            case 2:
                if (*rdptr == '=') state = 3;
                break;
            case 3:
                cnt = 0;
                while (cnt <= 199 && (*rdptr != '\0') && (*rdptr != '\n')) {
                    *wtptr++ = *rdptr++;
                    ++cnt;
                }
                *wtptr = '\0';
                state = 4;
                break;
            default:
                ;
        }
    }
    if (state == 4) return 0;
    return -1;
}

int readQuery(const char* input)
{
    const char *ptr;
    char *ptr_w = queryString;
    int tmpcnt = 0;
    ptr = strstr(input, "wlanuserip");
    if (ptr == NULL) 
        return -1;
    while ( tmpcnt < 297 && (*ptr != '\r') ) 
    {
        switch (*ptr)
        {
            case '=' :
                *ptr_w ++ = '%';
                *ptr_w ++ = '3';
                *ptr_w ++ = 'D';
                break;
            case '&':
                *ptr_w ++ = '%';
                *ptr_w ++ = '2';
                *ptr_w ++ = '6';
                break;
            default:
                *ptr_w ++ = *ptr;
        }
        ++ tmpcnt;
        ++ ptr;
    }
    *ptr_w = '\0';
    return 0;
}

int readJid(const char* input)
{
    const char *ptr;
    char *wrptr = jsessionid;
    int i;
    ptr = strstr(input, "JSESSIONID=");
    if (ptr == NULL)
        return -1;
    ptr += 11;
    for (i = 0; i < 32 && *ptr != '\0'; ++i)
        *wrptr ++ = *ptr ++;
    *wrptr = '\0';
    if (i < 32) 
        return -2;
    return 0;
}

int read_info_1(const char* input, char* output, int max_out)
{
    const char* rdptr = input;
    const char* spe_rdptr = input;
    const char* tmprdptr;
    char* wrptr;
    int tmpcnt = 0;
    int totcnt = 0;
    size_t tmp1, tmp2;
    char *tmp_conv1, *tmp_conv2;
    iconv_t convertor;
    convertor = iconv_open("utf8", "gb18030");
    totcnt += snprintf(output, max_out-totcnt, "<table border=\"0\" cellspacing=\"10\" frame=void>");
    while ( (rdptr = strstr(rdptr, "\"contextTitle")) != NULL)
    {
        rdptr += 13;
        if (*rdptr == '2') 
            continue;
        // Read ContextTitle
        rdptr = strstr(rdptr, ">");
        if (rdptr == NULL) break;
        ++rdptr;
        wrptr = info_ret_title;
        while (*rdptr != '<' && *rdptr != '\0' && tmpcnt < 511)
        {
            *wrptr ++ = *rdptr ++;
            tmpcnt ++;
        }
        *wrptr = '\0';
        trim(info_ret_title);
        tmpcnt = 0;
        // Change to UTF-8 Encoding
        tmp_conv1 = info_ret_title;
        tmp_conv2 = info_ret_utf8_title;
        memset(tmp_conv2, 0, 512);
        tmp1 = strlen(info_ret_title);
        tmp2 = 512;
        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
        totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td width=\"56\">%s</td>", info_ret_utf8_title);
        if (totcnt >= max_out-1) 
            break;
        // Read ContextData
        if (strcmp(info_ret_utf8_title, XQINFO) == 0)
        {
            spe_rdptr = strstr(spe_rdptr, "periodAndTimeOrFlowDetail");
            if (spe_rdptr == NULL)
            {
                rdptr = tmprdptr;
                info_ret_field[0] = '\0';
                continue;
            }
            spe_rdptr = strstr(spe_rdptr, "value=\"");
            spe_rdptr += 7;
            wrptr = info_ret_field;
            while (*spe_rdptr != '"' && *spe_rdptr != '\0' && tmpcnt < 511)
            {
                *wrptr ++ = *spe_rdptr ++;
                tmpcnt ++;
            }
            *wrptr = '\0';
            trim_nonaggresive(info_ret_field);
            tmpcnt = 0;
	    }
	    else
	    {
	        tmprdptr = rdptr;
	        rdptr = strstr(tmprdptr, "contextDate");
	        if (rdptr == NULL) 
	        {
	            rdptr = tmprdptr;
	            info_ret_field[0] = '\0';
	            continue;
	        }
	        tmprdptr = rdptr;
	        rdptr = strstr(tmprdptr, ">");
	        if (rdptr == NULL) 
	        {
	            rdptr = tmprdptr;
	            info_ret_field[0] = '\0';
	            continue;
	        }
	        ++rdptr;
	        wrptr = info_ret_field;
	        while (*rdptr != '<' && *rdptr != '\0' && tmpcnt < 511)
	        {
	            *wrptr ++ = *rdptr ++;
	            tmpcnt ++;
	        }
	        *wrptr = '\0';
	        trim_nonaggresive(info_ret_field);
	        tmpcnt = 0;
        }
        tmp_conv1 = info_ret_field;
        tmp_conv2 = info_ret_utf8_field;
        memset(tmp_conv2, 0, 512);
        tmp1 = strlen(info_ret_field);
        tmp2 = 512;
        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
        totcnt += snprintf(output+totcnt, max_out-totcnt, "<td valign=\"middle\">%s\n</td></tr>", info_ret_utf8_field);
        if (totcnt >= max_out-1) break;
    }
    if (totcnt < max_out - 8)
        totcnt += snprintf(output+totcnt, max_out-totcnt, "</table>");
    iconv_close(convertor);
    return totcnt;
}

int read_info_3(const char* input, char* output, int max_out)
{
    const char* rdptr = input;
    char* wrptr = output;
    int totcnt = 0;
    while ( (rdptr = strstr(rdptr, "inputId")) != NULL)
    {
        if ( (rdptr = strstr(rdptr, "userIp")) == NULL)
            return totcnt;
        if (totcnt >= max_out-1) return totcnt;
        totcnt += snprintf(output+totcnt, max_out-totcnt, "IP: ");
        wrptr = output+totcnt;
        if (totcnt >= max_out-1) return totcnt;
        if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            return totcnt;
        rdptr += 7;
        while( *rdptr != '"' && *rdptr != '\0')
        {
            *wrptr ++ = *rdptr ++;
            ++ totcnt;
            if (totcnt == max_out-2)
            {
                *wrptr = '\0';
                return totcnt;
            }
        }
        *wrptr = '\n';
        ++ totcnt;
        if ( (rdptr = strstr(rdptr, "usermac")) == NULL)
            return totcnt;
        totcnt += snprintf(output+totcnt, max_out-totcnt, "MAC: ");
        wrptr = output+totcnt;
        if (totcnt >= max_out-1) return totcnt;
        if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            return totcnt;
        rdptr += 7;
        while( *rdptr != '"' && *rdptr != '\0')
        {
            *wrptr ++ = *rdptr ++;
            ++ totcnt;
            if (totcnt == max_out-2)
            {
                *wrptr = '\0';
                return totcnt+1;
            }
        }
        *wrptr = '\n';
        ++ totcnt;
        if ( (rdptr = strstr(rdptr, "createTimeStr")) == NULL)
            return totcnt;
        totcnt += snprintf(output+totcnt, max_out-totcnt, "Logged in at: ");
        wrptr = output+totcnt;
        if (totcnt >= max_out-1) return totcnt;
        if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            return totcnt;
        rdptr += 7;
        while( *rdptr != '"' && *rdptr != '\0')
        {
            *wrptr ++ = *rdptr ++;
            ++ totcnt;
            if (totcnt == max_out-3)
            {
                *wrptr = '\0';
                return totcnt+1;
            }
        }
        *wrptr ++ = '\n';
        *wrptr = '\n';
        totcnt += 2;
    }
    *wrptr = '\0';
    return totcnt;
}

int get_ret_code(const char* input)
{
    const char* rdptr;
    if ( (rdptr = strstr(input, "HTTP/1.")) == NULL)
        return -257;
    rdptr += 7;
    if (*rdptr == '\0') return -257;
    else return (*rdptr - 48);
}

static seg_info* insert(seg_info* at, float n_start, float n_end, float n_rate, float n_drate)
{
    seg_info* n_seg = (seg_info*)malloc(sizeof(seg_info));
    if (n_seg == NULL) return NULL;
    n_seg->f_start = n_start;
    n_seg->f_end = n_end;
    n_seg->f_rate = n_rate;
    n_seg->f_drate = n_drate;
    n_seg->next = NULL;
    if (at != NULL)
        at->next = n_seg;
    return n_seg;
}

static void free_list(seg_info* head)
{
    seg_info* ptr = head;
    seg_info* tmp = ptr;
    while (ptr != NULL)
    {
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }
}

int read_info_2(const char* input, char* output, int max_out)
{
    const char* rdptr;
    char* wrptr;
    double flowinMB;
    double tmp_start, tmp_end, tmp_rate, tmp_drate;
    seg_info* head = NULL;
    seg_info* last = NULL;
    seg_info *prev = NULL, *curr = NULL, *nxtt = NULL;
    int i;
    int totcnt = 0;
    int tmpcnt = 0;
    int measure;
    double prevend;
    size_t tmp1, tmp2;
    char *tmp_conv1, *tmp_conv2;
    int remday;
    f_state curr_state;

    iconv_t convertor;

    if (max_out < 0) return -2;
    *output = '\0';
    rdptr = strstr(input, "typeForView");
    if (rdptr == NULL) return -1;
    rdptr = strstr(rdptr, "value=\"");
    if (rdptr == NULL) return -1;
    rdptr += 7;
    convertor = iconv_open("utf8", "gb18030");
    if (*rdptr == '5')
    {
        // Read used data flow
        if ( (rdptr = strstr(input, "periodTrafficCumut")) == NULL)
            if ( (rdptr = strstr(input, "culmulateFlow")) == NULL)
            {
                iconv_close(convertor);
                return -1;
            }
        if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
        {
            iconv_close(convertor);
            return -1;
        }
        rdptr += 7;
        flowinMB = atof(rdptr);
        if (flowinMB <= 0)
        {   
            if ( (rdptr = strstr(input, "culmulateFlow")) == NULL)
            {
                iconv_close(convertor);
                return -1;
            }
            if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            {
                iconv_close(convertor);
                return -1;
            }
            rdptr += 7;
            flowinMB = atof(rdptr);
        }
        if ( (rdptr = strstr(input, "measurement")) == NULL)
        {
            iconv_close(convertor);
            return -1;
        }
        if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
        {
            iconv_close(convertor);
            return -1;
        }
        rdptr += 7;
        measure = (int)((*rdptr) - 48);
        // Read flow table
        snprintf(seg_start, 17, "startb startitem");
        snprintf(seg_end, 8, "enditem");
        snprintf(seg_rate, 9, "rateitem");
        snprintf(seg_drate, 10, "drateitem");
        for (i = 1;; ++i)
        {
            snprintf(seg_start+16, 2, "%d", i);
            snprintf(seg_end+7, 2, "%d", i);
            snprintf(seg_rate+8, 2, "%d", i);
            snprintf(seg_drate+9, 2, "%d", i);
            if ( (rdptr = strstr(input, seg_start)) == NULL) break;
            if ( (rdptr = strstr(rdptr, "value=\"")) == NULL) 
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            rdptr += 7;
            tmp_start = atof(rdptr);
            if ( (rdptr = strstr(rdptr, seg_end)) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            rdptr += 7;
            tmp_end = atof(rdptr);
            if ( (rdptr = strstr(rdptr, seg_rate)) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            rdptr += 7;
            tmp_rate = atof(rdptr);
            if ( (rdptr = strstr(rdptr, seg_drate)) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            if ( (rdptr = strstr(rdptr, "value=\"")) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            rdptr += 7;
            tmp_drate = atof(rdptr);
            if ( (last = insert(last, tmp_start, tmp_end, tmp_rate, tmp_drate)) == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
            if (i == 1) head = last;
        }
        if (head == NULL) 
        {
            iconv_close(convertor);
            return -1;
        }
        // Calculate current segment
        curr = head;
        while ( (flowinMB/1024) > curr->f_end)
        {
            prev = curr;
            curr = curr->next;
            if (curr == NULL)
            {
                free_list(head);
                iconv_close(convertor);
                return -1;
            }
        }
        nxtt = curr->next;
        // Write data usage
        totcnt += snprintf(output, max_out-totcnt, "<table border=\"0\" cellspacing=\"10\" frame=void>");
        if (measure == 3)
            totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td>Data Usage:</td><td valign=\"middle\">%.2lfG</td></tr>", flowinMB/1024);
        else
            totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td>Data Usage:</td><td valign=\"middle\">%.2lfM</td></tr>", flowinMB);
        if (nxtt != NULL)
        {
            if (nxtt->f_rate != 0 || nxtt->f_drate != 0)
            {
                if ( (rdptr = strstr(input, "<em class=\"nexttip\">")) != NULL)
                {
                    rdptr += 20;
                    wrptr = info_ret_field;
                    while (*rdptr != '\0' && tmpcnt < 511)
                    {
                        if (*rdptr == '<')
                        {
                            snprintf(info_ret_title, 6, "%s", rdptr);
                            if (strcmp(info_ret_title, "</em>") == 0) break;
                        }
                        *wrptr ++ = *rdptr ++;
                        ++ tmpcnt;
                    }
                    *wrptr = '\0';
                    tmpcnt = 0;
                    trim(info_ret_field);
                    tmp1 = strlen(info_ret_field);
                    tmp2 = 512;
                    tmp_conv1 = info_ret_field;
                    tmp_conv2 = info_ret_utf8_field;
                    memset(tmp_conv2, 0, 512);
                    iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                    totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td width = \"70\">%s</td><td>%.0lfG</td></tr>", info_ret_utf8_field, nxtt->f_start);
                }
            }
            else
            {
                if ( (rdptr = strstr(input, "<em class=\"nexttip1\">")) != NULL)
                {
                    rdptr += 20;
                    wrptr = info_ret_field;
                    while (*rdptr != '\0' && tmpcnt < 511)
                    {
                        if (*rdptr == '<')
                        {
                            snprintf(info_ret_title, 6, "%s", rdptr);
                            if (strcmp(info_ret_title, "</em>") == 0) break;
                        }
                        *wrptr ++ = *rdptr ++;
                        ++ tmpcnt;
                    }
                    tmpcnt = 0;
                    trim(info_ret_field);
                    tmp1 = strlen(info_ret_field);
                    tmp2 = 512;
                    tmp_conv1 = info_ret_field;
                    tmp_conv2 = info_ret_utf8_field;
                    memset(tmp_conv2, 0, 512);
                    iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                    totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td width = \"70\">%s</td><td>%.0lfG</td></tr>", info_ret_utf8_field, nxtt->f_start);
                }
            }
        }
        // Search for other package
        if (curr->f_rate != 0 || curr->f_drate != 0) curr_state = charge;
        else if (curr->f_end >= max) curr_state = free3;
        else
        {
            if (prev == NULL) prevend = 0;
            else prevend = prev->f_end;
            if ((flowinMB/1024-prevend)/(curr->f_end-prevend) >= 0.8) curr_state = free2;
            else curr_state = free1;
        }
        if ( (rdptr = strstr(input, "<tr id=\"packageOther\">")) != NULL)
        { 
            curr_state = other;
            rdptr += 22;
            wrptr = info_ret_field;
            while (*rdptr != '\0' && tmpcnt < 511)
            {
                if (*rdptr == '<')
                {
                    snprintf(info_ret_title, 6, "%s", rdptr);
                    if (strcmp(info_ret_title, "</tr>") == 0) break;
                }
                *wrptr ++ = *rdptr ++;
                ++ tmpcnt;
            }
            tmpcnt = 0;
            trim_only_space_comm(info_ret_field);
            tmp1 = strlen(info_ret_field);
            tmp2 = 512;
            tmp_conv1 = info_ret_field;
            tmp_conv2 = info_ret_utf8_field;
            memset(tmp_conv2, 0, 512);
            iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
            totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr>%s</tr>", info_ret_utf8_field);
        }
        // Tip
        if ( (rdptr = strstr(input, "<tr id=\"normalid_tip\">")) != NULL)
        {
            rdptr += 22;
            if ( (rdptr = strstr(rdptr, "<td class=\"leftitle")) != NULL) 
                if ( (rdptr = strstr(rdptr, ">")) != NULL)
                {
                    // tips
                    ++ rdptr;
                    wrptr = info_ret_title;
                    tmpcnt = 0;
                    while (*rdptr != '\0' && tmpcnt < 511)
                    {
                        if (*rdptr == '<')
                        {
                            snprintf(info_ret_field, 6, "%s", rdptr);
                            if (strcmp(info_ret_field, "</td>") == 0) break;
                        }
                        *wrptr ++ = *rdptr ++;
                        ++ tmpcnt;
                    }
                    *wrptr = '\0';
                    tmpcnt = 0;
                    trim(info_ret_title);
                    tmp_conv1 = info_ret_title;
                    tmp_conv2 = info_ret_utf8_title;
                    memset(tmp_conv2, 0, 512);
                    tmp1 = strlen(info_ret_title);
                    tmp2 = 512;
                    iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                    totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td>%s</td>", info_ret_utf8_title);
                    // qin
                    if ( (rdptr = strstr(rdptr, "<td class=\"alignbo\">")) != NULL)
                    {
                        rdptr += 20;
                        wrptr = info_ret_title;
                        tmpcnt = 0;
                        while (*rdptr != '\0' && tmpcnt < 511)
                        {
                            if (*rdptr == '<')
                            {
                                snprintf(info_ret_field, 6, "%s", rdptr);
                                if (strcmp(info_ret_field, "<span") == 0) break;
                            }
                            *wrptr ++ = *rdptr ++;
                            ++ tmpcnt;
                        }
                        *wrptr = '\0';
                        tmpcnt = 0;
                        trim(info_ret_title);
                        tmp_conv1 = info_ret_title;
                        tmp_conv2 = info_ret_utf8_title;
                        memset(tmp_conv2, 0, 512);
                        tmp1 = strlen(info_ret_title);
                        tmp2 = 512;
                        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                        totcnt += snprintf(output+totcnt, max_out-totcnt, "<td>%s", info_ret_utf8_title);
                        // span
                        switch (curr_state)
                        {
                            case charge:
                                if ( (rdptr = strstr(rdptr, "chargespan")) != NULL)
                                    if ( (rdptr = strstr(rdptr, ">")) != NULL)
                                    {
                                        ++ rdptr;
                                        wrptr = info_ret_title;
                                        tmpcnt = 0;
                                        while (*rdptr != '\0' && tmpcnt < 511)
                                        {
                                            if (*rdptr == '<')
                                            {
                                                snprintf(info_ret_field, 8, "%s", rdptr);
                                                if (strcmp(info_ret_field, "</span>") == 0) break;
                                            }
                                            *wrptr ++ = *rdptr ++;
                                            ++ tmpcnt;
                                        }
                                        *wrptr = '\0';
                                        tmpcnt = 0;
                                        trim(info_ret_title);
                                        tmp_conv1 = info_ret_title;
                                        tmp_conv2 = info_ret_utf8_title;
                                        memset(tmp_conv2, 0, 512);
                                        tmp1 = strlen(info_ret_title);
                                        tmp2 = 512;
                                        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                                        totcnt += snprintf(output+totcnt, max_out-totcnt, "%s</td>", info_ret_utf8_title);
                                    }
                                break;
                            case free1:
                                if ( (rdptr = strstr(rdptr, "freespan1")) != NULL)
                                    if ( (rdptr = strstr(rdptr, ">")) != NULL)
                                    {
                                        ++ rdptr;
                                        wrptr = info_ret_title;
                                        tmpcnt = 0;
                                        while (*rdptr != '\0' && tmpcnt < 511)
                                        {
                                            if (*rdptr == '<')
                                            {
                                                snprintf(info_ret_field, 8, "%s", rdptr);
                                                if (strcmp(info_ret_field, "</span>") == 0) break;
                                            }
                                            *wrptr ++ = *rdptr ++;
                                            ++ tmpcnt;
                                        }
                                        *wrptr = '\0';
                                        tmpcnt = 0;
                                        trim(info_ret_title);
                                        tmp_conv1 = info_ret_title;
                                        tmp_conv2 = info_ret_utf8_title;
                                        memset(tmp_conv2, 0, 512);
                                        tmp1 = strlen(info_ret_title);
                                        tmp2 = 512;
                                        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                                        totcnt += snprintf(output+totcnt, max_out-totcnt, "%s</td>", info_ret_utf8_title);
                                    }
                                break;
                            case free2:
                                if ( (rdptr = strstr(rdptr, "freespan2")) != NULL)
                                    if ( (rdptr = strstr(rdptr, ">")) != NULL)
                                    {
                                        ++ rdptr;
                                        wrptr = info_ret_title;
                                        tmpcnt = 0;
                                        while (*rdptr != '\0' && tmpcnt < 511)
                                        {
                                            if (*rdptr == '<')
                                            {
                                                snprintf(info_ret_field, 8, "%s", rdptr);
                                                if (strcmp(info_ret_field, "</span>") == 0) break;
                                            }
                                            *wrptr ++ = *rdptr ++;
                                            ++ tmpcnt;
                                        }
                                        *wrptr = '\0';
                                        tmpcnt = 0;
                                        trim(info_ret_title);
                                        tmp_conv1 = info_ret_title;
                                        tmp_conv2 = info_ret_utf8_title;
                                        memset(tmp_conv2, 0, 512);
                                        tmp1 = strlen(info_ret_title);
                                        tmp2 = 512;
                                        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                                        remday = 0;
                                        if ( (rdptr = strstr(input, "leftPeriodRange")) != NULL && (rdptr = strstr(rdptr, "value=\"")) != NULL) 
                                        {
                                            rdptr += 7;
                                            while (*rdptr < 58 && *rdptr > 47)
                                            {
                                                remday *= 10;
                                                remday += (*(rdptr) - 48);
                                                ++ rdptr; 
                                            }
                                        }
                                        if (remday > 0 && measure == 3 && (curr->f_end*1024-flowinMB)/remday > 1024)
                                            totcnt += snprintf(output+totcnt, max_out-totcnt, "%s%.2lfG</td>", info_ret_utf8_title,\
                                                    (curr->f_end*1024-flowinMB)/(1024*remday));
                                        else 
                                            totcnt += snprintf(output+totcnt, max_out-totcnt, "%s%.2lfM</td>", info_ret_utf8_title,\
                                                    (curr->f_end*1024-flowinMB)/remday);
                                    }
                                break;
                            case free3:
                                if ( (rdptr = strstr(rdptr, "freespan3")) != NULL)
                                    if ( (rdptr = strstr(rdptr, ">")) != NULL)
                                    {
                                        ++ rdptr;
                                        wrptr = info_ret_title;
                                        tmpcnt = 0;
                                        while (*rdptr != '\0' && tmpcnt < 511)
                                        {
                                            if (*rdptr == '<')
                                            {
                                                snprintf(info_ret_field, 8, "%s", rdptr);
                                                if (strcmp(info_ret_field, "</span>") == 0) break;
                                            }
                                            *wrptr ++ = *rdptr ++;
                                            ++ tmpcnt;
                                        }
                                        *wrptr = '\0';
                                        tmpcnt = 0;
                                        trim(info_ret_title);
                                        tmp_conv1 = info_ret_title;
                                        tmp_conv2 = info_ret_utf8_title;
                                        memset(tmp_conv2, 0, 512);
                                        tmp1 = strlen(info_ret_title);
                                        tmp2 = 512;
                                        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                                        totcnt += snprintf(output+totcnt, max_out-totcnt, "%s</td>", info_ret_utf8_title);
                                    }
                                break;
                            case other:
                                if ( (rdptr = strstr(rdptr, "otherpackagespan")) != NULL)
                                    if ( (rdptr = strstr(rdptr, ">")) != NULL)
                                    {
                                        ++ rdptr;
                                        wrptr = info_ret_title;
                                        tmpcnt = 0;
                                        while (*rdptr != '\0' && tmpcnt < 511)
                                        {
                                            if (*rdptr == '<')
                                            {
                                                snprintf(info_ret_field, 8, "%s", rdptr);
                                                if (strcmp(info_ret_field, "</span>") == 0) break;
                                            }
                                            *wrptr ++ = *rdptr ++;
                                            ++ tmpcnt;
                                        }
                                        *wrptr = '\0';
                                        tmpcnt = 0;
                                        trim(info_ret_title);
                                        tmp_conv1 = info_ret_title;
                                        tmp_conv2 = info_ret_utf8_title;
                                        memset(tmp_conv2, 0, 512);
                                        tmp1 = strlen(info_ret_title);
                                        tmp2 = 512;
                                        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
                                        totcnt += snprintf(output+totcnt, max_out-totcnt, "%s</td>", info_ret_utf8_title);
                                    }
                                break;
                            default:
                                ;
                        }
                    }
                    totcnt += snprintf(output+totcnt, max_out-totcnt, "</tr>");
                }
        }
    }
    else 
    {
        // preserved 
        ;
    }
    iconv_close(convertor);
    return 0;
}
