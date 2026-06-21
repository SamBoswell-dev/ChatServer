#include "Application.h"
#include "../Util/Remote.h"
#include "../Util/Helpers.h"

#include <ws2tcpip.h>
#include <string>

void Application::Init()
{
	std::cout << "TCP port number: ";
	std::cin >> m_listenPort; // TODO: Perform validation

	std::cout << "\nChat capacity: ";
	std::cin >> m_chatCapacity;

	std::cout << "\nCommand character: ";
	std::cin >> m_commandCharacter;

	if (!SetupListenSocket())
	{
		std::cout << "Error creating the listen socket\n";
		std::exit(1);
	}

	char hostName[256];
	gethostname(hostName, sizeof(hostName));
	
	struct addrinfo hints;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(hostName, std::to_string(m_listenPort).c_str(), &hints, &result);

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		char ipString[INET6_ADDRSTRLEN];
		void* rawAddress = NULL;
		unsigned short numericPort = 0;

		if (ptr->ai_family == AF_INET) {
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)ptr->ai_addr;
			rawAddress = &(ipv4->sin_addr);
			numericPort = ntohs(ipv4->sin_port);

			InetNtopA(AF_INET, rawAddress, ipString, sizeof(ipString));
			printf("[IPv4 Address]: %s:%d\n", ipString, numericPort);
		}
		else if (ptr->ai_family == AF_INET6) {
			struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)ptr->ai_addr;
			rawAddress = &(ipv6->sin6_addr);
			numericPort = ntohs(ipv6->sin6_port);

			InetNtopA(AF_INET6, rawAddress, ipString, sizeof(ipString));
			printf("[IPv6 Address]: [%s]:%d\n", ipString, numericPort);
		}
	}

}

void Application::Run()
{
	fd_set clientFds;
	FD_ZERO(&clientFds);
	FD_SET(m_listenSocket, &clientFds);

	while (true)
	{
		fd_set tempFds = clientFds;
		select(0, &tempFds, nullptr, nullptr, nullptr);

		for (int i = 0; i < static_cast<int>(tempFds.fd_count); ++i)
		{
			if (tempFds.fd_array[i] == m_listenSocket) // New client connection
			{
				SOCKET clientFd = accept(m_listenSocket, nullptr, nullptr);
				FD_SET(clientFd, &clientFds);
				std::string welcomeMessage = std::string("Welcome to the server. The prefix for issuing commands is ") + m_commandCharacter;
				SendMessageToClient(clientFd, &welcomeMessage[0], welcomeMessage.size() + 1);
			}
			else // Incoming client message
			{
				// Read message
				std::string msg = ReadMessageFromClient(tempFds.fd_array[i]);

				
				if (msg[0] == m_commandCharacter) // Is command message
				{
					HandleCommand(tempFds.fd_array[i], msg);
				}
				else if (msg.empty()) // Client disconnect
				{
					FD_CLR(tempFds.fd_array[i], &clientFds);
				}
				else // Is other message
				{

				}
			}
		}
	}
}

bool Application::SetupListenSocket() 
{
	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listenSocket == INVALID_SOCKET)
		return false;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(m_listenPort);

	int result = bind(m_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (result == SOCKET_ERROR)
		return false;

	result = listen(m_listenSocket, 1);
	if (result == SOCKET_ERROR)
		return false;

	return true;
}

void Application::SendMessageToClient(SOCKET comSocket, char* data, int32_t length)
{
	uint8_t packetLength = static_cast<uint8_t>(length);
	SendTcpData(comSocket, reinterpret_cast<char*>(&packetLength), 1); // TODO: error check
	SendTcpData(comSocket, data, static_cast<uint16_t>(packetLength)); // TODO: error check
}

// Empty string indicates disconnect
std::string Application::ReadMessageFromClient(SOCKET comSocket)
{
	uint8_t messageSize = 0;
	int result = RecvTcpData(comSocket, (char*)&messageSize, 1);
	if ((result == 0)) // Graceful disconnect
	{
		return "";
	}
	else if ((result == SOCKET_ERROR))
	{
		int errorCode = WSAGetLastError();
		std::cout << "Error while reading message size from client: " << errorCode << std::endl;
		return "";
	}

	std::string buffer;
	buffer.resize(messageSize);
	result = RecvTcpData(comSocket, &buffer[0], messageSize);
	if ((result == SOCKET_ERROR) || (result == 0))
	{
		int errorCode = WSAGetLastError();
		std::cout << "Error while reading message from client: " << errorCode << std::endl;
	}

	return buffer;
}

void Application::HandleCommand(SOCKET comSocket, const std::string& command)
{
	std::string_view msg(command);
	msg.remove_prefix(1);
	msg.remove_suffix(1);

	std::vector<std::string_view> separatedMsg = SplitStringView(msg, " ");

	std::string response;


	if (separatedMsg[0] == "help")
	{
		response.append("Available commands:\n");
		response.append(m_commandCharacter + std::string("help - Displays this message\n"));
		response.append(m_commandCharacter + std::string("register <username> <password> - Registers a new user"));
		SendMessageToClient(comSocket, &response[0], response.size() + 1);
	}
	else if (separatedMsg[0] == "register")
	{
		if (separatedMsg.size() != 3)
		{
			response.append("Incorrect usage of register. Please use register <username> <password>");
			SendMessageToClient(comSocket, &response[0], response.size() + 1);
			return;
		}

		// Check if user is already registered
		auto it = m_registeredClients.find(std::string(separatedMsg[1]));
		if (it != m_registeredClients.end())
		{
			response.append("FAILED: This user has already been registered");
			SendMessageToClient(comSocket, &response[0], response.size() + 1);
			return;
		}

		// Check if capacity is already max
		if (m_currentCapacity >= m_chatCapacity)
		{
			response.append("DECLINE: There are currently max users.");
			SendMessageToClient(comSocket, &response[0], response.size() + 1);
			return;
		}

		// Add the credentials
		m_registeredClients[std::string(separatedMsg[1])] = std::string(separatedMsg[2]);
		response.append("SUCCESS: User successfully added");
		SendMessageToClient(comSocket, &response[0], response.size() + 1);
	}
}