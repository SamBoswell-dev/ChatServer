#pragma once
#include <winsock2.h>
#include <iostream>

inline int SendTcpData(SOCKET skSocket, const char* data, uint16_t length)
{
	int result;
	int bytesSent = 0;

	while (bytesSent < length)
	{
		result = send(skSocket, (const char*)data + bytesSent, length - bytesSent, 0);

		if (result <= 0)
			return result;

		bytesSent += result;
	}

	return bytesSent;
}

inline int RecvTcpData(SOCKET s, char* buf, int len)
{
	int total = 0;

	while (total < len)
	{
		int amountRead = recv(s, buf + total, len - total, 0);
		if (amountRead <= 0)
			return amountRead;

		total += amountRead;
	}

	return total;
}