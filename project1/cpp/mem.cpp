#include <iostream>

int main(int argc, const char** argv)
{
	int memloc = 0;

    char mode;

	std::cin.read(&mode,1);

	while(mode != 2)
	{
		switch(mode)
		{
			case 0:
				std::cout.write((char*)&memloc,sizeof(int));
				break;
			case 1:
				std::cin.read((char*)&memloc,sizeof(int));
				break;
		}

	    std::cin.read(&mode,1);
	}

	return 0;
}
