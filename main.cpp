#include <cstdio>
#include <iostream>

#include <wiringPi.h>
#include "LaserStream.h"

int main() {
	LaserStream laserStream;
	while (1) {
		std::cout << "> ";
		std::string answer;
		getline(std::cin, answer);
		if (answer == "/stop") {
			laserStream.close();
			return 0;
		}
		laserStream << answer;
	}

	return 0;
}