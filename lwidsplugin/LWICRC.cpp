
#include "LWICRC.h"
#include <time.h>

LWICRC* LWICRC::_instance = NULL;

void
LWICRC::Initialize()
{
    if (_instance != NULL)
    {
        delete _instance;
        _instance = NULL;
    }
    _instance = new LWICRC();
}

void
LWICRC::Cleanup()
{
    if (_instance)
    {
       delete _instance;
    }
    
    _instance = NULL;
}

unsigned long
LWICRC::GetCRC(
    char* pByteArray,
    int   length)
{
    assert(_instance != NULL);
    
    return _instance->CalculateCRC(pByteArray, length);
}

LWICRC::LWICRC()
: _key(time(NULL))
{
    InitTable();
}

LWICRC::LWICRC(unsigned long key)
: _key (key)
{
    InitTable();
}

void
LWICRC::InitTable()
{
    assert (_key != 0);
    
	// for all possible byte values
	for (unsigned i = 0; i < 256; ++i)
	{
		unsigned long reg = i << 24;
		// for all bits in a byte
		for (int j = 0; j < 8; ++j)
		{
			bool topBit = (reg & 0x80000000) != 0;
			reg <<= 1;
			if (topBit)
            {
				reg ^= _key;
            }
		}
		_table [i] = reg;
	}
}

unsigned long
LWICRC::CalculateCRC(char* pByteArray, int length) const
{
    unsigned long reg = 0;
    
    for (int i = 0; i < length; i++)
    {
        unsigned top = reg >> 24;
        top ^= *(pByteArray+i);
        reg = (reg << 8) ^ _table [top];
    }
    
    return reg;
}
