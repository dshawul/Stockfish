
#ifndef PROBE_H_INCLUDED
#define PROBE_H_INCLUDED

#include "position.h"

namespace EGBB {
	extern int probe_depth;
	extern int tb_hits;
	bool load(const std::string& path,int cache_size,int egbb_load_type);
	Value probe(const Position& pos, int ply, int fifty);
}

#endif // #ifndef PROBE_H_INCLUDED
