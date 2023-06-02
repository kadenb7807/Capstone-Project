#include <Windows.h>
#include <iostream>
#include <array>
#include <string>

static char key[] = { 0x3B, 0x06, 0x11, 0x1E, 0x12, 0x16, 0x1C, 0x3E, 0x11, 0x1E, 0x13, 0x06, 0x0C, 0x16, 0x0C, 0x36, 0x0C, 0x30, 0x09, 0x1A, 0x0D, 0x0F, 0x10, 0x08, 0x1A, 0x0D, 0x1A, 0x1B, '\0' };

int main()
{
	std::printf("Press enter to continue then input your key.\n");

	std::getchar();

	for (auto i = 0u; i < sizeof(key) - 1; ++i)
		key[i] = 0x7f ^ key[i];

	std::string input;
	while (std::getline(std::cin, input))
	{
		if (input == key)
			break;
		else
			std::printf("Incorrect Key! Try again.\n");
	}

	std::printf("You got the correct key!\n");

	std::getchar();

	return 0;
}