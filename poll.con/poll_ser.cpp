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
#include <poll.h>
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
        exit(1); // 修改为退出进程
    }
}

int CreateSocket(char *argv[])
{
    /*创建套接字*/
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        cerr << "create socket is fail" << endl;
        return -1; // 修改返回值为-1
    }

    /*允许地址快速重用*/
    int flag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        cerr << "setsockopt is fail" << endl;
        close(fd);
        return -1; // 修改返回值为-1
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
        return -1; // 修改返回值为-1
    }

    /*设置套接字为监听模式*/
    if (listen(fd, BACKLOG))
    {
        cerr << "listen is fail" << endl;
        close(fd);
        return -1; // 修改返回值为-1
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
        return -1; // 修改返回值为-1
    }
    int ret = recv(fd, buf, BUFSIZ, 0);
    if (ret < 0)
    {
        cerr << "recv failed, error: " << strerror(errno) << endl; // 增加错误信息打印
        return -1;                                                 // 修改返回值为-1
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

    int fd, newfd;
    struct pollfd fds[MAX_SOCKET_FD] = {};
    nfds_t nfds = 1;

    /*创建已设置监听模式的套接字*/
    fd = CreateSocket(argv);

    fds[0].fd = fd;
    fds[0].events = POLLIN;

    while (1)
    {
        if ((poll(fds, nfds, -1)) < 0)
        {
            cerr << "poll is fail, error: " << strerror(errno) << endl; 
            close(fds[0].fd);
            return 1;
        }
        for (int i = 0; i < nfds; ++i)
        {
            if (fds[i].fd == fd && fds[i].revents & POLLIN) 
            {
                /*接收客户端连接，并生成新的文件描述符*/
                if ((newfd = accept(fd, (Addr *)&clientAddr, &clientlen)) < 0)
                {
                    cerr << "accept is fail, error: " << strerror(errno) << endl;
                    close(fds[0].fd);
                    return 1;
                }

                fds[nfds].fd = newfd;
                fds[nfds++].events = POLLIN;
                cout << "[" << inet_ntoa(clientAddr.sin_addr) << ":"
                     << ntohs(clientAddr.sin_port) << "]"
                     << "nfds:" << nfds << endl;
            }
            if (i > 0 && fds[i].revents & POLLIN) 
            {
                if ((DataHandle(fds[i].fd)) <= 0)
                {
                    if ((getpeername(fds[i].fd, (Addr *)&clientAddr, &clientlen)) < 0)
                    {
                        cerr << "getpeername is fail, error: " << strerror(errno) << endl; 
                        close(fd);
                        return 1;
                    }
                    cout << "[" << inet_ntoa(clientAddr.sin_addr) << ":"
                         << ntohs(clientAddr.sin_port) << "]"
                         << "fd:" << nfds << " exit" << endl;
                    close(fds[i].fd);
                    for (int j = i; j < nfds - 1; j++)
                    {
                        fds[j] = fds[j + 1];
                    }
                    nfds--;
                    i--;
                }
            }
        }
    }
    close(fd);
    return 0;
}
