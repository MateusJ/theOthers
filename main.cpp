#include <winsock2.h>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main(int argc, char const *argv[])
{
    //Declaração das variaveis de endpoint
    string ip_host = "43.204.246.63";  
    int port_host = 34567;

    //Todos os requests já estão codificados e estão armazenados no vetor na ordem de seq_number 0, 1, 2 e 3
    vector<string> requests {"eyJhbGciOiJSUzI1NiIsImN0eSI6IkpXVCJ9.eyJncm91cCI6IlBJTkhBTyIsInNlcV9udW1iZXIiOjAsInNlcV9tYXgiOjMsIm1hdHJpY3VsYSI6MjIyNTA3NzR9.tYnvJd1seng8oKn-wggpdazZou0ebnnGKtjvpamo_w3gUUiWMFCV5JY5YaBEDVgIZsP64-r49kroNoOfeGEb-b10q9eIRedu2cwRkwJUcaioOcLsZLOq2_KIvwMTGzMs9EAzxgPT3Lpf4wfeclMfNV0uJrLZtSUhLZSdh0Yv9gU",
                             "eyJhbGciOiJSUzI1NiIsImN0eSI6IkpXVCJ9.eyJncm91cCI6IlBJTkhBTyIsInNlcV9udW1iZXIiOjEsInNlcV9tYXgiOjMsIm1hdHJpY3VsYSI6MjIyNTA3NzR9.mn9kvfv7PLCZfB7vCn_rM-gVO_LgTEExM7MFYeHjyT6gh9mP380vGB4uNIj7VNyDnuqV58YunqzTsgFb0I8V2l-O4vGNCM4jcEP1EugaxJtnUhJ7NxEwG_-KhGtzzpL3pzUkPQ3hsk4ENDAp-7_kDXr0DXHikd1Exv2QCLoCWHw",
                             "eyJhbGciOiJSUzI1NiIsImN0eSI6IkpXVCJ9.eyJncm91cCI6IlBJTkhBTyIsInNlcV9udW1iZXIiOjIsInNlcV9tYXgiOjMsIm1hdHJpY3VsYSI6MjExMDUwODd9.yzlXWE2E8BszgispyFwERtMygoiquWsV3RI16Hn6YQZO3sP5pZI2KMEniQzwlJi4-vKPLWNxugNCOkvLaA0IYR0O3WQMwrRxVwy4ZEtM_vhqnExqQW9lcu1V53rtjFOC4GEsGsLi7sCmYG6hKpiHFwjYXqYh3C4KkX8FbadZCPU",
                             "eyJhbGciOiJSUzI1NiIsImN0eSI6IkpXVCJ9.eyJncm91cCI6IlBJTkhBTyIsInNlcV9udW1iZXIiOjMsInNlcV9tYXgiOjMsIm1hdHJpY3VsYSI6MjIyMTUzMTZ9.L-N6Dht6q9I-fjXeFPOwto0uYchNAclyBBktxDnCwIANXFoE2mVrK07ssFjveGLu3t086-waCsNj8rRLEdXGpYykbr_QOpeayK3ZZcnUKBm7hH9YDJo6H-ugSSHAgPbBYRbex0yuM22aI72Nqd5yYRuSgtv9eQA5v4e6pic5BOE"
    };

    string str_out;

    //Inicializa a biblioteca Winsock
    WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "Falha ao inicializar o Winsock." << endl;
            return false;
        }

    //Cria o socket UDP para cada enviar os requests
    for(int i = 0; i < requests.size(); i++){
        SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == INVALID_SOCKET) {
            cerr << "Falha ao criar o socket." << endl;
            WSACleanup();
            return false;
        }
        // Configura o endpoit do servidor
        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port_host);
        serverAddress.sin_addr.s_addr = inet_addr(ip_host.c_str());

        //Envia payload ao servidor
        string request = requests[i];
        if(sendto(sockfd, request.c_str(), request.size(), 0, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) != SOCKET_ERROR) {
            
            //Estrutura do timeout
            fd_set fds ;
            int n ;
            struct timeval tv ;
            FD_ZERO(&fds) ;
            FD_SET(sockfd, &fds) ;
            tv.tv_sec = 7 ; //Tempo em segundos do timeout
            tv.tv_usec = 0 ;

            // Espera até o tempo esgotar ou receber dados
            n = select(sockfd, &fds, NULL, NULL, &tv ) ;

            if(n == 0){ // Tempo esgotado
                closesocket(sockfd);
            }else if(n == -1 ){ // Retornou erro
                printf("Error..\n");
                closesocket(sockfd);
            }else{
                // Armazena a resposta do servidor em log
                char buffer[1024];
                int bytesRead = recvfrom(sockfd, buffer, sizeof(buffer), 0, nullptr, nullptr);
                string response(buffer, bytesRead);
                str_out += response;
                str_out += "\n";
                closesocket(sockfd);
            }
        }
    }

    //Salva log.txt
    str_out += "\n";
    ofstream log;
    log.open("log.txt", ios::app);
    log << str_out;

    //Encerra a biblioteca;
    WSACleanup();
    return 0;
}