#include "Application/Application.h"

int main() 
{
	WSADATA wsadata;
	WSAStartup(WINSOCK_VERSION, &wsadata);

	Application app;
	app.Init();
	app.Run();

	WSACleanup();

	return EXIT_SUCCESS;
}