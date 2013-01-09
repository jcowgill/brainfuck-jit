#include "BfCompiler.h"
#include <fstream>
#include <iostream>
#include <Windows.h>

int main(void)
{
    //Test stuff
    std::ifstream input("input.bf", std::ios::in);
    std::ofstream output("output.raw", std::ios::out | std::ios::trunc | std::ios::binary);

    //Do the test compile
    char * rawMem = (char *) VirtualAlloc(NULL, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    char * heap = new char[0x10000]();

    bf::CompilerState state(rawMem, 0x10000, heap);
    bf::compile(input, state);

    //Write to output (until 10 consecutive nulls are found)
    output.write(rawMem, state.getPosition());

    //Execute code
    reinterpret_cast<void (*)()>(rawMem)();

    //Free memory
    delete [] heap;
    VirtualFree(rawMem, 0, MEM_RELEASE);
}
