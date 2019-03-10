#include <conio.h>
#include <iostream.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#define MAX_SIZE 4096

char CmdBuf[MAX_SIZE];
char Command[MAX_SIZE];
char ReplyMsg[MAX_SIZE];
int nReplyCode;
bool bConnected = false;
SOCKET SocketControl;
SOCKET SocketData;

//从 FTP 服务器接收响应
bool RecvReply()
{
    //控制连接接收数据
    int nRecv;
    memset(ReplyMsg, 0, MAX_SIZE);
    nRecv = recv(SocketControl, ReplyMsg, MAX_SIZE, 0);
    if (nRecv == SOCKET_ERROR)
    {
        cout<<"Socket receive error!"<<endl;
        closesocket(SocketControl);
        return false;
    }
    //获取响应信息与响应码
    if (nRecv > 4) {
        char* ReplyCodes = new char[3];
        memset(ReplyCodes, 0, 3);
        memcpy(ReplyCodes, ReplyMsg, 3);
        nReplyCode = atoi(ReplyCodes);
    }
    return true;
}

//从 FTP 服务器发送命令
bool SendCommand()
{
    //控制连接发送数据
    int nSend;
    nSend = send(SocketControl, Command, strlen(Command), 0);
    if (nSend == SOCKET_ERROR) {
        cout<<"Socket send error!"<<endl;
        return false;
    }
    return true;    
}

bool DataConnect(char* ServerAddr)
{
    //向 FTP 服务器发送 PASV 命令
    memset(Command, 0, MAX_SIZE);
    memcpy(Command, "PASV", strlen("PASV"));
    memcpy(Command + strlen("PASV"),"\r\n", 2);
    if (!SendCommand()) {
        return false;
    }

    //获得 PASV 命令的应答信息
    if (RecvReply()) {
        if (nReplyCode != 227) {
            cout<<"PASV response error!"<<endl;
            closesocket(SocketControl);
            return false;
        }
    }

    //分离 PASV 命令应答信息
    char* part[6];
    if (strtok(ReplyMsg, "(")) {
        for(int i = 0; i < 5; i++)
        {
            part[i] = strtok(NULL, ",");
            if (!part[i]) {
                return false;
            }
        }
        part[5] = strtok(NULL,")");
        if (!part[5]) {
            return false;
        }
    } else {
        return false;
    }

    //获取 FTP 服务器数据端口
    unsigned short ServerPort;
    ServerPort = unsigned short((atoi(part[4])<<8)+atoi(part[5]));

    //创建数据连接 Socket
    SocketData = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketData == INVALID_SOCKET) {
        cout<<"Create socket error!"<<endl;
        return false;
    }
    
    //定义 Socket 地址和端口
    sockaddr_in serveraddr2;
    memset(&serveraddr2, 0, sizeof(serveraddr2));
    serveraddr2.sin_family = AF_INET;
    serveraddr2.sin_port = htons(ServerPort);
    serveraddr2.sin_addr.S_un.S_addr = inet_addr(ServerAddr);
    
    //向 FTP 服务器发送 Connect 请求
    int nConnect;
    nConnect = connect(SocketData, (sockaddr*)&serveraddr2, sizeof(serveraddr2));
    if (nConnect == SOCKET_ERROR) {
        cout<<endl<<"Server connect error!"<<endl;
        return false;
    }
    return true;
}

void main(int argc, char* argv[])
{
    //检擦命令行参数
    if (argc != 2) {
        cout<<"Please input command: FtpClient server_addr"<<endl;
        return;
    }

    //建立与 Socket 库绑定
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2,2), &WSAData) != 0) {
        cout<<"WSAStartup error!"<<endl;
        return;
    }
    
    //创建控制连接 Socket
    SocketControl = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketControl == INVALID_SOCKET) {
        cout<<"Create socket error!"<<endl;
        return;
    }
    
    //定义 Socket 地址和端口
    sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(21);
    serveraddr.sin_addr.S_un.S_addr = inet_addr(argv[1]);

    //向 FTP 服务器发送 Connect 请求
    cout<<"FTP->Control Connect...";
    int nConnect;
    nConnect = connect(SocketControl, (sockaddr*)&serveraddr, sizeof(serveraddr));
    if (nConnect == SOCKET_ERROR) {
        cout<<endl<<"Server connect error!"<<endl;
        return;
    }
    
    //获得 Connect 应答信息
    if (RecvReply()) {
        if (nReplyCode == 220) {
            cout<<ReplyMsg;
        } else {
            cout<<"Connect response error!"<<endl;
            closesocket(SocketControl);
            return;
        }
    }

    //向 FTP 服务器发送 USER 命令
    cout<<"FTP>USER:";
    memset(CmdBuf, 0, MAX_SIZE);
    cin.getline(CmdBuf,MAX_SIZE, '\n');
    memset(Command, 0, MAX_SIZE);
    memcpy(Command, "USER ", strlen("USER "));
    memcpy(Command+strlen("USER "), CmdBuf, strlen(CmdBuf));
    memcpy(Command+strlen("USER ")+strlen(CmdBuf),"\r\n",2);
    if (!SendCommand()) {
        return;
    }
    
    //获得 USER 命令的应答信息
    if (RecvReply()) {
        if (nReplyCode == 230 || nReplyCode == 331) {
            cout<<ReplyMsg;
        } else {
            cout<<"USER response error!"<<endl;
            closesocket(SocketControl);
            return;
        }
    }

    if (nReplyCode == 331) {
        //向 FTP 服务器发送 PASS 命令
        cout<<"FTP>PASS:";
        memset(CmdBuf, 0, MAX_SIZE);
        cout.flush();
        for(int i = 0; i < MAX_SIZE; i++)
        {
            CmdBuf[i] = getch();
            if (CmdBuf[i] == '\r') {
                CmdBuf[i] = '\0';
                break;
            } else {
                cout<<'*';
            }
        }
        memset(Command, 0, MAX_SIZE);
        memcpy(Command, "PASS ", strlen("PASS "));
        memcpy(Command+strlen("PASS "), CmdBuf, strlen(CmdBuf));
        memcpy(Command+strlen("PASS ")+strlen(CmdBuf), "\r\n", 2);
        if (!SendCommand()) {
            return;
        }

        //获得 PASS 命令的应答信息
        if (RecvReply()) {
            if (nReplyCode == 230) {
                cout<<ReplyMsg;
            } else {
                cout<<"PASS response error!"<<endl;
                closesocket(SocketControl);
                return;
            }
        }
    }

    //向 FTP 服务器发送 LIST 命令
    cout<<"FTP>LIST"<<endl;
    char FtpServer[MAX_SIZE];
    memset(FtpServer, 0, MAX_SIZE);
    memcpy(FtpServer, argv[1], strlen(argv[1]));
    if (!DataConnect(FtpServer)) {
        return;
    }
    memset(Command, 0, MAX_SIZE);
    memcpy(Command, "LIST", strlen("LEST"));
    memcpy(Command+strlen("LEST"), "\r\n", 2);
    if (!SendCommand()) {
        return;
    }
    
    //获得 LIST 命令的应答信息
    if (RecvReply()) {
        if (nReplyCode == 125 || nReplyCode == 150 || nReplyCode == 226) {
            cout<<ReplyMsg;
        } else {
            cout<<"LIST response error!"<<endl;
            closesocket(SocketControl);
            return;
        }
    }
    
    //获得 LIST 命令的目录信息
    int nRecv;
    char ListBuf[MAX_SIZE];
    while(true){
        memset(ListBuf, 0, MAX_SIZE);
        nRecv = recv(SocketData, ListBuf, MAX_SIZE, 0);
        if (nRecv == SOCKET_ERROR) {
            cout<<endl<<"Socket receive error!"<<endl;
            closesocket(SocketData);
            return;
        }
        if (nRecv <= 0) {
            break;
        }
        cout<<ListBuf;
    }
    closesocket(SocketData);

    //获得 LIST 命令的应答信息
    if (RecvReply()) {
        if (nReplyCode == 226) {
            cout<<ReplyMsg;
        } else {
            cout<<"LIST response error!"<<endl;
            closesocket(SocketControl);
            return;
        }
    }

    //向 FTP 服务器发送 QUIT 命令
    cout<<"FTP>QUIT"<<endl;
    memset(Command, 0, MAX_SIZE);
    memcpy(Command,"QUIT",strlen("QUIT"));
    memcpy(Command+strlen("QUIT"), "\r\n", 2);
    if (!SendCommand()) {
        return;
    }
    
    //获得 QUIT 命令的应答信息
    if (RecvReply()) {
        if (nReplyCode == 221) {
            cout<<ReplyMsg;
            bConnected = false;
            closesocket(SocketControl);
            return;
        } else {
            cout<<endl<<"QUIT response error!"<<endl;
            closesocket(SocketControl);
            return;
        }
    }
    
    WSACleanup();
}
