#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <vector>
#include <regex>

int main(int argc, char **argv) {
	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
  
	// You can use print statements as follows for debugging, they'll be visible when running tests.
	std::cout << "Logs from your program will appear here!\n";
	
	// AF_INET is IPv4
	// SOCK_STREAM means we are using TCP and not UDP
	// 0 means protocol is chosen automatically
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// If return value is negative then socket has not been created
	if (server_fd < 0) {
		std::cerr << "Failed to create server socket\n";
		return 1;
	}
  
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		std::cerr << "setsockopt failed\n";
		return 1;
	}
	
	// Bind to IP and port
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	// Set to any IP on this machine (0.0.0.0)
	server_addr.sin_addr.s_addr = INADDR_ANY;
	// Set to port 4221
	server_addr.sin_port = htons(4221);	//host to network short
	
	// Run bind command
	if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
		std::cerr << "Failed to bind to port 4221\n";
		return 1;
	}
	
	// Max number of connection requests in queue
	int connection_backlog = 5;	
	// Mark the socket for listening
	if (listen(server_fd, connection_backlog) != 0) {
		std::cerr << "listen failed\n";
		return 1;
	}
  
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
  
	std::cout << "Waiting for a client to connect...\n";
	
	// Accept incoming connection and create a new socket that will communicate
	int client_socket = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
	std::cout << "Client connected\n";

	close(server_fd);

	// Buffer variable where the data is temporarily stored
	char buffer[4096];
	ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
	// Add null termination
	buffer[bytes_received] = '\0';

	// Convert request to string
	std::string s(buffer, bytes_received);

	std::istringstream ss(s);

	std::string request_type, request_target, http_version;

	// The following are the first three space delimited tokens
	ss >> request_type >> request_target >> http_version;

	std::string response;
	
	// Regex to capture /echo/{string}
	std::regex r(R"(\/echo\/(.*))");
	std::smatch match;

	if (request_target == "/")
	{
		response = "HTTP/1.1 200 OK\r\n\r\n";
	}
	else if (std::regex_match(request_target, match, r))
	{
		response = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ")
			+ std::to_string(match[1].str().size())
			+ std::string("\r\n\r\n") + match[1].str();
	}
	else
	{
		response = "HTTP/1.1 404 Not Found\r\n\r\n";
	}

	// Send response back
	ssize_t bytes_sent = send(client_socket, response.c_str(), response.size(), 0);

	close(client_socket); // Done with this client

	return 0;
}


//std::string request_line;
//std::vector<std::string> headers;
//std::string request_body;
//
//size_t pos = 0;
//int item_no = 0;
//while (true)
//{
//	size_t end = s.find("\r\n", pos);
//
//	if (end == std::string::npos) {
//
//		request_body = s.substr(pos);
//		item_no = 2;
//		break;
//	}
//
//	if (item_no == 0)
//	{
//		request_line = s.substr(pos, end - pos);
//		item_no = 1;
//	}
//	else if (item_no == 1)
//	{
//		headers.push_back(s.substr(pos, end - pos));
//	}
//
//	pos = end + 2;
//}