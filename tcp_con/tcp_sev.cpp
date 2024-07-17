#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <stdexcept> // 用于异常处理
#include <cstring>   // 用于 memset
#include <cstdlib>   // 用于 atoi
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
using namespace std;


/*

进程间tcp通信
设置信号量防止出现僵尸线程

*/


const int BACKLOG = 5;
void ClientHandle(int newsockfd);
void Sighandle(int sig)
{
    if (sig == SIGCHLD)
    {
        cout << "client exited" << endl;
        wait(nullptr);
    }
}


int main(int argc, char *argv[])
{
    int newsockfd;
    pid_t pid;
    if (argc < 3)
    {
        cerr << argv[0] << " <addr> <port>" << endl;
        return 1;
    }

    struct sigaction act; // 设置信号量
    act.sa_handler = Sighandle;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, nullptr);

    int socketfd = socket(AF_INET, SOCK_STREAM, 0); // 创建socket套接字
    if (socketfd < 0)
    {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    struct sockaddr_in addr;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    memset(&addr, 0, sizeof(addr)); // 清零结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2])); // 设置端口号

    if (strcmp(argv[1], "0") == 0)
    {
        addr.sin_addr.s_addr = INADDR_ANY; // 绑定到所有可用接口
    }
    else
    {
        if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0)
        { // 使用用户提供的地址
            cerr << "inet_pton error occurred" << endl;
            close(socketfd);
            return 1;
        }
    }

    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // 地址快速重用
    {
        cerr << "setsockopt failed" << endl;
        close(socketfd);
        return 1;
    }

    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) // 绑定通信结构体
    {
        cerr << "Bind failed" << endl;
        close(socketfd);
        return 1;
    }

    if (listen(socketfd, BACKLOG) == -1) // 设置套接字为监听模式
    {
        cerr << "Listen failed" << endl;
        close(socketfd);
        return 1;
    }

    while (1)
    {
        newsockfd = accept(socketfd, (struct sockaddr *)&client_addr, &addrlen);//接收客户端的请求
        if (newsockfd == -1)
        {
            cerr<<"accept is fail"<<endl;
            close(socketfd);
            return 1;
        }
        cout << "addr:" << inet_ntoa(client_addr.sin_addr) 
        << "   port:" << ntohs(client_addr.sin_port) << endl;//打印客户端的地址和端口号
        if ((pid = fork()) < 0)
        {
            cerr << "create pid failed" << endl;
            return 1;
        }
        else if (pid == 0) // 子进程
        {
            close(socketfd);
            ClientHandle(newsockfd);
            exit(0);
        }
        else // 父进程
        {
            close(newsockfd);
        }
    }

    close(socketfd); // 关闭服务器套接字
    return 0;
}

void ClientHandle(int newsockfd)
{
    while (1)
    {
        char buffer[BUFSIZ] = {};
        memset(buffer, 0, BUFSIZ);
        int ret = read(newsockfd, buffer, BUFSIZ);
        if (ret < 0)
        {
            cerr << "read failed" << endl;
            return;
        }
        else if (ret == 0)
        {
            return;
        }
        else
        {
            cout << buffer;
        }
    }
}
