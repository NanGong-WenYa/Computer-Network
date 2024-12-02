#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")
using namespace std;


const int maxBuffSize = 1024;//缓冲区最大长度

double timeLimit = 1 * CLOCKS_PER_SEC;


u_short getCheckSum(u_short* tempHeader, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, tempHeader, size);
    u_long checkSum = 0;

    for (int i = 0; i < count; i++) {
        checkSum += buf[i];  
        if (checkSum & 0xffff0000) {  // 检查是否有进位
            checkSum &= 0xffff;
            checkSum++;
        }
    }
    return ~(checkSum & 0xffff);
}


enum PacketFlags : unsigned char {
    INIT = 0x0,     //初始值
    SYN = 0x1,      // SYN = 1
    ACK = 0x2,      // ACK = 1
    ACK_SYN = 0x3,  // SYN = 1, ACK = 1
    FIN = 0x4,      // FIN = 1
    FIN_ACK = 0x5,  // FIN = 1, ACK = 1
    OVER = 0x7      // 结束
};

// int转换成对应的PacketFlags类型
PacketFlags intToFlag(int a) {
    switch (a) {
    case 0x0:
        return INIT;
    case 0x1:
        return SYN;
    case 0x2:
        return ACK;
    case 0x3:
        return ACK_SYN;
    case 0x4:
        return FIN;
    case 0x5:
        return FIN_ACK;
    case 0x7:
        return OVER;
    default:
        std::cerr << "Unknown flag value: " << a << std::endl;
        return INIT; // 默认返回 INIT
    }
}

struct Header
{
    //u_short16位
    u_short checkSum = 0;
    u_short dataLen = 0;
    PacketFlags flag;

    unsigned char seqNum = 0;

    Header() {
        checkSum = 0;
        dataLen = 0;
        flag = INIT;
        seqNum = 0;
    }
};

int serverShake(Header header,char*Buffer,SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen,int seqOfShake)    {
    if (seqOfShake == 2) {
        header.flag = ACK;
    }
    else {
        cout<<"[server]"<< "server shake error!" << endl;
        return -1;
        
    }
    
    header.checkSum = 0;
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;
    memcpy(Buffer, &header, sizeof(header));
    if (sendto(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, clientAddrLen) == -1)
    {
        return -1;
    }
    return 0;
}

int Connect(SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen)
{

    Header header;
    char* Buffer = new char[sizeof(header)];

    //监听第一次握手
    while (1 == 1)
    {
        if (recvfrom(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, &clientAddrLen) == -1)
        {
            return -1;
        }
        memcpy(&header, Buffer, sizeof(header));
        if (header.flag == SYN && getCheckSum((u_short*)&header, sizeof(header)) == 0)
        {
            cout<<"[server]"<< "first wave from client received." << endl;
            break;
        }
    }

    //发送第二次握手信息

    serverShake(header, Buffer, serverSock, clientAddr, clientAddrLen, 2);
    clock_t start = clock();

    //接收第三次握手
    while (recvfrom(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, &clientAddrLen) <= 0)
    {
        if (clock() - start > timeLimit)
        {
            cout<<"[server]"<< "third wave from client time out,resending second wave" << endl;
            serverShake(header, Buffer, serverSock, clientAddr, clientAddrLen, 2);
            
        }
    }

    Header temp1;
    memcpy(&temp1, Buffer, sizeof(header));
    if (temp1.flag == ACK_SYN && getCheckSum((u_short*)&temp1, sizeof(temp1) == 0))
    {
        cout<<"[server]"<< "Connection established!Data transmission is permitted. " << endl;
    }
    else
    {
        cout<<"[server]"<< "Connection error from Client, restart for Client is required!" << endl;
        return -1;
    }
    return 1;
}

int sendACK(Header header, char* Buffer, SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen,int seq) {
    header.flag = ACK;
    header.dataLen = 0;
    header.seqNum = (unsigned char)seq;
    header.checkSum = 0;
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;
    memcpy(Buffer, &header, sizeof(header));
    
    sendto(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, clientAddrLen);
    cout<<"[server]"<< "Send to Clinet ACK:" << (int)header.seqNum << " seqNum:" << (int)header.seqNum << endl;
    return 0;
}
int sendOver(Header header, char* Buffer, SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen) {
    header.flag = OVER;
    header.checkSum = 0;
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;
    memcpy(Buffer, &header, sizeof(header));
    if (sendto(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, clientAddrLen) == -1)
    {
        return -1;
    }
    return 0;
}

int ListenRecv(SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen, char* message)
{
    long int packLen = 0;//文件长度
    Header header;
    char* Buffer = new char[maxBuffSize + sizeof(header)];
    int seq = 0;
   

    while (1 == 1)
    {
        int length = recvfrom(serverSock, Buffer, sizeof(header) + maxBuffSize, 0, (sockaddr*)&clientAddr, &clientAddrLen);//接收报文长度
        
        memcpy(&header, Buffer, sizeof(header));
        //判断是否结束
        if (header.flag == OVER && getCheckSum((u_short*)&header, sizeof(header)) == 0)
        {
            cout<<"[server]"<< "Data package received." << endl;
            break;
        }
        if (header.flag == INIT && getCheckSum((u_short*)Buffer, length - sizeof(header)))
        {
            //是否接受的不是指定的包
            if (seq != int(header.seqNum))
            {
                sendACK(header, Buffer, serverSock, clientAddr, clientAddrLen, seq);
                continue;//丢弃
            }
            seq = int(header.seqNum);
            if (seq > 255)
            {
                seq = seq - 256;
            }
            //取buffer存的的数据
            cout<<"[server]"<< "Send message " << endl;
            cout<<"[server]"<<"      information: length(Bytes):" <<length - sizeof(header) << "   seqNum :" << int(header.seqNum) << "   Flag: " << int(header.flag) << endl;
            char* temp = new char[length - sizeof(header)];
            memcpy(temp, Buffer + sizeof(header), length - sizeof(header));
           
            memcpy(message + packLen, temp, length - sizeof(header));
            packLen = packLen + int(header.dataLen);


            sendACK(header, Buffer, serverSock, clientAddr, clientAddrLen, seq);
            seq++;
            if (seq > 255)
            {
                seq = seq - 256;
            }
        }
    }
 
    sendOver(header, Buffer, serverSock, clientAddr, clientAddrLen);
    return packLen;
}

int serverWave(Header header, char* Buffer, SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen, int seqOfWave) {
    if (seqOfWave == 2)
    {
        header.flag = ACK;
    }
    else {
        header.flag = FIN_ACK;
    }
    
    header.checkSum = 0;
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;
    memcpy(Buffer, &header, sizeof(header));
    if (sendto(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, clientAddrLen) == -1)
    {
        return -1;
    }
}

int disconnect(SOCKET& serverSock, SOCKADDR_IN& clientAddr, int& clientAddrLen)
{
    Header header;
    char* Buffer = new char[sizeof(header)];
    while (1 == 1)
    {
        int length = recvfrom(serverSock, Buffer, sizeof(header) + maxBuffSize, 0, (sockaddr*)&clientAddr, &clientAddrLen);//接收报文长度
        memcpy(&header, Buffer, sizeof(header));
        if (header.flag == FIN && getCheckSum((u_short*)&header, sizeof(header)) == 0)
        {
            cout<<"[server]"<< "receieved first wave from client" << endl;
            break;
        }
    }
    //发送第二次挥手
    serverWave(header, Buffer, serverSock, clientAddr, clientAddrLen, 2);
    clock_t start = clock();

    //接收第三次挥手
    while (recvfrom(serverSock, Buffer, sizeof(header), 0, (sockaddr*)&clientAddr, &clientAddrLen) <= 0)
    {
        if (clock() - start > timeLimit)
        {
            cout<<"[server]"<< "third wave from client TIME OUT, resending second wave now" << endl;
            serverWave(header, Buffer, serverSock, clientAddr, clientAddrLen, 2);
            
        }
    }

    Header temp1;
    memcpy(&temp1, Buffer, sizeof(header));
    if (temp1.flag == FIN_ACK && getCheckSum((u_short*)&temp1, sizeof(temp1) == 0))
    {
        cout<<"[server]"<< "received third wave" << endl;
    }
    else
    {
        cout<<"[server]"<< "error(third wave receive)" << endl;
        return -1;
    }

    //发送第四次挥手
    serverWave(header, Buffer, serverSock, clientAddr, clientAddrLen, 4);
    cout<<"[server]"<< "Completed:fourth wave,disconnected!" << endl;
    return 1;
}


int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN serverAddr;
    SOCKET server;

    serverAddr.sin_family = AF_INET;//IPV4
    serverAddr.sin_port = htons(2456);
    serverAddr.sin_addr.s_addr = htonl(2130706433);

    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    cout<<"[server]"<< "Listening for client to start....." << endl;
    int len = sizeof(serverAddr);
    //建立连接
    Connect(server, serverAddr, len);
    char* clientID = new char[20];
    char* dataPack = new char[100000000];
    int namelen = ListenRecv(server, serverAddr, len, clientID);
    int datalen = ListenRecv(server, serverAddr, len, dataPack);
    string a;
    for (int i = 0; i < namelen; i++)
    {
        a = a + clientID[i];
    }
    disconnect(server, serverAddr, len);
    ofstream fout(a.c_str(), ofstream::binary);
    for (int i = 0; i < datalen; i++)
    {
        fout << dataPack[i];
    }
    fout.close();
    cout<<"[server]"<< "Download datapack successfully!" << endl;
}

