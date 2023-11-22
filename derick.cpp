#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <jwt-cpp/jwt.h>

int main()
{
    int port = 44555;
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[1024];
    socklen_t addr_size;

	std::string rsa_priv_key = R"(-----BEGIN OPENSSH PRIVATE KEY-----
b3BlbnNzaC1rZXktdjEAAAAACmFlczI1Ni1jdHIAAAAGYmNyeXB0AAAAGAAAABA8bR7bF0
MO5GlR8d86lkd9AAAAEAAAAAEAAABoAAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlz
dHAyNTYAAABBBOIjs4d648RzCycDJzoa2Wj6vixD3Xk/yPW0kWMG/AOo59qVYF3aO1JY0s
F521hu7Vd9ijcZg13agukq35Qt5LYAAACwWtIHC5IMHbdivb8CRzLLyfAHiWJxgTLLf6vv
VKxwUpzAhdWxfH/LnoEC2AVunrBKKoTY+3T3V/tI7HpZPgHq+eeaenW5L4enaulQPoGnNo
Q0uX5VOl1r1auJUbp3X5/mkssyd5cwdufoC+SUFnGMcmltkoCNjLiPBfF05WixW0tfZIZr
R3l9AFBaIVHPOoJX9XFxm7nT7Nl3+8AvCp+efn7h/hzdGSVFV3Gi8YUarDI=
-----END OPENSSH PRIVATE KEY-----)";

    // Cria um socket UDP
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    memset(&serverAddr, '\0', sizeof(serverAddr));


	auto token = jwt::create()
					 .set_issuer("auth0")
					 .set_type("JWT")
					 .set_id("rsa-create-example")
					 .set_issued_at(std::chrono::system_clock::now())
					 .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{36000})
					 .set_payload_claim("sample", jwt::claim(std::string{"test"}))
					 .sign(jwt::algorithm::es256("", rsa_priv_key, "", ""));

	std::cout << "token:\n" << token << std::endl;
    // Configura os parâmetros do servidor
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr("172.233.186.185");

    // Copia a mensagem para o buffer
    strcpy(buffer, "Hello Server\n");

    // Envia a mensagem para o servidor
    sendto(sockfd, buffer, 1024, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    printf("[+]Data Send: %s", buffer);

    // Espera pela resposta do servidor
    recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
    printf("[+]Data Received: %s", buffer);

    // Fecha o socket

    return 0;
}
