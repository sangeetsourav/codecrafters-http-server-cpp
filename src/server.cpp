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
#include <thread>
#include <filesystem>
#include <fstream>

std::map<std::string, std::string> parse_http_request(std::string request, std::string files_endpt_dir)
{
	std::map<std::string, std::string> http_info;

	std::istringstream ss(request);

	std::string request_type, request_target, http_version;

	// The following are the first three space delimited tokens
	ss >> request_type >> request_target >> http_version;

	http_info["request_type"] = request_type;
	http_info["request_target"] = request_target;
	http_info["http_version"] = http_version;
	http_info["Content-Type"] = "";
	http_info["Content-Length"] = "";
	http_info["request_body"] = "";
	http_info["response"] = "";
	http_info["status_code"] = " 404 Not Found";

	std::string request_status = "";

	// Regex to capture /echo/{string}
	std::regex echo_endpt(R"(\/echo\/(.*))");

	// Regex to capture user-agent
	std::regex user_agent_endpt(R"(user-agent: (.*)\r\n)", std::regex::icase);

	// Regex to capture /files/{filename}
	std::regex files_endpt(R"(\/files\/(.*))");
	std::smatch match;

	if (http_info["request_target"] == "/")
	{
		http_info["status_code"] = " 200 OK";
	}

	else if (std::regex_match(request_target, match, echo_endpt) || std::regex_search(request, match, user_agent_endpt))
	{
		http_info["Content-Type"] = "Content-Type: text/plain";
		http_info["Content-Length"] = "Content-Length: " + std::to_string(match[1].str().size());
		http_info["request_body"] = match[1].str();
		http_info["status_code"] = " 200 OK";
	}

	else if (std::regex_match(request_target, match, files_endpt))
	{
		std::filesystem::path file_path = std::filesystem::path(files_endpt_dir) / std::filesystem::path(match[1].str());

		http_info["Content-Type"] = "Content-Type: application/octet-stream";
		http_info["request_body"] = "";
		
		if (std::filesystem::exists(file_path))
		{
			std::uintmax_t fileSize = std::filesystem::file_size(file_path);
			http_info["Content-Length"] = "Content-Length: " + std::to_string(fileSize);

			std::ifstream file(file_path.c_str());
			std::string str;
			while (std::getline(file, str))
			{
				http_info["request_body"] += str;
			}

			http_info["status_code"] = " 200 OK";
		}
	}

	http_info["response"] = http_info["http_version"] + http_info["status_code"] + "\r\n" + http_info["Content-Type"] + "\r\n" + http_info["Content-Length"] + "\r\n\r\n" + http_info["request_body"];

	return http_info;
}

void process_request(int client_socket, std::string files_endpt_dir)
{
	// Buffer variable where the data is temporarily stored
	char buffer[4096];

	ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

	// Add null termination
	buffer[bytes_received] = '\0';

	// Convert request to string
	std::string complete_request(buffer, bytes_received);

	auto http_info = parse_http_request(complete_request, files_endpt_dir);

	// Send response back
	ssize_t bytes_sent = send(client_socket, http_info["response"].c_str(), http_info["response"].size(), 0);

	close(client_socket); // Done with this client
}

int main(int argc, char **argv) {
	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
  
	// You can use print statements as follows for debugging, they'll be visible when running tests.
	std::cout << "Logs from your program will appear here!\n";

	std::string directory = "";

	if (argc==3)
	{
		if (strcmp(argv[1], "--directory") == 0)
		{
			directory = argv[2];
		}
	}

	std::cout << directory << "\n";

	
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
  
	std::cout << "Waiting for clients to connect...\n";
	
	while (true)
	{
		// Accept incoming connection and create a new socket that will communicate
		int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);

		std::cout << "Client connected. Creating a thread to handle this\n";
		
		std::thread worker(process_request, client_socket, directory);
		worker.detach();
	}

	close(server_fd);

	return 0;
}