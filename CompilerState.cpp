#include "BfCompiler.h"

// CompilerState helper class
//

bf::CompilerState::CompilerState(void * output, std::size_t outputSize, void * heap,
    unsigned char cellSize, EofCode eofCode)
    : output_(reinterpret_cast<uint8_t *>(output)), outputSize_(outputSize),
        heap_(heap), cellSize_(cellSize), eofCode_(eofCode), pos_(0), failed_(false)
{
}

void * bf::CompilerState::getHeap() const
{
    return heap_;
}

std::uint32_t bf::CompilerState::getPosition() const
{
    return pos_;
}

bool bf::CompilerState::failed() const
{
    return failed_;
}

std::uint8_t bf::CompilerState::getCellSize() const
{
    return cellSize_;
}

bf::EofCode const& bf::CompilerState::getEofCode() const
{
    return eofCode_;
}

std::stack<std::uint32_t>& bf::CompilerState::loopStack()
{
    return loopStack_;
}

std::stack<std::uint32_t> const& bf::CompilerState::loopStack() const
{
    return loopStack_;
}

void bf::CompilerState::put(std::uint8_t b1)
{
    //Fail if at the end
    if(pos_ >= outputSize_)
        failed_ = true;
    else
        output_[pos_++] = b1;
}

void bf::CompilerState::put(std::uint8_t b1, std::uint8_t b2)
{
    put(b1);
    put(b2);
}

void bf::CompilerState::put(std::uint8_t b1, std::uint8_t b2, std::uint8_t b3)
{
    put(b1, b2);
    put(b3);
}

void bf::CompilerState::put(std::uint8_t b1, std::uint8_t b2, std::uint8_t b3, std::uint8_t b4)
{
    put(b1, b2);
    put(b3, b4);
}

// Number putters
void bf::CompilerState::putShort(std::uint16_t number)
{
    put(static_cast<std::uint8_t>(number),
        static_cast<std::uint8_t>(number >> 8));
}

void bf::CompilerState::putInt(std::uint32_t number)
{
    put(static_cast<std::uint8_t>(number),
        static_cast<std::uint8_t>(number >> 8),
        static_cast<std::uint8_t>(number >> 16),
        static_cast<std::uint8_t>(number >> 24));
}

// Relative position putter
void bf::CompilerState::putRelative(std::uint32_t relPosition)
{
    putInt(relPosition - pos_ - 4);
}

void bf::CompilerState::putRelative(void * rawPointer)
{
    putRelative(reinterpret_cast<uint8_t *>(rawPointer) - output_);
}

// Put at (can only be used to put at PREVIOUS positions)
void bf::CompilerState::putAt(std::uint32_t position, std::uint8_t number)
{
    output_[position] = number;
}

void bf::CompilerState::putShortAt(std::uint32_t position, std::uint16_t number)
{
    output_[position]     = static_cast<uint8_t>(number);
    output_[position + 1] = static_cast<uint8_t>(number >> 8);
}

void bf::CompilerState::putIntAt(std::uint32_t position, std::uint32_t number)
{
    output_[position]     = static_cast<uint8_t>(number);
    output_[position + 1] = static_cast<uint8_t>(number >> 8);
    output_[position + 2] = static_cast<uint8_t>(number >> 16);
    output_[position + 3] = static_cast<uint8_t>(number >> 24);
}

void bf::CompilerState::putRelativeAt(std::uint32_t position, std::uint32_t relPosition)
{
    putIntAt(position, relPosition - position - 4);
}

void bf::CompilerState::putRelativeAt(std::uint32_t position, void * rawPointer)
{
    putRelativeAt(position, reinterpret_cast<uint8_t *>(rawPointer) - output_);
}
