
#ifdef _MSC_VER
#    include <windows.h>
#    undef CDECL
#    define CDECL __cdecl
#else
#    include <dlfcn.h>
#    define CDECL
#endif

#include "probe.h"

enum egbb_colors {
	_WHITE,_BLACK
};
enum egbb_occupancy {
	_EMPTY,_WKING,_WQUEEN,_WROOK,_WBISHOP,_WKNIGHT,_WPAWN,
    _BKING,_BQUEEN,_BROOK,_BBISHOP,_BKNIGHT,_BPAWN
};
enum egbb_load_types {
	LOAD_NONE,LOAD_4MEN,SMART_LOAD,LOAD_5MEN
};

#define _NOTFOUND 99999
#define MAX_PIECES 6

typedef int (CDECL *PPROBE_EGBB) (int player, int* piece,int* square);
typedef void (CDECL *PLOAD_EGBB) (char* path,int cache_size,int load_options);
static PPROBE_EGBB probe_egbb;
static int is_loaded = false;
int EGBB::probe_depth = 0;

//Load EGBBs:
//Load the appropriate EGBB dll and get the address 
//of load/probe functions.
#ifdef _MSC_VER
#	ifdef IS_64BIT
#		define EGBB_NAME "egbbdll64.dll"
#	else
#		define EGBB_NAME "egbbdll.dll"
#	endif
#else
#	ifdef IS_64BIT
#		define EGBB_NAME "egbbso64.so"
#	else
#		define EGBB_NAME "egbbso.so"
#	endif
#endif

#ifndef _MSC_VER
#    define HMODULE void*
#    define LoadLibrary(x) dlopen(x,RTLD_LAZY)
#    define GetProcAddress dlsym
#endif

bool EGBB::load(const std::string& spath,int egbb_cache_size) {
	HMODULE hmod;
	PLOAD_EGBB load_egbb;
	char path[256];
	char terminator;
	char* main_path = (char*)spath.c_str();
	size_t plen = strlen(main_path);
	strcpy(path,main_path);
	if (plen) {
		terminator = main_path[strlen(main_path)-1];
		if (terminator != '/' && terminator != '\\') {
			if (strchr(path, '\\') != NULL)
				strcat(path, "\\");
			else
				strcat(path, "/");
		}
	}
	strcat(path,EGBB_NAME);
	if((hmod = LoadLibrary(path)) != 0) {
		load_egbb = (PLOAD_EGBB) GetProcAddress(hmod,"load_egbb_xmen");
     	probe_egbb = (PPROBE_EGBB) GetProcAddress(hmod,"probe_egbb_xmen");
        load_egbb(main_path,egbb_cache_size * 1024 * 1024,LOAD_4MEN);
		is_loaded = true;
		return true;
	}
	is_loaded = false;
	return false;
}

//Probe EGBBs:
//Change interanal Stockfish board representaion to 
//EGBB board representation and then probe
Value EGBB::probe(const Position& pos, int ply, int fifty) {
	int npieces = pos.count<ALL_PIECES>(WHITE) + pos.count<ALL_PIECES>(BLACK);

	if(is_loaded                                                   //must be loaded
		&& npieces <= MAX_PIECES                                   //maximum 6 pieces
		&& (ply >= probe_depth || (ply > 1 && fifty == 0))         //exceeded depth OR capture/pawn-push
		&& ( npieces <= 4 ||                                       //assume 4-pieces are in RAM
		     (ply <= probe_depth && npieces < MAX_PIECES) ||       //5-pieces probed whole search depth
		     (ply <= probe_depth / 2 && npieces == MAX_PIECES) )   //6-pieces only in the first half
		); 
	else
		return VALUE_NONE;

	Square s;
	int piece[8],square[8],count = 0;

#define ADD_PIECE(Piece,Col,PieceS) {			\
	const Square* pl = pos.list<Piece>(Col);	\
	while ((s = *pl++) != SQ_NONE) {			\
		piece[count] = PieceS;					\
		square[count] = s;						\
		count++;								\
	}											\
};
	ADD_PIECE(KING,   WHITE, _WKING);
	ADD_PIECE(KING,   BLACK, _BKING);
	ADD_PIECE(QUEEN,  WHITE, _WQUEEN);
	ADD_PIECE(QUEEN,  BLACK, _BQUEEN);
	ADD_PIECE(ROOK,   WHITE, _WROOK);
	ADD_PIECE(ROOK,   BLACK, _BROOK);
	ADD_PIECE(BISHOP, WHITE, _WBISHOP);
	ADD_PIECE(BISHOP, BLACK, _BBISHOP);
	ADD_PIECE(KNIGHT, WHITE, _WKNIGHT);
	ADD_PIECE(KNIGHT, BLACK, _BKNIGHT);
	ADD_PIECE(PAWN,   WHITE, _WPAWN);
	ADD_PIECE(PAWN,   BLACK, _BPAWN);
	piece[count] = _EMPTY;
	square[count] = pos.ep_square();

	int score = probe_egbb(
		(pos.side_to_move() == WHITE) ? _WHITE : _BLACK,
		piece,square);

	if(score == _NOTFOUND)
		return VALUE_NONE;

	Value value;
	if(score > 0)
		value =  VALUE_EGBB_WIN - VALUE_EGBB_PLY * ply + (score - 5000);
	else if(score < 0)
		value = -VALUE_EGBB_WIN + VALUE_EGBB_PLY * ply + (score + 5000);
	else
		value = VALUE_DRAW;

	return value;
}