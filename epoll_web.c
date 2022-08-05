
#include "stdio.h"
#include "wrap.h"
#include "sys/epoll.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "pub.h"
#include "dirent.h"
#include "signal.h"

#define PORT 8080


void send_header(int cfd, int code, char *info, char *filetype, int length)
{
    //发送状态行
    char buf[1024] = "";
    int len =0;
    len = sprintf(buf,"HTTP/1.1 %d %s\r\n",code,info);
    send(cfd, buf, len, 0);
    //发送消息头
    len = sprintf(buf, "Content-Type:%s\r\n", filetype);
    send(cfd, buf, len, 0);
    if(length>0)
    {
        len = sprintf(buf, "Content-Length:%d\r\n", length);
        send(cfd, buf, len, 0);
    }
    //发送空行

    send(cfd, "\r\n", 2, 0);
};

void send_file(int cfd, char *path,  struct epoll_event *ev, int epfd, int flag)
{
    int fd = open(path, O_RDONLY);
    if(fd<0)
    {
        perror("");
        return;
    }
    char buf[1024]  = "";
    int len = 0;
    while(1)
    {
        len = read(fd, buf, sizeof(buf));
        if(len<0)
        {
            perror("");
            break;
        }
        else if(len ==0)
        {
            break;
        }
        else
        {
            send(cfd, buf, len, 0);
        }
    }
    close(fd);
    //关闭CFD,下树
    if(flag == 1)
    {
        close(cfd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, cfd,ev);
    }

};

void read_client_request(int epfd, struct epoll_event *ev)
{
    char buf[1024] = "";
    char tmp[1024] = "";
    int n =Readline(ev->data.fd, buf, sizeof(buf));
    if(n<=0)
    {
        printf("close or error\n");
        epoll_ctl(epfd, EPOLL_CTL_DEL, ev->data.fd, ev);
        close(ev->data.fd);
        return ;
    }
    printf("%s\n", buf);
    
    int ret =0;
    while( (ret = Readline(ev->data.fd, tmp, sizeof(tmp))) >0);
    
    //解析请求行GET /a.txt HTTP/1.1/R/N
    char method[256] = "";
    char content[256] ="";
    char protocol[256] ="";
    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, content, protocol);
    //是否为get请求，对get请求进行处理
    if(strcasecmp(method, "GET") == 0)
    {
        char *strfile = content+1;
        strdecode(strfile,strfile);  //对中文名的文件进行转码
        //得到浏览请求文件，如果没有请求文件，就默认请求  ./   
        if(*strfile == 0)
        {
            strfile = "./";
        }
        //判断文件是否存在，如果存在（普通文件，目录）
        struct stat s;
        if(stat(strfile, &s)<0)
        {
            printf("file not fount\n");
            //先发送 报头（状态行 消息头 空行）
            send_header(ev->data.fd, 404, "Not Found", get_mime_type("*.html"), 0);
            //发送文件error.html
            send_file(ev->data.fd, "error.html", ev, epfd, 1);
        }
        else
        {
            //普通文件
            if(S_ISREG(s.st_mode))
            {
                printf("FILE\n");
                //先发送 报头（状态行 消息头 空行）
                send_header(ev->data.fd, 200, "OK", get_mime_type(strfile),s.st_size);
                //发送文件
                send_file(ev->data.fd, strfile, ev, epfd, 1);
                printf("FILE over\n");
            }
            else if(S_ISDIR(s.st_mode))
            { //目录
                printf("DIR\n");
                //发送一个列表，就是一个网页
                send_header(ev->data.fd, 200, "OK", get_mime_type("*.html"), 0);
                //发送头
                send_file(ev->data.fd,"dir_header.html",ev,epfd,0);
                
                //组包，列表
                struct dirent **mylist = NULL;
                char buf[1024] = "";
                int len =0;
                int n = scandir(strfile, &mylist, NULL, alphasort);
                for(int i=0; i<n; i++)
                {
                    if(mylist[i]->d_type == DT_DIR)
                    {
                        //如果超链接是目录
                        len = sprintf(buf,"<li><a href=%s/ >%s</a></li>",mylist[i]->d_name,mylist[i]->d_name);
                        
                    }
                    else
                    {
                        len = sprintf(buf,"<li><a href=%s >%s</a></li>",mylist[i]->d_name,mylist[i]->d_name);
                    }
                    
                    send(ev->data.fd, buf, len, 0);

                    free(mylist[i]);
                }
                free(mylist);

                //发送尾
                send_file(ev->data.fd, "dir_tail.html", ev, epfd, 1);
            }

        }



    }
    //得到浏览请求文件，如果没有请求文件，就默认请求  ./   
    //判断文件是否存在，如果存在（普通文件，目录）
    //不存在，发送error . html



}

int main(int argc, char const *argv[])
{
    //忽略阻塞
    signal(SIGPIPE, SIG_IGN);
    //切换工作目录
    //获取当前目录工作路径
    char pwd_path[256] = "";
    char *path = getenv("PWD");
    strcpy(pwd_path, path);
    strcat(pwd_path, "/web-http");
    chdir(pwd_path);

    //创建套接字， 绑定
    int lfd = tcp4bind(PORT, NULL);
    //监听
    Listen(lfd, 128);
    //创建树
    int epfd = epoll_create(1);
    //LFD上树
    struct epoll_event ev, evs[1024];
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    

    //循环监听
    while (1)
    {
        int nready = epoll_wait(epfd, evs, 1024, -1);
        if(nready<0)
        {
            perror("");
            break;
        }
        else
        {
            for(int i =0; i<nready; i++)
            {
                if(evs[i].data.fd == lfd && evs[i].events &EPOLLIN)
                {
                    struct sockaddr_in cliaddr;
                    char ip[16] = "";
                    socklen_t len = sizeof(cliaddr);
                    int cfd = Accept(lfd, (struct sockaddr*)&cliaddr, &len);
                    printf("new client ip = %s, port = %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, ip,16), ntohs(cliaddr.sin_port));

                    //设置cfd非阻塞
                    int flag = fcntl(cfd, F_GETFL);
                    flag |= O_NONBLOCK;
                    fcntl (cfd, F_SETFL, flag);
                    //上树
                    ev.data.fd = cfd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
                }
                else if(evs[i].events & EPOLLIN)  //CFD
                {
                    read_client_request(epfd,&evs[i]);
                }
            }

        }
    }
    
    //收尾

    return 0;
}