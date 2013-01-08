#include "BfCompiler.h"
#include <fstream>

int main(void)
{
    //Test stuff
    std::ifstream input = std::ifstream("input.bf");
    std::ofstream output = std::ofstream("output.raw", std::ios::out | std::ios::trunc || std::ios::binary);

    //Do the test compile
    char * rawMem = new char[0x10000]();
    bf::compile(input, rawMem, (void *) 0xDEADBEEF);

    //Find end of compiled code
    int lastValidByte = 0xFFFF;
    for(; lastValidByte >= 0; lastValidByte--)
    {
        //Non 0?
        if(rawMem[lastValidByte] != 0)
            break;
    }

    //Write to output (until 10 consecutive nulls are found)
    output.write(rawMem, lastValidByte + 1);
}
