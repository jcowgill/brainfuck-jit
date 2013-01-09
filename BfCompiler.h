#ifndef _BFCOMPILER_H
#define _BFCOMPILER_H

// Brainfuck Compiler
//

#include <istream>
#include <cstdint>
#include <stack>

namespace bf
{
    // Information about the type of code which represents an EOF
    struct EofCode
    {
        // True to return an eof code (false means that nothing is modified)
        bool modifyValue;

        // The code (only used if modifyValue is true)
        std::int32_t code;

        // Creates a new EofCode which DOES NOT produce a value
        EofCode()
            : modifyValue(false), code(0)
        {
        }

        // Creates a new EofCode using the given code
        EofCode(std::int32_t code)
            : modifyValue(true), code(code)
        {
        }
    };

    // Provides the INPUT for the compiler
    class CompilerState
    {
    private:
        // Compiler results and options
        std::uint8_t * output_;
        std::uint32_t outputSize_;
        void * heap_;
        std::uint8_t cellSize_;
        EofCode eofCode_;

        // Current position
        std::uint32_t pos_;
        bool failed_;

        // Loop stack
        std::stack<std::uint32_t> loopStack_;

    public:
        // Creates a new compiler state with the given options
        //  output       = Memory location to store code at
        //  outputSize   = Size of output
        //  heap         = Pointer to the heap memory
        //  cellSize     = Size of cells to use (must be 1, 2 or 4)
        //  eofCode      = What code to produce on EOF (see bf::EofCode)
        CompilerState(void * output, std::uint32_t outputSize, void * heap,
            std::uint8_t cellSize = 1, EofCode eofCode = EofCode(-1));

        // Gets the heap address
        void * getHeap() const;

        // Gets the current output address
        std::uint32_t getPosition() const;

        // Returns true if there was not enough bytes to write everything
        bool failed() const;

        // Gets the cell size in use
        unsigned char getCellSize() const;

        // Gets the EOF Code in use
        EofCode const& getEofCode() const;

        // Gets the loop stack (containing positions)
        std::stack<std::uint32_t>& loopStack();
        std::stack<std::uint32_t> const& loopStack() const;

        // Byte putters
        void put(std::uint8_t b1);
        void put(std::uint8_t b1, std::uint8_t b2);
        void put(std::uint8_t b1, std::uint8_t b2, std::uint8_t b3);
        void put(std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4);

        // Number putters
        void putShort(std::uint16_t number);
        void putInt(std::uint32_t number);

        // Relative position putter
        void putRelative(std::uint32_t relPosition);
        void putRelative(void * rawPointer);

        // Put at (can only be used to put at PREVIOUS positions)
        void putAt(std::uint32_t position, std::uint8_t number);
        void putShortAt(std::uint32_t position, std::uint16_t number);
        void putIntAt(std::uint32_t position, std::uint32_t number);
        void putRelativeAt(std::uint32_t position, std::uint32_t relPosition);
        void putRelativeAt(std::uint32_t position, void * rawPointer);
    };

    // The result of the compilation process
    enum CompileResult
    {
        OK,                     // Compiled OK
        IO_ERROR,               // I/O error on the input
        OUT_OF_OUTPUT_SPACE,    // Not enough space in output buffer
        MISMATCHED_BRAKETS,     // Mismatched brakets
    };

    // Compiles a brainfuck program to machine code
    CompileResult compile(std::istream& input, CompilerState& state);
}

#endif
