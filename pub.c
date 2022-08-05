#include "pub.h"
//ͨ���ļ����ֻ���ļ�����
char *get_mime_type(char *name)
{
    char* dot;
    //返回文件类型

    dot = strrchr(name, '.');	//����������ҡ�.���ַ�;�粻���ڷ���NULL
    /*
     *charset=iso-8859-1	��ŷ�ı��룬˵����վ���õı�����Ӣ�ģ�
     *charset=gb2312		˵����վ���õı����Ǽ������ģ�
     *charset=utf-8			��������ͨ�õ����Ա��룻
     *						�����õ����ġ����ġ����ĵ��������������Ա�����
     *charset=euc-kr		˵����վ���õı����Ǻ��ģ�
     *charset=big5			˵����վ���õı����Ƿ������ģ�
     *
     *���������ݴ��ݽ������ļ�����ʹ�ú�׺�ж��Ǻ����ļ�����
     *����Ӧ���ļ����Ͱ���http����Ĺؼ��ַ��ͻ�ȥ
     */
    if (dot == (char*)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
//���һ�����ݣ�ÿ����\r\n��Ϊ�������
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);//MSG_PEEK �ӻ����������ݣ��������ݲ��ӻ��������
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

//����ĺ����ڶ���ʹ��
/*
 * ����������Ǵ���%20֮��Ķ�������"����"���̡�
 * %20 URL�����еġ� ��(space)
 * %21 '!' %22 '"' %23 '#' %24 '$'
 * %25 '%' %26 '&' %27 ''' %28 '('......
 * ���֪ʶhtml�еġ� ��(space)��&nbsp
 */
 // %E8%8B%A6%E7%93%9C
void strdecode(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from) {

        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) { //�����ж�from�� %20 �����ַ�

            *to = hexit(from[1])*16 + hexit(from[2]);//�ַ���E8�����������16���Ƶ�E8
            from += 2;                      //�ƹ��Ѿ������������ַ�(%21ָ��ָ��1),����ʽ3��++from�����������һ���ַ�
        } else
            *to = *from;
    }
    *to = '\0';
}

//16������ת��Ϊ10����, return 0�������
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

//"����"��������д�������ʱ�򣬽�����ĸ���ּ�/_.-~������ַ�ת����д��
//strencode(encoded_name, sizeof(encoded_name), name);
void strencode(char* to, size_t tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}
