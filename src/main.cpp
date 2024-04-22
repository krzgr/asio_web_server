#include "WebServer.hpp"

int main()
{
	WebServer w(8888);
	w.launch();

	return 0;
}