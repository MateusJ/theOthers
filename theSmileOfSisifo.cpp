#include <iostream>
#include <iomanip>
#include <fstream>
#include <bitset>
#include <string>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/rand.h> 
#include <jwt-cpp/jwt.h>
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace std;
using json = nlohmann::json;


/*
 * function	to generate a random 16-byte hex string
 * @return string
 */
string generateJwtId() {
    unsigned char buffer[16];
    RAND_bytes(buffer, sizeof(buffer));
    stringstream ss;
    for (int i = 0; i < sizeof(buffer); i++) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(buffer[i]);
    }
    return ss.str();
}

picojson::value convertToPicoValue(const json& j) {
    stringstream ss;
    ss << j;
    picojson::value v;
    ss >> v;
    return v;
}

/*
 * function to sign jwt
 * @param1 payload with claims
 * @param2 private key
 * @return signed jwt as a string
 */
string signJwt(const json& payload, const string& privateKey) {
	try {
		picojson::value picoPayload = convertToPicoValue(payload);
		auto payloadMap = picoPayload.get<picojson::object>();
		auto token = jwt::create().set_algorithm("RS256").set_type("JWT");
		for (const auto& entry : payloadMap) {
            token.set_payload_claim(entry.first, jwt::claim(entry.second));
        }

		auto signedToken = token.sign(jwt::algorithm::rs256("", privateKey, "", ""));
		cout << signedToken << endl;
    	return signedToken;
    } catch (const exception& e) {
        cerr << "Error signing JWT: " << e.what() << endl;   
        return "";  
    }
}

/*
 * function to write responses and status into file
 * @param1 file name
 * @param2 response
 * @param3 status
 */
void writeToFile(const string& fileName, const string& response, const string& allGood) {
	time_t timeStamp = chrono::system_clock::to_time_t(chrono::system_clock::now());
	string timeStampStr = ctime(&timeStamp);
	ofstream outputFile(fileName, ios::app);
	if(outputFile.is_open()) {
		outputFile << timeStampStr << "status: " + allGood << endl << response << endl;
		outputFile.close();
		cout << "Data successfully written to " << fileName << endl;
	} else {
		cout << "Error openning file" << endl;
	}
}

/*
 * function	to delete logfile if it exists
 */
void checkFile() {
	FILE *f;
	if (f = fopen("output.txt", "r")) {
		remove("output.txt");
	}
}

/*
 * function to set timeout
 * @param1 socket
 * @param2 timeout value in seconds
 * @return boolean
 */
bool setSocketTimeout(int sockfd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        return false;
    }
    return true;
}

/*
 * function to verify the order of the states
 * @param1 received nextSeqNumber
 * @param2 last seqNumber sent (state)
 * @return true if verification succeeds, false otherwise
 */
bool verifySeqOrder(const int& receivNeSeqNumb, const int& state) {
	if (state == 6) {
		return receivNeSeqNumb == 0;
	} else {
		return receivNeSeqNumb == state + 1;
	}
}

/*
 * function to verify the signature of a JWT using the provided public key
 * @param1 jwt to verify
 * @param2 public key
 * @param3 actual state
 * @return true if verification succeeds, false otherwise
 */
bool verifyJwt(const string& jwt, const string& publicKey, int& state) {
	const string secretKey = "secret-key-for-dec7557\n";
	try {
    	auto decoded = jwt::decode(jwt);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secretKey})
            .verify(decoded);
        string payloadStr = decoded.get_payload();
        int rnsn = decoded.get_payload_claim("next_number").as_number();
        if(verifySeqOrder(rnsn, state)) {
        	writeToFile("output.txt", payloadStr, "OK");
        	return true;	
        } else {
        	writeToFile("output.txt", payloadStr, "NOT OK -> NEXTNUMBER OUT OF ORDER");
        	return false;	
        }
    } catch (const exception& e) {
        cerr << "JWT verification failed: " << e.what() << endl;
        writeToFile("output.txt", "Verification failed", "NOT OK");
        return false;
    }
}

/*
 * function to load the keys
 * @param path to keys
 * @return key
 */
string loadKeyFromFile(const string& filePath) {
    ifstream fileStream(filePath);
    string key((istreambuf_iterator<char>(fileStream)), istreambuf_iterator<char>());
    return key;
}

/*
 * function to handle the payload claims
 * @param actual state
 * @return json payload
 */
json payLoadHandler(const int& seq_state) {
	vector<string> seq_states;
	seq_states = {"RED", "ORANGE", "YELLOW", "GREEN", "BLUE", "INDIGO", "VIOLET"};
	
	vector<int> registrations;
	registrations = {21203170, 20105143, 21101364, 21203170, 20105143, 21101364, 21203170};
	
	json payLoad = {
		{"sub", "THEOTHERS"},
        {"aud", "udp.socket.server.for.jwt"},
        {"jti", generateJwtId()},
        {"iat", time(0)},
        {"exp", time(0) + 30},
        {"seq_state_number", seq_state},
        {"seq_state", seq_states[seq_state]},
        {"seq_max", 3},
        {"registration", registrations[seq_state]},
	};
	return payLoad;
}

/*
 * function to send the token
 * @param1 socket
 * @param2 token jwt
 * @param3 socket structure related
 * @return boolean
 */
bool sendJwtToken(int sockfd, const string& jwtToken, const sockaddr_in& serverAddr) {
    ssize_t sentBytes = sendto(sockfd, jwtToken.c_str(), jwtToken.length(), 0,
                               (const struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if (sentBytes < 0) {
        perror("Error sending JWT token");
        return false;
    }
    return true;
}

/*
 * function to receive the token
 * @param1 socket
 * @param2 socket structure related
 * @param3 timeout in sec
 * @return token
 */
string receiveJwtToken(int sockfd, sockaddr_in& serverAddr, int timeoutSeconds) {
    char buffer[1024];
    socklen_t serverAddrLen = sizeof(serverAddr);
    if (!setSocketTimeout(sockfd, timeoutSeconds)) {
        cerr << "Error setting socket timeout" << endl;
        return "";
    }

    ssize_t receivedBytes = recvfrom(sockfd, buffer, 1024 - 1, 0,
                                     (struct sockaddr*)&serverAddr, &serverAddrLen);

    if (receivedBytes < 0) {
        perror("Error receiving JWT token");
        return "";
    }

    buffer[receivedBytes] = '\0'; // Null-terminate the received data
    return string(buffer);
}

int main() {
    const string privateKey = loadKeyFromFile("/home/jean/Documents/redes/private_key.pem");
    const string publicKey = loadKeyFromFile("/home/jean/Documents/redes/public_key.pem");
	const string ip = "172.233.186.185";
	const int port = 44555;
	const int setTimeOut = 10;
	checkFile();
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }
    
    // set up server address structure
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(serverAddr.sin_addr));

	for (int state = 0; state < 7; state++) {
        //sign the jwt using the private key and create payload
        string jwt = signJwt(payLoadHandler(state), privateKey);

        //send the jwt token to the server
        if (sendJwtToken(sockfd, jwt, serverAddr)) {
            cout << "JWT signed and sent successfully." << endl;
            
            //receive the jwt token from the server
            sockaddr_in receivedServerAddr;
            string receivedJwt = receiveJwtToken(sockfd, receivedServerAddr, setTimeOut);
            
            if (!receivedJwt.empty()) {
                //verify the jwt signature using the public key
                if (verifyJwt(receivedJwt, publicKey, state)) {
                    cout << "JWT received and verified successfully." << endl;
                } else {
                    cerr << "Failed to verify received JWT." << endl;
                    sleep(3);
                    state = 0; //start from scratch
                }
            } else {
                cerr << "Failed to receive JWT." << endl;
                return 1;
            }
        } else {
            cerr << "Failed to send JWT." << endl;
            return 1;
        }
    }

    // close socket
    close(sockfd);
	
    return 0;
}
