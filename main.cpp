#include <Windows.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include "BfCompiler.h"

// Compiler Options
#define CODE_SIZE (1024 * 1024)      // 1MB
#define HEAP_SIZE (1024 * 1024)      // 1MB
#define CELL_SIZE 1
#define EOF_CODE (bf::EofCode(-1))

// VirtualFree unique_ptr deleter
class VirtualFreeDeleter
{
public:
    void operator()(void * ptr)
    {
        if(ptr != NULL)
            ::VirtualFree(ptr, 0, MEM_RELEASE);
    }
};

typedef std::unique_ptr<void, VirtualFreeDeleter> VirtualAllocPtr;

// Private Functions
static void printHelp();
static bool parseArgs(int argc, char const ** argv, std::string& input, std::string& output);

int main(int argc, char const ** argv)
{
    std::string inputName, outputName;

    //Parse args
    if(!parseArgs(argc, argv, inputName, outputName))
    {
        printHelp();
        return 1;
    }

    //Open files
    std::ifstream inputFile;
    std::streambuf * inputFileBuf;
    std::ofstream output;

    if(inputName.empty())
    {
        //Using stdin
        inputFileBuf = std::cin.rdbuf();
    }
    else
    {
        //Using a file
        inputFile.open(inputName, std::ios::in);
        if(!inputFile)
        {
            std::cerr << "Failed to open input file: " << inputName << std::endl;
            return 1;
        }

        inputFileBuf = inputFile.rdbuf();
    }

    if(outputName.empty())
    {
        //Force output file to fail to write anything
        output.setstate(std::ios::badbit);
    }
    else
    {
        //Using a file
        output.open(outputName, std::ios::out | std::ios::trunc | std::ios::binary);
        if(!output)
        {
            std::cerr << "Failed to open output file: " << outputName << std::endl;
            return 1;
        }
    }

    //Create input
    std::istream input(inputFileBuf);

    //Allocate memory
    VirtualAllocPtr codePtr(::VirtualAlloc(NULL, CODE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    VirtualAllocPtr heapPtr(::VirtualAlloc(NULL, HEAP_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

    if(codePtr.get() == NULL || heapPtr.get() == NULL)
    {
        std::cerr << "Failed to allocate code and heap memory" << std::endl;
        return 1;
    }

    //Compile program
    bf::CompilerState state(codePtr.get(), CODE_SIZE, heapPtr.get(), CELL_SIZE, EOF_CODE);
    switch(bf::compile(input, state))
    {
    case bf::IO_ERROR:
        std::cerr << "Error reading input stream" << std::endl;
        return 1;

    case bf::OUT_OF_OUTPUT_SPACE:
        std::cerr << "Brainfuck file too large" << std::endl;
        return 1;

    case bf::MISMATCHED_BRAKETS:
        std::cerr << "Mismatched brakets" << std::endl;
        return 1;
    }

    //Write to output
    output.write(reinterpret_cast<char *>(codePtr.get()), state.getPosition());

    //Execute code
    reinterpret_cast<void (*)()>(codePtr.get())();
    return 0;
}

// Print program usage
static void printHelp()
{
    std::cerr << "Brainfuck Compiler - James Cowgill\n"
                 "\n"
                 "Usage:\n"
                 " bfc [-o <output>] [<input>]\n"
                 "\n"
                 "Compiles a Brainfuck program and runs it\n"
                 " <input>  = the file to read the program from\n"
                 "            if omitted, the program is read from stdin\n"
                 " <output> = if specified, the raw x86 code is written to the file <output>\n"
                 "            NOTE: the code is not executable as it contains hardcoded addresses";

    std::cerr << std::flush;
}

//Parses args and stores them in input and output
// Returns false to print help
static bool parseArgs(int argc, char const ** argv, std::string& input, std::string& output)
{
    bool nextIsOutput = false;

    //Clear output
    input.clear();
    output.clear();

    //Process args
    for(int i = 1; i < argc; i++)
    {
        char const * arg = argv[i];

        //Handle output option
        if(nextIsOutput)
        {
            output.assign(arg);
        }
        else if(std::strcmp(arg, "-o") == 0)
        {
            //Output already processed?
            if(!output.empty())
                return false;

            nextIsOutput = true;
            continue;
        }
        else if(!input.empty() ||
            std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "/?") == 0)
        {
            //Handle help option and too many inputs
            return false;
        }
        else
        {
            //Must be the input
            input.assign(arg);
        }

        nextIsOutput = false;
    }

    //Disallow dangling -o
    return !nextIsOutput;
}
