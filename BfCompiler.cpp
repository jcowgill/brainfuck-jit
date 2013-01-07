#include "BfCompiler.h"
#include <istream>

void bf::Compile(std::istream const& input, void * outputCode, void * heap, int cellSize, EofCode eofCode)
{
}

void bf::Compile(std::istream const& input, void * outputCode, void * heap, int cellSize /* = 1 */)
{
	//Forward to other function
	EofCode zeroCode = { true, 0 };
	bf::Compile(input, outputCode, heap, cellSize, zeroCode);
}
