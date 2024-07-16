#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
using namespace std;

typedef struct sockaddr Addr;
typedef struct sockaddr_in Addr_in;
#define BACKLOG 5
#define MAX_SOCKET_FD 1024

void Argment(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << argv[0] << " <addr> <port>" << endl;
        exit(1);
    }
}

int CreateSocket(char *argv[])
{
    /*创建套接字*/
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        cerr << "create socket is fail" << endl;
        return -1;
    }

    /*允许地址快速重用*/
    int flag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        cerr << "setsockopt is fail" << endl;
        close(fd);
        return -1;
    }

    /*设置通信结构体*/
    Addr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]); // 增加此行设置 IP 地址

    /*绑定通信结构体*/
    if (bind(fd, (Addr *)&addr, sizeof(Addr_in)))
    {
        cerr << "bind is fail" << endl;
        close(fd);
        return -1;
    }

    /*设置套接字为监听模式*/
    if (listen(fd, BACKLOG))
    {
        cerr << "listen is fail" << endl;
        close(fd);
        return -1;
    }

    return fd;
}

int DataHandle(int fd) // 处理客户端的数据
{
    char buf[BUFSIZ] = {};
    Addr_in clientaddr;
    socklen_t clientlen = sizeof(Addr_in);
    if (getpeername(fd, (Addr *)&clientaddr, &clientlen) < 0)
    {
        cerr << "getpeername is fail" << endl;
        return -1;
    }
    int ret = recv(fd, buf, BUFSIZ, 0);
    if (ret < 0)
    {
        cerr << "recv failed, error: " << strerror(errno) << endl;
        return -1;
    }
    if (ret > 0)
        cout << "IP:" << inet_ntoa(clientaddr.sin_addr) << " "
             << "端口号:" << ntohs(clientaddr.sin_port) << " buf:" << buf << endl;
    return ret;
}



int main(int argc, char *argv[])
{
    /*检查参数，小于3个 直接退出进程*/
    Argment(argc, argv);

    Addr_in clientAddr;
    socklen_t clientlen = sizeof(Addr_in);

    int fd, newfd, epfd, nfds;
    struct epoll_event temp, events[MAX_SOCKET_FD] = {};

    /*创建已设置监听模式的套接字*/
    fd = CreateSocket(argv);

    if ((epfd = epoll_create(1)) < 0)
    {
        cerr << "create  epool is fail" << endl;
        return -1;
    }

    temp.events = EPOLLIN;
    temp.data.fd = fd;
    if ((epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &temp)) < 0)
    {
        cerr << "epool_ctl is fail" << endl;
        return -1;
    }

    while (1)
    {
        if ((nfds = epoll_wait(epfd, events, MAX_SOCKET_FD, -1)) < 0)
        {
            cerr << "epool_wait is fail" << endl;
            return -1;
        }
        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == fd) 
            {
                /*接收客户端连接，并生成新的文件描述符*/
                if ((newfd = accept(fd, (Addr *)&clientAddr, &clientlen)) < 0)
                {
                    cerr << "accept is fail, error: " << strerror(errno) << endl;
                    close(events[i].data.fd);
                    return 1;
                }
                cout << "[" << inet_ntoa(clientAddr.sin_addr) << ":"
                     << ntohs(clientAddr.sin_port) << "]"<< " connection"<<endl;
                temp.events = EPOLLIN;
                temp.data.fd = newfd;
                if ((epoll_ctl(epfd, EPOLL_CTL_ADD, newfd, &temp)) < 0)
                {
                    cerr << "epool_ctl is fail" << endl;
                    return -1;
                }
            }
            else
            {
                if ((DataHandle(events[i].data.fd)) <= 0)//客户端退出
                {

                    if ((epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr)) < 0)
                    {
                        cerr << "epool_ctl is fail" << endl;
                        return -1;
                    }
                    if ((getpeername(events[i].data.fd, (Addr *)&clientAddr, &clientlen)) < 0)
                    {
                        cerr << "getpeername is fail, error: " << strerror(errno) << endl;
                        close(fd);
                        return 1;
                    }
                    cout << "[" << inet_ntoa(clientAddr.sin_addr) << ":"
                         << ntohs(clientAddr.sin_port) << "]"<< " exit" << endl;
                    close(events[i].data.fd);          
                }
            }
        }
    }
    close(epfd);
    close(fd);
    return 0;
}
