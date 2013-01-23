// ***********************************************************
//
// GuidGenerator.cpp : 
//		Defines the exported functions for the generation library.
//
//		http://en.wikipedia.org/wiki/Globally_Unique_Identifier
//
//		This uses a hybrid style of guid generation based parts of V1 and V4:
//
//		128 bits L->R :=
//			V1:
//				( 60 ) temporal locality = unique ~100ns timestamp. 
//				( 4 ) multiplexor = unique temporal identifier, permits 1024 guis per 100ns tick.
//			Standard:			
//				( 16 ) clock hi = cpu ticks since reset ( standard )
//				( 4 ) version = "B" for author :)
//				( 12 ) clock low = remaining cpu ticks since reset
//			V4:
//				( 32 ) pseudo-random sequence = 8 x random unsigned shorts
//
// ***********************************************************

#include "stdafx.h"
#include "Generator.h"

#define DLL_EXPORT

// ----------[Exported Functions]----------

EXPORT GUID __stdcall Empty()
{
	return EmptyGuid;
}

EXPORT GUID __stdcall Generate()
{
	GUID generated;

	EnterCriticalSection( &CriticalSection );

	GenerateGuid( &generated );

	LeaveCriticalSection( &CriticalSection );

	return generated;
}

EXPORT GUID __stdcall FromStringA( const char * idString )
{
	wchar_t unicodeIdString[ 39 ];

	MultiByteToWideChar( CP_ACP, 0, idString, -1, unicodeIdString, 39 );

	return FromStringW( unicodeIdString );
}

EXPORT GUID __stdcall FromStringW( const wchar_t * idString )
{
	GUID id = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

	if( wcslen( idString ) == 36 || ( wcslen( idString ) == 38 && idString[ 0 ] == '{' && idString[ 37 ] == '}' ) )
	{
		wchar_t buffer[ 33 ], data1[ 9 ], data2[ 5 ], data3[ 5 ], data4[ 3 ];
		int index, offset = 16;

		PurifyIdSet( idString, buffer );
		
		GetCharacterSet( data1, buffer, 0, 8 );
		id.Data1 = ( long ) Base16Encode( data1 );

		GetCharacterSet( data2, buffer, 8, 4 );
		id.Data2 = ( unsigned short ) Base16Encode( data2 );

		GetCharacterSet( data3, buffer, 12, 4 );
		id.Data3 = ( unsigned short ) Base16Encode( data3 );

		for( index = 0; index < 8; index++ )
		{
			GetCharacterSet( data4, buffer, offset, 2 );

			id.Data4[ index ] = ( char ) Base16Encode( data4 );

			offset += 2;
		}
	}

	return id;
}

// ----------[Internal Functions]----------

static void PurifyIdSet( const wchar_t * characters, wchar_t * buffer )
{
	int index = 0;

	while( *characters )
	{
		if( *characters != '{' && *characters != '}' && *characters != '-' )
			buffer[ index++ ] = *characters;

		characters++;
	}

	buffer[ index ] = '\0';
}

static void GetCharacterSet( wchar_t * buffer, const wchar_t * characters, int start, int length )
{
	int index;

	for( index = 0; index < length; index++ )
	{
		buffer[ index ] = characters[ index + start ];
	}

	buffer[ length ] = '\0';
}

static unsigned long Base16Encode( const wchar_t * characters )
{
	unsigned long returnValue = 0;

	while( *characters )
	{
		const wchar_t * value = 
			wcschr( Base16EncodingTable, toupper( *characters ) );

		returnValue = returnValue << 4;

		if( value )
			returnValue = ( returnValue ^ ( value - Base16EncodingTable ) );

		characters++;
	}

	return returnValue;
}

void GetSystemTime( time_t *systemTime )
{
	ULARGE_INTEGER time;

	GetSystemTimeAsFileTime( ( LPFILETIME ) &time );

	time.QuadPart += nTimeSinceEpoch;

	*systemTime = time.QuadPart;
}

static time_t GetUniqueTimeStamp()
{
	time_t timeNow;

	static time_t timeLast;
	static unsigned short multiplexer;
	static bool initialized;

	if( !initialized )
	{
		nTimeSinceEpoch = nSecondsSinceEpoch * nDaysSinceEpoch * nYearsSinceEpoch;
		GetSystemTime( &timeLast );
		multiplexer = 0;
		initialized = true;
	}

	while( true )
	{
		GetSystemTime( &timeNow );

		if( timeLast != timeNow )
		{
			GetSystemTime( &timeLast );
			multiplexer = 0;
			break;
		}

		if( multiplexer < GUIDS_PER_TICK )
		{
			multiplexer++;
			break;
		}
	}

	return ( timeNow + multiplexer );
}

ClockSequence GetClockSequence()
{
	ClockSequence sequence;

	// ClockCount := EDX:EAX
	// Save AX, shift eax right 4 bytes to access the LSW with AH/AL
	// Save original AL pattern in CH
	// Set AH = Guid Version ( 8 bit pattern ), shift AX ( VVVV-VVVV-XXXX-XXXX ) left by 4 bits leaving EAX := VVVV-XXXX-XXXX-0000
	// AL |= CH ( Restore last 4 bits of original AX pattern ), AX := VVVV-XXXX-XXXX-XXXX
	// Restore AX, move EAX into sequence memory address space

	__asm
	{
		RdTSC

		push ax
		shr eax, 16

		mov ch, al
		mov ah, GuidVersion
		shl ax, 4
		mov al, ch

		shl eax, 16
		pop ax
		
		lea ebx, sequence
		mov [ebx], eax
	}
	
	return sequence;
}

static unsigned short GetPseudoRandom()
{
	static bool initialized = false;

	if( !initialized )
	{
		time_t currentTime = GetUniqueTimeStamp();
		unsigned int randomizerSeed = 0;

		// EAX:EBX := Unique Time Stamp ( ~100ns )
		// EAX = EAX / GUIDS_PER_TICK
		// EBX = ABS( EBX / 2 ) + 1
		// EAX += EBX ( psudo random ODD number )
		// EAX ^= EBX ( range reduction, quick testing produced ranges L[~XXX] - H[~XXXXXX] )
		// Seed = cx ( unsigned short 16 bit randomizer seed )

		__asm
		{
			mov eax, DWORD PTR [currentTime]
			mov ebx, DWORD PTR [currentTime + 4]
			shl eax, 10
			shr ebx, 1
			inc ebx
			add eax, ebx
			xor ebx, eax
			lea ecx, randomizerSeed
			mov [ecx], ax
		}

		srand( randomizerSeed );

		initialized = true;
	}

	return ( rand() );
}

void GenerateTimeStamp( unsigned long *timeStamp )
{
	*timeStamp = ( unsigned long ) GetUniqueTimeStamp();
}

void GenerateClockSequence( unsigned short *hiPart, unsigned short *lowPart )
{
	ClockSequence clockSequence = GetClockSequence();

	*hiPart = clockSequence.hPart;
	*lowPart = clockSequence.lPart;
}

void GenerateDataNode( unsigned char * node, int size )
{
	for( int index = 0; index < size; index++ )
	{
		*( node + index ) = ( unsigned char ) GetPseudoRandom();
	}
}

void GenerateGuid( GUID * guid )
{
	GenerateTimeStamp( &guid->Data1 );
	GenerateClockSequence( &guid->Data2, &guid->Data3 );
	GenerateDataNode( guid->Data4, 8 );
}