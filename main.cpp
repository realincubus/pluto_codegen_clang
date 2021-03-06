
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <sstream>
#include <map>
#include <iostream>

#include <cloog/cloog.h>

#include "version.h"

//#include "program.h"
//#include "ast_transform.h"
#include "pluto/libpluto.h"

extern "C"{
#include "program.h"
#include "constraints.h"
#include "math_support.h"
void pluto_mark_parallel(struct clast_stmt *root, const PlutoProg *prog, CloogOptions *options);
void pluto_mark_vector(struct clast_stmt *root, const PlutoProg *prog, CloogOptions *options);
int pluto_is_hyperplane_scalar(const Stmt *stmt, int level);
}

#include "pluto_codegen_clang.hpp"
#include "clast_clang.hpp"


using namespace std;

namespace pluto_codegen_clang{

static int get_first_point_loop(Stmt *stmt, const PlutoProg *prog)
{
    int i, first_point_loop;

    if (stmt->type != ORIG) {
        for (i=0; i<prog->num_hyperplanes; i++)   {
            if (!pluto_is_hyperplane_scalar(stmt, i)) {
                return i;
            }
        }
        /* No non-scalar hyperplanes */
        return 0;
    }

    for (i=stmt->last_tile_dim+1; i<stmt->trans->nrows; i++)   {
        if (stmt->hyp_types[i] == H_LOOP)  break;
    }

    if (i < prog->num_hyperplanes) {
        first_point_loop = i;
    }else{
        /* Should come here only if
         * it's a 0-d statement */
        first_point_loop = 0;
    }

    return first_point_loop;
}



/* Call cloog and generate code for the transformed program
 *
 * cloogf, cloogl: set to -1 if you want the function to decide
 *
 * --cloogf, --cloogl overrides everything; next cloogf, cloogl if != -1,
 *  then the function takes care of the rest
 */
int pluto_gen_cloog_code_clang(const PlutoProg *prog, int cloogf, int cloogl,
        stringstream& outfp, FILE* cloogfp, vector<std::string>& statement_texts, EMIT_CODE_TYPE emit_code_type )
{
    CloogInput *input ;
    CloogOptions *cloogOptions ;
    CloogState *state;
    int i;

    struct clast_stmt *root;

    Stmt **stmts = prog->stmts;
    int nstmts = prog->nstmts;

    state = cloog_state_malloc();
    cloogOptions = cloog_options_malloc(state);

    cloogOptions->fs = (int*)malloc (nstmts*sizeof(int));
    cloogOptions->ls = (int*)malloc(nstmts*sizeof(int));
    cloogOptions->fs_ls_size = nstmts;

    for (i=0; i<nstmts; i++) {
        cloogOptions->fs[i] = -1;
        cloogOptions->ls[i] = -1;
    }

    cloogOptions->name = "CLooG file produced by PLUTO";
    cloogOptions->compilable = 0;
    cloogOptions->esp = 1;
    cloogOptions->strides = 1;
    cloogOptions->quiet = options->silent;

    /* Generates better code in general */
    cloogOptions->backtrack = options->cloogbacktrack;

    if (options->cloogf >= 1 && options->cloogl >= 1) {
        cloogOptions->f = options->cloogf;
        cloogOptions->l = options->cloogl;
    }else{
        if (cloogf >= 1 && cloogl >= 1) {
            cloogOptions->f = cloogf;
            cloogOptions->l = cloogl;
        }else if (options->tile)   {
            for (i=0; i<nstmts; i++) {
                cloogOptions->fs[i] = get_first_point_loop(stmts[i], prog)+1;
                cloogOptions->ls[i] = prog->num_hyperplanes;
            }
        }else{
            /* Default */
            cloogOptions->f = 1;
            /* last level to optimize: number of hyperplanes;
             * since Pluto provides full-ranked transformations */
            cloogOptions->l = prog->num_hyperplanes;
        }
    }

    if (!options->silent)   {
        if (nstmts >= 1 && cloogOptions->fs[0] >= 1) {
            printf("[pluto] using statement-wise -fs/-ls options: ");
            for (i=0; i<nstmts; i++) {
                printf("S%d(%d,%d), ", i+1, cloogOptions->fs[i], 
                        cloogOptions->ls[i]);
            }
            printf("\n");
        }else{
            printf("[pluto] using Cloog -f/-l options: %d %d\n", 
                    cloogOptions->f, cloogOptions->l);
        }
    }

    if (options->cloogsh)
        cloogOptions->sh = 1;

    cloogOptions->name = "PLUTO-produced CLooG file";

    //outfp << "/* Start of CLooG code */" << endl;
    /* Get the code from CLooG */
    printf("[pluto] cloog_input_read\n");
    input = cloog_input_read(cloogfp, cloogOptions) ;
    printf("[pluto] cloog_clast_create\n");
    root = cloog_clast_create_from_input(input, cloogOptions);
#if 1
    if (options->prevector) {
        pluto_mark_vector(root, prog, cloogOptions);
    }
    if (options->parallel) {
        pluto_mark_parallel(root, prog, cloogOptions);
    }
#endif

    if ( emit_code_type == EMIT_ACC ) {
      clast_clang_acc::clast_pprint(outfp, root, 0, cloogOptions, statement_texts);
    }else if( emit_code_type == EMIT_OPENMP ){
      clast_clang_omp::clast_pprint(outfp, root, 0, cloogOptions, statement_texts);
    }else{
      std::cout << "not impemented" << std::endl;
      return 0;
    }
    cloog_clast_free(root);

    //fprintf(outfp, "/* End of CLooG code */\n");
    //outfp << "/* End of CLooG code */" << endl;

    cloog_options_free(cloogOptions);
    cloog_state_free(state);

    return 0;
}

static void gen_stmt_macro(const Stmt *stmt, stringstream& outfp)
{
    int j;

    for (j=0; j<stmt->dim; j++) { 
        if (stmt->iterators[j] == NULL) {
            printf("Iterator name not set for S%d; required \
                    for generating declarations\n", stmt->id+1);
            assert(0);
        }
    }
    outfp << "#define S"<< stmt->id+1;
    outfp << "(";
    for (j=0; j<stmt->dim; j++)  { 
        if (j!=0) {
	  outfp << ",";
	}
        outfp << stmt->iterators[j];
    }
    outfp << ")\t";

    outfp << stmt->text << endl;
}


/* Generate code for a single multicore; the ploog script will insert openmp
 * pragmas later */
int pluto_multicore_codegen( stringstream& outfp, 
    const PlutoProg *prog, 
    FILE* cloogfp, 
    vector<std::string>& statement_texts,
    EMIT_CODE_TYPE emit_code_type
    )
{ 

    if (options->multipar) {
      assert( 0 && "not tested" );
      //fprintf(outfp, "\tomp_set_nested(1);\n");
      outfp << "\tomp_set_nested(1);" << endl;
      //fprintf(outfp, "\tomp_set_num_threads(2);\n");
      outfp << "\tomp_set_num_threads(2);" << endl;
    }   

    std::cout << "pluto_codegen_clang ndeps " <<  prog->ndeps << std::endl;

    pluto_gen_cloog_code_clang(prog, -1, -1, outfp, cloogfp, statement_texts, emit_code_type );

    return 0;
}

}
