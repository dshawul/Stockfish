
#ifndef PROBE_H_INCLUDED
#define PROBE_H_INCLUDED

#include "position.h"

namespace EGBB {
	extern int ply_limit;
	extern int depth_limit;
	extern int tb_hits;
	bool load(const std::string& path,int cache_size,int egbb_load_type);
	Value probe(const Position& pos, int ply, int depth, int fifty);
}

#endif // #ifndef PROBE_H_INCLUDED
