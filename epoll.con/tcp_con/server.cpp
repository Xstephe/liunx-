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
#include <pthread.h>
using namespace std;

/*

线程间tcp通信


*/


const int BACKLOG = 5;
void *ClientHandle(void *arg);

int main(int argc, char *argv[])
{
    int newsockfd;
    if (argc < 3)
    {
        cerr << argv[0] << " <addr> <port>" << endl;
        return 1;
    }
    pthread_t tid;

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

    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) < 0)
    { // 使用用户提供的地址
        cerr << "inet_pton error occurred" << endl;
        close(socketfd);
        return 1;
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
        newsockfd = accept(socketfd, (struct sockaddr *)&client_addr, &addrlen);
        if (newsockfd == -1)
        {

            close(socketfd);
            return 1;
        }
        cout << "addr:" << inet_ntoa(client_addr.sin_addr) << "   port:" << ntohs(client_addr.sin_port) << endl;
        pthread_create(&tid, nullptr, ClientHandle, &newsockfd);
        pthread_detach(tid);//设置线程为分离属性
    }

    close(socketfd); // 关闭服务器套接字
    return 0;
}

void *ClientHandle(void *arg)
{
    char buffer[BUFSIZ] = {};
    int newsockfd = *(int *)arg;
    while (1)
    {
        memset(buffer, 0, BUFSIZ);
        int ret = read(newsockfd, buffer, BUFSIZ);
        if (ret < 0)
        {
            cerr << "read failed" << endl;
            return nullptr;
        }
        else if (ret == 0)
        {

            break;
        }
        else
        {
            cout << buffer;
        }
    }
    cout << "client is exit" << endl;
    return nullptr;
}
