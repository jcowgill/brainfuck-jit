#include "BfCompiler.h"
#include <istream>
#include <iostream>
#include <cstdint>
#include <stack>

using namespace std;
using namespace bf;

// Output buffer class
class OutBuffer
{
private:
    uint8_t * outPtr_;
    uint32_t cellSize_;
    EofCode eofCode_;
    stack<uint8_t *> loopStack_;

public:
    OutBuffer(uint8_t * outPtr, uint32_t cellSize, EofCode eofCode)
        : outPtr_(outPtr), eofCode_(eofCode)
    {
        //Validate cell size
        if(cellSize == 1 || cellSize == 2 || cellSize == 4)
            cellSize_ = cellSize;
        else
            cellSize_ = 1;
    }

    // Gets the current output address
    uint8_t * getCurrentAddress() const
    {
        return outPtr_;
    }

    // Gets the cell size in use
    int getCellSize() const
    {
        return cellSize_;
    }

    // Gets the EOF Code in use
    EofCode const& getEofCode() const
    {
        return eofCode_;
    }

    // Pushes the current address on the loop stack
    void pushAddress()
    {
        loopStack_.push(outPtr_);
    }

    // Pops an address off the loop stack
    //  Returns NULL if there's an empty stack
    uint8_t * popAddress()
    {
        if(loopStack_.empty())
        {
            return NULL;
        }
        else
        {
            uint8_t * retVal = loopStack_.top();
            loopStack_.pop();
            return retVal;
        }
    }

    // Byte putters
    void put(uint8_t b1)
    {
        *outPtr_++ = b1;
    }

    void put(uint8_t b1, uint8_t b2)
    {
        put(b1);
        put(b2);
    }

    void put(uint8_t b1, uint8_t b2, uint8_t b3)
    {
        put(b1, b2);
        put(b3);
    }

    void put(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
    {
        put(b1, b2);
        put(b3, b4);
    }

    // Number putters
    static void putShort(uint8_t * ptr, int16_t number)
    {
        ptr[0] = static_cast<uint8_t>(number);
        ptr[1] = static_cast<uint8_t>(number >> 8);
    }

    static void putInt(uint8_t * ptr, int32_t number)
    {
        ptr[0] = static_cast<uint8_t>(number);
        ptr[1] = static_cast<uint8_t>(number >> 8);
        ptr[2] = static_cast<uint8_t>(number >> 16);
        ptr[3] = static_cast<uint8_t>(number >> 24);
    }

    void putShort(int16_t number)
    {
        putShort(outPtr_, number);
        outPtr_ += 2;
    }

    void putInt(int32_t number)
    {
        putInt(outPtr_, number);
        outPtr_ += 4;
    }
};

//Callback functions used in code
static void __fastcall outputCallback(int c)
{
    cout.put(static_cast<char>(c));
}

static int __fastcall inputCallback(int eofCode)
{
    int result = cin.get();

    //Patch eofCode
    if(cin.eof())
        result = eofCode;

    return result;
}

//Returns true if the given character is bufferable
static bool bufferable(int c)
{
    return c == '+' || c == '-' || c == '<' || c == '>';
}

//Writes the prolog for the program
static void writeProlog(OutBuffer& out, void * heap)
{
    out.put(0x55);			// push ebp
    out.put(0x89, 0xE5);	// mov ebp, esp
    out.put(0x53);			// push ebx
    out.put(0xBB);			// mov ebx, <heap>
    out.putInt(reinterpret_cast<int32_t>(heap));
}

//Writes the epilog for the program
static void writeEpilog(OutBuffer& out)
{
    out.put(0x5B);          // pop ebx
    out.put(0x5D);          // pop ebp
    out.put(0xC3);          // ret
}

//Processes the given character
static void processChar(OutBuffer& out, int c, uint32_t number = 1)
{
    uint32_t cellSize = out.getCellSize();
    uint32_t byteInc = number * cellSize;

    //Ignore "no times"
    if(number == 0)
        return;

    //What is it?
    switch(c)
    {
    case '+':
        //Only 1?
        if(number == 1)
        {
            //Insert an increment
            if(cellSize == 1)
                out.put(0xFE, 0x03);        // inc byte [ebx]
            else if(cellSize == 2)
                out.put(0x66, 0xFF, 0x03);  // inc word [ebx]
            else
                out.put(0xFF, 0x03);        // inc dword [ebx]
        }
        else
        {
            //Insert an add
            if(cellSize == 1)
            {
                out.put(0x80, 0x03);        // add byte [ebx], <number>
                out.put(static_cast<char>(number));
            }
            else if(cellSize == 2)
            {
                out.put(0x66, 0x81, 0x03);  // add word [ebx], <number>
                out.putShort(static_cast<int16_t>(number));
            }
            else
            {
                out.put(0x81, 0x03);        // add dword [ebx], <number>
                out.putInt(number);
            }
        }

        break;

    case '-':
        //Only 1?
        if(number == 1)
        {
            //Insert an decrement
            if(cellSize == 1)
                out.put(0xFE, 0x0B);        // dec byte [ebx]
            else if(cellSize == 2)
                out.put(0x66, 0xFF, 0x0B);  // dec word [ebx]
            else
                out.put(0xFF, 0x0B);        // dec dword [ebx]
        }
        else
        {
            //Insert a sub
            if(cellSize == 1)
            {
                out.put(0x80, 0x2B);        // sub byte [ebx], <number>
                out.put(static_cast<char>(number));
            }
            else if(cellSize == 2)
            {
                out.put(0x66, 0x81, 0x2B);  // sub word [ebx], <number>
                out.putShort(static_cast<int16_t>(number));
            }
            else
            {
                out.put(0x81, 0x2B);        // sub dword [ebx], <number>
                out.putInt(number);
            }
        }

        break;

    case '>':
        //Add correct amount
        if(byteInc == 1)
        {
            out.put(0x43);              // inc ebx
        }
        else if(byteInc <= 0x7F)        // Sign-Extended
        {
            out.put(0x83, 0xC3);        // add ebx, byte <byteInc>
            out.put(static_cast<char>(byteInc));
        }
        else
        {
            out.put(0x81, 0xC3);        // add ebx, dword <byteInc>
            out.putInt(byteInc);
        }

        break;

    case '<':
        //Subtract correct amount
        if(byteInc == 1)
        {
            out.put(0x4B);              // dec ebx
        }
        else if(byteInc <= 0x7F)        // Sign-Extended
        {
            out.put(0x83, 0xEB);        // sub ebx, byte <byteInc>
            out.put(static_cast<char>(byteInc));
        }
        else
        {
            out.put(0x81, 0xEB);        // sub ebx, dword <byteInc>
            out.putInt(byteInc);
        }

        break;

    case '[':
        // Emit jump and store loop start location on the stack
        out.put(0xE9);                  // jmp near <location>
        out.pushAddress();
        out.putInt(0);
        break;

    case ']':
        {
            // Get fixup address
            uint8_t * fixupAddr = out.popAddress();

            // Ignore loop mismatch
#pragma message("Do we want to do this?")
            if(fixupAddr == NULL)
                break;

            // Fix address to jump to current location
            OutBuffer::putInt(fixupAddr, out.getCurrentAddress() - fixupAddr);

            // Write loop end
            if(cellSize == 1)
                out.put(0x80, 0x3B, 0x00);        // cmp byte [ebx], 0
            else if(cellSize == 2)
                out.put(0x66, 0x83, 0x3B, 0x00);  // cmp word [ebx], 0
            else
                out.put(0x83, 0x3B, 0x00);        // cmp word [ebx], 0

            out.put(0x0F, 0x85);            // jnz near <location>
            out.putInt(out.getCurrentAddress() - (fixupAddr + 4));
            break;
        }

    case '.':
        // Store character to display
        if(cellSize == 1)
            out.put(0x0F, 0xB6, 0x0B);  // movzx ecx, byte [ebx]
        else if(cellSize == 2)
            out.put(0x0F, 0xB7, 0x0B);  // movzx ecx, word [ebx]
        else
            out.put(0x8B, 0x0B);        // mov ecx, [ebx]

        // Call outputCallback
        out.put(0xE8);                  // call near <location>
        out.putInt(out.getCurrentAddress() - reinterpret_cast<uint8_t *>(outputCallback));
        break;

    case ',':
        {
            // Get eof character
            EofCode eofCode = out.getEofCode();
            int32_t codeToUse;

            if(eofCode.modifyValue)
                codeToUse = eofCode.code;
            else
                codeToUse = -1;

            // Store it in ecx
            out.put(0xB9);                  // mov ecx, <number>
            out.putInt(codeToUse);

            // Call inputCallback
            out.put(0xE8);                  // call near <location>
            out.putInt(out.getCurrentAddress() - reinterpret_cast<uint8_t *>(inputCallback));

            // Skip store if we're using ignore EOF
            if(!eofCode.modifyValue)
            {
                out.put(0x83, 0xF8, 0xFF);  // cmp eax, -1

                if(cellSize == 2)
                    out.put(0x74, 0x03);    // je +3
                else
                    out.put(0x74, 0x02);    // je +2
            }

            // Store result
            if(cellSize == 1)
                out.put(0x88, 0x03);        // mov [ebx], al
            else if(cellSize == 2)
                out.put(0x66, 0x89, 0x03);  // mov [ebx], ax
            else
                out.put(0x89, 0x03);        // mov [ebx], eax

            break;
        }
    }
}

void bf::compile(istream& input, void * outputCode, void * heap, uint32_t cellSize, EofCode eofCode)
{
    //Create buffer and write prolog
    OutBuffer out = OutBuffer(static_cast<uint8_t *>(outputCode), cellSize, eofCode);
    writeProlog(out, heap);

    //Process input
    int bufferedChar = -1;
    uint32_t bufferedTimes = 0;

    for(;;)
    {
        //Get next character
        int c = input.get();

        if(!input)
            break;

        //Same as buffered char?
        if(c == bufferedChar)
        {
            //Add 1 to occurences
            bufferedTimes++;
        }
        else
        {
            //Process buffered character
            processChar(out, bufferedChar, bufferedTimes);

            //Is this character bufferable?
            if(bufferable(c))
            {
                //Buffer it
                bufferedTimes = 1;
                bufferedChar = c;
            }
            else
            {
                //Erase buffer and process now
                bufferedTimes = 0;
                bufferedChar = -1;
                processChar(out, c);
            }
        }
    }

    //Since the buffer cannot hold any i/o or loop chars, it doesn't need processing

    //Write epilog
    writeEpilog(out);
}

void bf::compile(istream& input, void * outputCode, void * heap, uint32_t cellSize /* = 1 */)
{
    //Forward to other function
    EofCode zeroCode = { true, 0 };
    compile(input, outputCode, heap, cellSize, zeroCode);
}
