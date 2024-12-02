#include <iostream>
#include<fstream>
#include<chrono>
#include <time.h>
#include <cmath>
#include <windows.h>
#include<vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
using namespace chrono;
const int maxBuffSize = 1024;//传输缓冲区最大长度
double timeLimit =  1* CLOCKS_PER_SEC;

enum PacketFlags : unsigned char {
    INIT = 0x0,     //初始值
    SYN = 0x1,      // SYN = 1
    ACK = 0x2,      // ACK = 1
    ACK_SYN = 0x3,  // SYN = 1, ACK = 1
    FIN = 0x4,      // FIN = 1
    FIN_ACK = 0x5,  // FIN = 1, ACK = 1
    OVER = 0x7      // 结束
};
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
        flag =INIT;
        seqNum = 0;
    }
};

u_short getCheckSum(u_short* tempHeader, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, tempHeader, size);
    u_long checkSum = 0;
    
    for (int i = 0; i < count; i++) {
        checkSum += buf[i];  
        if (checkSum & 0xffff0000) {  // 检查进位
            checkSum &= 0xffff;
            checkSum++;
        }
    }
    return ~(checkSum & 0xffff);
}

int clientShake(Header header,char* Buffer, SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen,int seqOfShake) {

    if (seqOfShake == 1) {
        header.flag = PacketFlags::SYN;
    }
    else if(seqOfShake == 3){
        header.flag = PacketFlags::ACK_SYN;
    }
    else
    {
        cout<<"[client]"<< "sequence of shake error!" << endl;
        return -1;
    }
    
    header.checkSum = 0;//校验和置零
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;//计算校验和
    if (seqOfShake == 1) {
        memcpy(Buffer, &header, sizeof(header));//将首部放入缓冲区
        if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
        {
            return -1;
        }
    }
    else {
        if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
        {
            return -1;
        }
    }
    

    return 0;
};

int Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)//三次握手建立连接
{
    Header header;
    char* Buffer = new char[sizeof(header)];

    u_short checkSum;


    
    clientShake(header, Buffer, socketClient, servAddr, servAddrlen,1);
    clock_t start = clock(); 
    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //第二次握手
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() -start > timeLimit)//超时重传第一次握手
        {
            cout<<"[client]"<< "second shake from server time out,resending first shake" << endl;
            clientShake(header, Buffer, socketClient, servAddr, servAddrlen,1);
            start = clock();
            
        }
    }


    //校验和检验
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == ACK && getCheckSum((u_short*)&header, sizeof(header) == 0))
    {
        cout<<"[client]"<< "second shake from server received." << endl;
    }
    else
    {
        cout<<"[client]"<< "Connection error!Please restart the client." << endl;
        return -1;
    }

    clientShake(header, Buffer, socketClient, servAddr, servAddrlen, 3);
    cout<<"[client]"<< "Successfully connected to server.Permission for sending data accepted" << endl;
    return 1;
}


void sendDataPack (SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len, int& order)
{
    Header header;
    char* buffer = new char[maxBuffSize + sizeof(header)];
    header.dataLen = len;
    header.seqNum = unsigned char(order);//序列号
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), message, sizeof(header) + len);
    u_short temp = getCheckSum((u_short*)buffer, sizeof(header) + len);//计算校验和
    header.checkSum = temp;
    memcpy(buffer, &header, sizeof(header));

    auto sendStart = high_resolution_clock::now();  // 记录发送开始时间



    sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送
    auto sendEnd = high_resolution_clock::now();  // 记录发送结束时间
    duration<double> sendDuration = sendEnd - sendStart;  // 计算发送时延
    cout << "[client]"<< "Message sent from client" << "---------Information: length(Bytes): " << len << " seqNum: " << int(header.seqNum) << " flag: " << int(header.flag)  <<endl;
    /*cout << "--------Information: length(Bytes): " << len << " seqNum: " << int(header.seqNum) << " flag: " << int(header.flag) << endl;*/
    cout << "Send Round-trip time: " << sendDuration.count() * 1000 << " ms" << endl;
   
    
    double throughput = (len * 8) / (sendDuration.count());  // 单位为bits/秒
    cout << "Throughput: " << throughput << " bits/sec" << endl;

    clock_t start = clock();
    //接收ack等信息
    while (true)
    {
        u_long mode = 1;//socket进入非阻塞模式
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, buffer, maxBuffSize, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            if (clock() - start > timeLimit)
            {
                header.dataLen = len;
                header.seqNum = u_char(order);//序列号
                header.flag = INIT;

                memcpy(buffer, &header, sizeof(header));
                memcpy(buffer + sizeof(header), message, sizeof(header) + len);

                u_short temp = getCheckSum((u_short*)buffer, sizeof(header) + len);//计算校验和
                header.checkSum = temp;

                memcpy(buffer, &header, sizeof(header));
                sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//发送
                cout<<"[client]"<< "Time Out,resending messages" << "---------Inforamation: length(Bytes) : " << len << " seqNum : " << int(header.seqNum) << " flag : " << int(header.flag) << endl;
                
                //cout<<"[client]"<< "      Inforamation: length(Bytes) : " << len << " seqNum : " << int(header.seqNum) << " flag : " << int(header.flag) << endl;
                clock_t start = clock();
            }
        }
        memcpy(&header, buffer, sizeof(header));//缓冲区读取信息
        u_short check = getCheckSum((u_short*)&header, sizeof(header));
        if (header.seqNum == u_short(order) && header.flag == ACK)
        {
            cout<<"[client]"<< "Send has been confirmed!---------Information: Flag:" << int(header.flag) << " seqNum:" << int(header.seqNum) << endl;
            cout<<"[client]"<< "Message sent from client" << "---------Inforamation: " << " seqNum : " << int(header.seqNum) << " flag : " << int(header.flag) << endl;
            //cout<<"[client]"<< "      Inforamation: " << " seqNum : " << int(header.seqNum) << " flag : " << int(header.flag) << endl;
            break;
        }
        else
        {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式
}

void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len)
{
    int packNum = len / maxBuffSize + (len % maxBuffSize != 0);
    int tempSeqNum = 0;
    auto sendStart = high_resolution_clock::now();  // 记录总发送开始时间
    int totalBytesSent = 0;

    for (int i = 0; i < packNum; i++)
    {
        sendDataPack(socketClient, servAddr, servAddrlen, message + i * maxBuffSize, i == packNum - 1 ? len - (packNum - 1) * maxBuffSize : maxBuffSize, tempSeqNum);
        totalBytesSent += (i == packNum - 1 ? len - (packNum - 1) * maxBuffSize : maxBuffSize);
        tempSeqNum++;
        if (tempSeqNum > 255)
        {
            tempSeqNum = tempSeqNum - 256;
        }
    }

    auto sendEnd = high_resolution_clock::now();  // 记录总发送结束时间
    duration<double> sendDuration = sendEnd - sendStart;  // 计算总发送时延

    // 计算往返时延
    double roundTripTime = sendDuration.count() * 1000;  // 转换为毫秒

    // 输出总发送的字节数、往返时延和吞吐率
    cout << "[client]" << "Total Bytes Sent: " << totalBytesSent << " Bytes" << endl;
    cout << "[client]" << "Round-trip time: " << roundTripTime << " ms" << endl;

    double throughput = (totalBytesSent * 8) / sendDuration.count();  // 吞吐率：比特/秒
    cout << "[client]" << "Throughput: " << throughput << " bits/sec" << endl;



    //发送结束信息
    Header header;
    char* Buffer = new char[sizeof(header)];
    header.flag = PacketFlags::OVER;
    header.checkSum = 0;
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;
    memcpy(Buffer, &header, sizeof(header));
    sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
    cout<<"[client]"<< "ending-message sent to server" << endl;
    clock_t start = clock();
    while (1 == 1)
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, Buffer, maxBuffSize, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            if (clock() - start > timeLimit)
            {   
                cout<<"[client]"<< "Time Out,start ending-message resending " << " ";
                char* Buffer = new char[sizeof(header)];
                header.flag = PacketFlags::OVER;
                header.checkSum = 0;
                u_short temp = getCheckSum((u_short*)&header, sizeof(header));
                header.checkSum = temp;
                memcpy(Buffer, &header, sizeof(header));
                sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
                cout<<"[client]"<< "resend ended" << endl;
                start = clock();
            }
        }

       

        //缓冲区读取
        memcpy(&header, Buffer, sizeof(header));
        u_short check = getCheckSum((u_short*)&header, sizeof(header));
        if (header.flag == OVER)
        {
            cout<<"[client]"<< "server received ending-message" << endl;
            break;
        }
        else
            continue;
        
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式
}


int clientWave(Header header ,char* Buffer, SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen,int seqOfWave) {
    if (seqOfWave == 1) {
        header.flag = PacketFlags::FIN;
    }
    else {
        header.flag = PacketFlags::FIN_ACK;
    }
    
    header.checkSum = 0;
    u_short temp = getCheckSum((u_short*)&header, sizeof(header));
    header.checkSum = temp;
    memcpy(Buffer, &header, sizeof(header));
  
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    return 0;
}



int disconnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
    Header header;
    char* Buffer = new char[sizeof(header)];

    u_short checkSum;

    
    clientWave(header, Buffer, socketClient, servAddr, servAddrlen,1);
    clock_t start = clock(); 

    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //接收第二次挥手
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > timeLimit)//超时重新传输
        {
           
            clientWave(header, Buffer, socketClient, servAddr, servAddrlen,1);
            start = clock();
            cout<<"[client]"<< "second wave from server time out，resending first wave" << endl;
        }
    }


    //进行校验和检验
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == ACK && getCheckSum((u_short*)&header, sizeof(header) == 0))
    {
        cout<<"[client]"<< "second wave received from server" << endl;
    }
    else
    {
        cout<<"[client]"<< "connection error，quiting now" << endl;
        return -1;
    }

    //第三次挥手

    clientWave(header, Buffer, socketClient, servAddr, servAddrlen,3);

    start = clock();


    //接收第四次挥手
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > timeLimit)//超时重新传输第三次挥手
        {
            cout<<"[client]"<< "fourth wave from server time out.resending third wave" << endl;
            clientWave(header, Buffer, socketClient, servAddr, servAddrlen,3);
            start = clock();
            cout<<"[client]"<< "" << endl;
        }
    }
    cout<<"[client]"<< "fourth wave from server received,disconnection completed." << endl;
    return 1;
}


int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKET server;

    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(2456);
    server_addr.sin_addr.s_addr = htonl(2130706433);

    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    
    if (Connect(server, server_addr, len) == -1)
    {
        return -1;
    }

    string filename;
    cout<<"[client]"<< "input filename:";
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);
    char* buffer = new char[10000000];
    int i = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        buffer[i] = temp;
        temp = fin.get();
        i++;
    }
    fin.close();
    // 打印文件内容大小
    //cout<<"[client]"<< "File content size: " << i << " bytes." << endl;
    send(server, server_addr, len, (char*)(filename.c_str()), filename.length());
    clock_t start = clock();
    send(server, server_addr, len, buffer, i);
    clock_t end = clock();
   // cout<<"[client]"<< "Transmission done!" << endl;
   // cout<<"[client]"<< "Info: total time:" << (end - start) / CLOCKS_PER_SEC << "s, " << "throughPut:" << ((float)i) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
    disconnect(server, server_addr, len);
}
