#include "BfCompiler.h"
#include <istream>
#include <iostream>
#include <cstdint>
#include <stack>

using namespace std;
using namespace bf;

//Dummy exception thrown on a mismatched brakets error
class MismatchedBraketsException {};

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

//Writes the prolog for the program
static void writeProlog(CompilerState& out)
{
    out.put(0x55);			// push ebp
    out.put(0x89, 0xE5);	// mov ebp, esp
    out.put(0x53);			// push ebx
    out.put(0xBB);			// mov ebx, <heap>
    out.putInt(reinterpret_cast<int32_t>(out.getHeap()));
}

//Writes the epilog for the program
static void writeEpilog(CompilerState& out)
{
    out.put(0x5B);          // pop ebx
    out.put(0x5D);          // pop ebp
    out.put(0xC3);          // ret
}

//Processes the given character
static void processChar(CompilerState& out, int c, uint32_t number = 1)
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
        out.loopStack().push(out.getPosition());
        out.putInt(0);
        break;

    case ']':
        {
            // Detect loop mismatch
            if(out.loopStack().empty())
                throw MismatchedBraketsException();

            // Get fixup position
            uint32_t fixupAddr = out.loopStack().top();
            out.loopStack().pop();

            // Fix address to jump to current location
            out.putRelativeAt(fixupAddr, out.getPosition());

            // Write loop end
            if(cellSize == 1)
                out.put(0x80, 0x3B, 0x00);        // cmp byte [ebx], 0
            else if(cellSize == 2)
                out.put(0x66, 0x83, 0x3B, 0x00);  // cmp word [ebx], 0
            else
                out.put(0x83, 0x3B, 0x00);        // cmp word [ebx], 0

            out.put(0x0F, 0x85);            // jnz near <location>
            out.putRelative(fixupAddr + 4);
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
        out.putRelative(outputCallback);
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
            out.putRelative(inputCallback);

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

//Type of data being buffered
#define BUFFER_NONE     0
#define BUFFER_VALUE    1
#define BUFFER_POINTER  2

//Processes the current buffered characters
void processBuffer(CompilerState& out, uint32_t& bufferType, int32_t& bufferNumber,
                   uint32_t newType = BUFFER_NONE, int32_t newNumber = 0)
{
    //Flush buffer?
    if(bufferType != BUFFER_NONE && bufferType != newType && bufferNumber != 0)
    {
        //Get correct character and number
        char properChar;
        uint32_t properNumber;

        if(bufferNumber >= 0)
        {
            properNumber = bufferNumber;
            properChar = bufferType == BUFFER_VALUE ? '+' : '>';
        }
        else
        {
            properNumber = -bufferNumber;
            properChar = bufferType == BUFFER_VALUE ? '-' : '<';
        }

        //Process it
        processChar(out, properChar, properNumber);

        //Reset bufferNumber
        bufferNumber = newNumber;
    }
    else
    {
        //Update bufferNumber
        bufferNumber += newNumber;
    }

    //Update bufferType
    bufferType = newType;
}

CompileResult bf::compile(std::istream& input, CompilerState& out)
{
    //Write prolog
    writeProlog(out);

    //Process input
    uint32_t bufferType = BUFFER_NONE;
    int32_t bufferNumber = 0;

    try
    {
        for(;;)
        {
            //Get next character
            int c = input.get();

            //Test for immediate failiures
            if(out.failed())
                return OUT_OF_OUTPUT_SPACE;
            else if(input.fail())
                return IO_ERROR;
            else if(input.eof())
                break;

            //What is it?
            switch(c)
            {
            case '+':
                processBuffer(out, bufferType, bufferNumber, BUFFER_VALUE, 1);
                break;

            case '-':
                processBuffer(out, bufferType, bufferNumber, BUFFER_VALUE, -1);
                break;

            case '>':
                processBuffer(out, bufferType, bufferNumber, BUFFER_POINTER, 1);
                break;

            case '<':
                processBuffer(out, bufferType, bufferNumber, BUFFER_POINTER, -1);
                break;

            case '[':
            case ']':
            case '.':
            case ',':
                //Ignore buffer and process directly
                processBuffer(out, bufferType, bufferNumber);
                processChar(out, c);
                break;
            }
        }
    }
    catch(MismatchedBraketsException)
    {
        //Mismatched brakets
        return MISMATCHED_BRAKETS;
    }

    //Since the buffer cannot hold any i/o or loop chars, it doesn't need processing

    //Ensure loop stack is empty
    if(!out.loopStack().empty())
        return MISMATCHED_BRAKETS;

    //Write epilog
    writeEpilog(out);
    return OK;
}
