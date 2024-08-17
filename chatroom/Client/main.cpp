#include "Client.h"

int main(int argc, char* argv[]) {
    std::string port_str(argv[2]);
    int port = std::stoi(port_str);
    
    Client client(argv[1], port);
    return 0;
}