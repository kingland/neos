#include <neos/neos.hpp>
#include <iostream>

int main()
{
	std::cout << "neos " << neos::NEOS_VERSION << std::endl;
	for (;;)
	{
		std::cout << ">";
		std::string line;
		std::getline(std::cin, line);
	}
}
