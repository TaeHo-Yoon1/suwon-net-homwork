#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>

constexpr int SERVER_PORT = 9000;
constexpr int BUFFER_SIZE = 1024;

// 전역 변수
static std::atomic<bool> running{true};
static int client_socket = -1;

// 유틸리티 함수: 에러 로깅
void log_error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

// 서버로부터 메시지를 수신하는 스레드 함수
void receive_messages() {
    char buffer[BUFFER_SIZE];
    
    while (running) {
        int len = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (len <= 0) {
            if (running) {
                log_error("Server disconnected");
                running = false;
            }
            break;
        }
        
        buffer[len] = '\0';
        std::cout << buffer << std::flush;
    }
}

// SIGINT (Ctrl+C) 핸들러
void signal_handler(int) {
    running = false;
    if (client_socket != -1) {
        // 종료 메시지 전송
        const char* quit_msg = "/quit\n";
        send(client_socket, quit_msg, strlen(quit_msg), 0);
        close(client_socket);
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // SIGINT 핸들러 등록
    signal(SIGINT, signal_handler);
    
    // 서버 주소 (기본값: localhost)
    std::string server_ip = "127.0.0.1";
    if (argc > 1) {
        server_ip = argv[1];
    }

    // 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        log_error("Failed to create socket");
        return 1;
    }

    // 서버 주소 설정
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        log_error("Invalid address");
        close(client_socket);
        return 1;
    }

    // 서버 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("Connection failed");
        close(client_socket);
        return 1;
    }

    // 수신 스레드 시작
    std::thread receive_thread(receive_messages);
    receive_thread.detach();

    // 사용자 입력 처리
    std::string input;
    while (running && std::getline(std::cin, input)) {
        if (input.empty()) continue;

        // /quit 명령어 처리
        if (input == "/quit") {
            running = false;
            break;
        }

        // 입력 끝에 개행 추가
        input += '\n';
        
        // 서버로 전송
        if (send(client_socket, input.c_str(), input.length(), 0) < 0) {
            log_error("Failed to send message");
            break;
        }
    }

    // 정리
    running = false;
    close(client_socket);
    
    return 0;
}