
#include "pluto.h"

#include <sstream>
#include <vector>

namespace pluto_codegen_clang{

  enum EMIT_CODE_TYPE{
      EMIT_ACC,
      EMIT_OPENMP,
      EMIT_HPX,
      EMIT_LIST_END
  };

  int pluto_multicore_codegen( std::stringstream& outfp, 
      const PlutoProg *prog, 
      FILE* cloogfp, 
      std::vector<std::string>& statement_texts,
      EMIT_CODE_TYPE emit_code_type
      );


}
