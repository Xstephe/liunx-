#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << argv[0] << " <addr> <port>" << endl;
        return 1;
    }
    char buf[BUFSIZ] = {};
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) // 创建udp套接字
    {
        cerr << "create socket failed" << endl;
        return 1;
    }

    struct sockaddr_in addr;        // 设置通信结构体
    memset(&addr, 0, sizeof(addr)); // 清零结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));                 // 设置端口号
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) // 用户提供的地址
    {
        cerr << "inet_pton error occurred" << endl;
        close(fd);
        return 1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cerr << "setsockopt failed" << endl;
        close(fd);
        return 1;
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) // 绑定结构体
    {
        cerr << "bind failed" << endl;
        close(fd);
        return 1;
    }
    while (1)
    {
        memset(buf, 0, BUFSIZ);
        recvfrom(fd, buf, BUFSIZ, 0, nullptr, nullptr);
        cout << buf << endl;
    }

    close(fd);
    return 0;
}
