
#include <string>
#include <iostream>
#include <cstdio>

void scaleHkxAnimationDuration(std::string sourceXmlPath, std::string outputXmlPath, float durationScale){

}

int main(int argc, const char* args)
{
	printf("Hello\n");

	std::string sourceXmlPath = "C:/Projects/Dark Souls/Anim research/a00_3004.orig.hkx.xml";
	std::string outputXmlPath = "C:/Projects/Dark Souls/Anim research/a00_3004.hkx.xml";

	scaleHkxAnimationDuration(sourceXmlPath, outputXmlPath, 0.25f);

	int unused;
	std::cin >> unused;

    return 0;
}
