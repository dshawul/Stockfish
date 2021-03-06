
#include <cstring>
#include "probe.h"

#ifdef _WIN32   // Windows and MinGW
#    undef CDECL
#    define CDECL __cdecl
#else     // Linux - Unix
#	 define dlsym __shut_up
#    include <dlfcn.h>
#	 undef dlsym
extern "C" void *(*dlsym(void *handle, const char *symbol))();
#    define CDECL
#endif

//EGBB piece and color types
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

#define _NOTFOUND   99999

//Function pointers to probe/load EGBBs
namespace EGBB {
	typedef int (CDECL *PPROBE_EGBB) (int player, int* piece,int* square);
	typedef void (CDECL *PLOAD_EGBB) (const char* path,int cache_size,int load_options);

	PPROBE_EGBB probe_egbb;
	bool is_loaded = false;
	int ply_limit = 0;
	int depth_limit = 6;
	int tb_hits = 0;
	const int MAX_PIECES = 6;
}

//Load EGBBs:
//Load the appropriate EGBB dll and get the address 
//of load/probe functions.
#ifdef _WIN32
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

#ifndef _WIN32
#    define HMODULE void*
#    define LoadLibrary(x) dlopen(x,RTLD_LAZY)
#    define FreeLibrary(x) dlclose(x)
#    define GetProcAddress dlsym
#endif

bool EGBB::load(const std::string& spath,int cache_size,int egbb_load_type) {
	static HMODULE hmod = 0;
	PLOAD_EGBB load_egbb;
	char path[256];

	strcpy(path,spath.c_str());
	size_t plen = strlen(path);
	if (plen) {
		char terminator = path[plen - 1];
		if (terminator != '/' && terminator != '\\') {
			if (strchr(path, '\\') != NULL)
				strcat(path, "\\");
			else
				strcat(path, "/");
		}
	}
	plen = strlen(path);
	strcat(path,EGBB_NAME);

	if(hmod) FreeLibrary(hmod);
	if((hmod = LoadLibrary(path)) != 0) {
		load_egbb = (PLOAD_EGBB) GetProcAddress(hmod,"load_egbb_xmen");
		probe_egbb = (PPROBE_EGBB) GetProcAddress(hmod,"probe_egbb_xmen");
		path[plen] = 0;
        load_egbb(path,cache_size * 1024 * 1024,egbb_load_type);
		is_loaded = true;
		return true;
	}
	is_loaded = false;
	return false;
}

//Probe EGBBs:
//Change interanal Stockfish board representaion to 
//EGBB board representation and then probe
Value EGBB::probe(const Position& pos, int ply, int depth, int fifty) {
	int npieces = pos.count<ALL_PIECES>(WHITE) + pos.count<ALL_PIECES>(BLACK);
	int dlimit = depth_limit * ONE_PLY;
	int plimit = (npieces < MAX_PIECES) ? ply_limit : ply_limit / 2;                   
	if(is_loaded && (ply > 1)                //must be loaded
		&& npieces <= MAX_PIECES             //maximum 6 pieces
		&& (depth >= dlimit || npieces <= 4) //hard depth limit
		&& (ply >= plimit || fifty == 0)     //ply above threshold or capt/pawn move
		); 
	else
		return VALUE_NONE;

	//Loop over pieces and fill up piece and square arrays
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

	//probe and return adjusted value
	int score = probe_egbb(
		(pos.side_to_move() == WHITE) ? _WHITE : _BLACK,
		piece,square);

	if(score == _NOTFOUND)
		return VALUE_NONE;
	else {
		tb_hits++;
		if(score > 0)
			return  VALUE_EGBB_WIN - VALUE_EGBB_PLY * ply + (score - 5000);
		else if(score < 0)
			return -VALUE_EGBB_WIN + VALUE_EGBB_PLY * ply + (score + 5000);
		else
			return VALUE_DRAW;
	}
}
