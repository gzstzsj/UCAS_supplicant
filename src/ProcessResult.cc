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
char info_ret_title[300];
char info_ret_utf8_title[300];
char info_ret_field[300];
char info_ret_utf8_field[300];

static const char* RES = "result\"";
static const char* MES = "message\"";
static const char* UID = "userIndex\"";
static const char* XQINFO = "套餐详情";

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
        while (*rdptr != '<' && *rdptr != '\0' && tmpcnt < 299)
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
        memset(tmp_conv2, 0, 300);
        tmp1 = strlen(info_ret_title);
        tmp2 = 300;
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
            while (*spe_rdptr != '"' && *spe_rdptr != '\0' && tmpcnt < 299)
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
	        while (*rdptr != '<' && *rdptr != '\0' && tmpcnt < 299)
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
        memset(tmp_conv2, 0, 300);
        tmp1 = strlen(info_ret_field);
        tmp2 = 300;
        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
        totcnt += snprintf(output+totcnt, max_out-totcnt, "<td valign=\"middle\">%s\n</td></tr>", info_ret_utf8_field);
        if (totcnt >= max_out-1) break;
    }
    if (totcnt < max_out - 8)
        totcnt += snprintf(output+totcnt, max_out-totcnt, "</table>");
    iconv_close(convertor);
    return totcnt;
}

int read_info_2(const char* input, char* output, int max_out)
{
    const char* rdptr = input;
    char* wrptr;
    char rem_flow[10];
    char tot_flow[10];
    int tmpcnt = 0;
    int totcnt = 0;
    size_t tmp1, tmp2;
    char *tmp_conv1, *tmp_conv2;
    iconv_t convertor;
    convertor = iconv_open("utf8", "gb18030");
    totcnt += snprintf(output, max_out-totcnt, "<table border=\"0\" cellspacing=\"10\" frame=void>");
    // Read flow
    rdptr = strstr(rdptr, "portal_freeFlowDesc");
    if (rdptr != NULL) 
    {
        if ( (rdptr = strstr(rdptr, "value=\"")) != NULL)
        {
            rdptr += 7;
            wrptr = tot_flow;
            while (*rdptr != '"' && *rdptr != '\0' && tmpcnt < 9)
            {
                *wrptr ++ = *rdptr ++;
                ++ tmpcnt;
            }
            *wrptr = '\0';
            tmpcnt = 0;
            if ( (rdptr = strstr(rdptr, "portal_usedFreeDesc")) != NULL)
            {
                if ( (rdptr = strstr(rdptr, "value=\"")) != NULL)
                rdptr += 7;
                wrptr = rem_flow;
                while (*rdptr != '"' && *rdptr != '\0' && tmpcnt < 9)
                {
                    *wrptr ++ = *rdptr ++;
                    ++ tmpcnt;
                }
                *wrptr = '\0';
                tmpcnt = 0;
                totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td width=\"56\">Data Usage:</td><td valign=\"middle\">%s/%s</td></tr>", rem_flow, tot_flow);
            }
        }
    }
    // Read other messages
    if (rdptr == NULL) rdptr = input;
    while ( (rdptr = strstr(rdptr, "<td class=\"leftitle alignbo")) != NULL)
    {
        if ( (rdptr = strstr(rdptr, ">")) == NULL) break;
        ++ rdptr;
        wrptr = info_ret_title;
        while (*rdptr != '\0' && tmpcnt < 299)
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
        // Convert to utf8
        tmp_conv1 = info_ret_title;
        tmp_conv2 = info_ret_utf8_title;
        memset(tmp_conv2, 0, 300);
        tmp1 = strlen(info_ret_title);
        tmp2 = 300;
        iconv(convertor, &tmp_conv1, &tmp1, &tmp_conv2, &tmp2);
        totcnt += snprintf(output+totcnt, max_out-totcnt, "<tr><td width=\"56\">%s</td>", info_ret_utf8_title);
        if (totcnt >= max_out-1) break;
        // Read field
        if ( (rdptr = strstr(rdptr, "<td class=\"alignbo")) == NULL || (rdptr = strstr(rdptr, ">")) == NULL)
        {
            totcnt += snprintf(output+totcnt, max_out-totcnt, "</tr>");
            if (totcnt >= max_out-1) break;
            continue;
        }
        ++ rdptr;
        wrptr = info_ret_field;
        while (*rdptr != '\0' && tmpcnt < 299)
        {
            if (*rdptr == '<')
            {
                snprintf(info_ret_title, 6, "%s", rdptr);
                if (strcmp(info_ret_title, "</td>") == 0) break;
            }
            *wrptr ++ = *rdptr ++;
            ++ tmpcnt;
        }
        *wrptr = '\0';
        tmpcnt = 0;
        trim(info_ret_field);
        tmp_conv1 = info_ret_field;
        tmp_conv2 = info_ret_utf8_field;
        memset(tmp_conv2, 0, 300);
        tmp1 = strlen(info_ret_field);
        tmp2 = 300;
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
