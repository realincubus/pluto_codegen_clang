
#include "pluto.h"

#include <sstream>
#include <vector>

namespace pluto_codegen_clang{
  int pluto_multicore_codegen( std::stringstream& outfp, const PlutoProg *prog, osl_scop_p scop, std::vector<std::string>& statement_texts );
}
