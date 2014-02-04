
#ifndef PROBE_H_INCLUDED
#define PROBE_H_INCLUDED

#include "position.h"

namespace EGBB {
	extern int probe_depth;
	bool load(const std::string& path,int cache_size);
	Value probe(const Position& pos, int ply, int fifty);
}

#endif // #ifndef PROBE_H_INCLUDED
