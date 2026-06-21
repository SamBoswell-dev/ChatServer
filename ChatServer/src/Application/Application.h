#pragma once
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

#include <vector>
#include <iostream>
#include <unordered_map>

class Application
{
public:
	void Init();
	void Run();

private:
	SOCKET m_listenSocket;
	std::vector<SOCKET> m_comSockets;
	uint16_t m_listenPort;
	uint32_t m_chatCapacity;
	char m_commandCharacter;

	std::unordered_map<std::string, std::string> m_registeredClients;
	uint32_t m_currentCapacity = 0;

private:
	bool SetupListenSocket();
	void SendMessageToClient(SOCKET comSocket, char* data, int32_t length);
	std::string ReadMessageFromClient(SOCKET comSocket);
	void HandleCommand(SOCKET comSocket, const std::string& command);
};