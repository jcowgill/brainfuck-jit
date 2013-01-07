#ifndef _BFCOMPILER_H
#define _BFCOMPILER_H

// Brainfuck Compiler
//

#include <istream>
#include <cstdint>

namespace bf
{
	// Information about the type of code which represents an EOF
	struct EofCode
	{
		// True to return an eof code (false means that nothing is modified)
		bool modifyValue;

		// The code (only used if modifyValue is true)
		std::int32_t code;
	};

	// Compiles a brainfuck program to machine code
	//  input      = the input stream to read the program from
	//  outputCode = memory region to write code to
	//  heap       = address of heap to be used by the program
	//  cellSize   = size of each cell
	//  eofCode    = code to produce for eof situations
	void Compile(std::istream const& input, void * outputCode, void * heap, int cellSize, EofCode eofCode);

	// See above
	//  Defaults to cell size of 1, and an eof code of 0
	void Compile(std::istream const& input, void * outputCode, void * heap, int cellSize = 1);
}

#endif
