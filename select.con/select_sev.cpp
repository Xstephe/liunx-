#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

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
        return;
    }
}

int CreateSocket(char *argv[])
{
    /*创建套接字*/
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        cerr << "create socket is fail" << endl;
        return 1;
    }

    /*允许地址快速重用*/
    int flag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        cerr << "setsockopt is fail" << endl;
        close(fd);
        return 1;
    }

    /*设置通信结构体*/
    Addr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    /*绑定通信结构体*/
    if (bind(fd, (Addr *)&addr, sizeof(Addr_in)))
    {
        cerr << "bind is fail" << endl;
        close(fd);
        return 1;
    }

    /*设置套接字为监听模式*/
    if (listen(fd, BACKLOG))
    {
        cerr << "listen is fail" << endl;
        close(fd);
        return 1;
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
        return 1;
    }
    int ret = recv(fd, buf, BUFSIZ, 0);
    if (ret < 0)
    {
        cerr << "recv failed" << endl;
        return 1;
    }
    if (ret > 0)
        cout << "IP:" << inet_ntoa(clientaddr.sin_addr) << " " << "端口号:" << ntohs(clientaddr.sin_port) << " buf:" << buf << endl;
    return ret;
}

int main(int argc, char *argv[])
{
    /*检查参数，小于3个 直接退出进程*/
    Argment(argc, argv);

    Addr_in clientAddr;
    socklen_t clientlen = sizeof(Addr_in);

    int fd, newfd, ret, i;
    fd_set set, temset;

    /*创建已设置监听模式的套接字*/
    fd = CreateSocket(argv);
    FD_ZERO(&set);
    FD_ZERO(&temset);
    FD_SET(fd, &set);

   while (1)
    {
        temset = set;
        if ((ret = select(MAX_SOCKET_FD, &temset, nullptr, nullptr, nullptr)) < 0)
        {
            cerr << "select is fail" << endl;
            break;
        }

        /* 检查监听套接字 fd，处理新连接 */
        if (FD_ISSET(fd, &temset))
        {
            /* 接收客户端连接，并生成新的文件描述符 */
            if ((newfd = accept(fd, (Addr *)&clientAddr, &clientlen)) < 0)
            {
                cerr << "accept is fail" << endl;
                close(fd);
                return 1;
            }
            cout << "IP:" << inet_ntoa(clientAddr.sin_addr) << "已建立连接\t"
                 << "端口号:" << ntohs(clientAddr.sin_port) << endl;
            FD_SET(newfd, &set);
        }

        /* 遍历文件描述符，处理客户端数据 */
        for (int i = fd + 1; i < MAX_SOCKET_FD; ++i)
        {
            if (FD_ISSET(i, &temset))
            {
                if (DataHandle(i) <= 0) // 客户端退出，把文件描述符剔除
                {
                    if (getpeername(i, (Addr *)&clientAddr, &clientlen) < 0)
                    {
                        cerr << "getpeername is fail" << endl;
                        return 1;
                    }
                    cout << "IP:" << inet_ntoa(clientAddr.sin_addr) << " "
                         << "端口号:" << ntohs(clientAddr.sin_port) <<" 已退出连接\t" << endl;
                    FD_CLR(i, &set);
                }
            }
        }
    }

    close(fd);
    close(newfd);
    return 0;
}
