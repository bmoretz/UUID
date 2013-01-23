// ***********************************************************
//
// Generator.h : 
//		Defines the exported function prototypes, constants and utility macros.
//
// ***********************************************************

#ifndef _GENERATOR_DLL_H_

	#define _GENERATOR_DLL_H_

	#if defined DLL_EXPORT
		#define EXPORT extern "C" __declspec( dllexport )
	#else
		#define EXPORT extern "C" __declspec( dllexport )
	#endif

	// ----------[Export Prototypes]----------

	EXPORT GUID __stdcall Empty();
	EXPORT GUID __stdcall Generate();
	EXPORT GUID __stdcall FromStringA( const char * );
	EXPORT GUID __stdcall FromStringW( const wchar_t * );

	// ----------[Definitions]----------

	#define GUIDS_PER_TICK 1024
	
	typedef struct
	{
		unsigned short hPart, lPart;
	} ClockSequence;

	// ----------[Constants]----------

	const 	GUID 				EmptyGuid = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

	const wchar_t *				Base16EncodingTable = L"0123456789ABCDEF";

	// UUID / GUID time uses

	const 	unsigned __int64 	nSecondsSinceEpoch = 10000000;
	const 	unsigned __int64 	nDaysSinceEpoch = 96400;
	const 	unsigned __int64 	nYearsSinceEpoch = 6653;

	// ----------[Globals]----------

	static 	unsigned __int64 	nTimeSinceEpoch = 0;
	static 	unsigned char 		GuidVersion = 0xB; // GUID Version, Note: Only the right 4 bits will be used.

	// ----------[Internal Prototypes]----------

	void GenerateGuid( GUID * );
	void GetCharacterSet( wchar_t *, const wchar_t *, int, int );
	void PurifyIdSet( const wchar_t *, wchar_t * );
	
	unsigned long Base16Encode( const wchar_t * );

#endif