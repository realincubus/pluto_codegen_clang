
#include "pluto.h"

#include <sstream>

namespace pluto_codegen_clang{
  int pluto_multicore_codegen( std::stringstream& outfp, const PlutoProg *prog, osl_scop_p scop);
}
