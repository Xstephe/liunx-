#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <stdexcept> // 用于标准异常处理

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << argv[0] << " <addr> <port>" << endl;
        return 1;
    }

    char buffer[BUFSIZ] = {};
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
    if (socketfd < 0)
    {
        throw runtime_error("Socket creation failed");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) < 0)
    {
        cerr << "inet_pton error occurred" << endl;
        close(socketfd);
        return 1;
    }

    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) // 向服务端发起连接请求
    {
        cerr << "Connect failed" << endl;
        close(socketfd);
        return 1;
    }

    while (1)
    {
        cout << "Enter message: ";
        fgets(buffer, BUFSIZ, stdin);

        // 检查输入是否为空
        if (strlen(buffer) == 0)
        {
            cerr << "No input received, exiting." << endl;
            break;
        }

        write(socketfd, buffer, strlen(buffer));
        cout << "Sent: " << buffer;
    }

    close(socketfd);
    return 0;
}
