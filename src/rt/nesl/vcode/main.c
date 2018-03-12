/*
* Copyright (c) 1992, 1993, 1994, 1995
* Carnegie Mellon University SCAL project:
* Guy Blelloch, Jonathan Hardwick, Jay Sipelstein, Marco Zagha
*
* All Rights Reserved.
*
* Permission to use, copy, modify and distribute this software and its
* documentation is hereby granted, provided that both the copyright
* notice and this permission notice appear in all copies of the
* software, derivative works or modified versions, and any portions
* thereof.
*
* CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
* CONDITION.  CARNEGIE MELLON AND THE SCAL PROJECT DISCLAIMS ANY 
* LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM 
* THE USE OF THIS SOFTWARE.
*
* The SCANDAL project requests users of this software to return to 
*
*  Guy Blelloch				nesl-contribute@cs.cmu.edu
*  School of Computer Science
*  Carnegie Mellon University
*  5000 Forbes Ave.
*  Pittsburgh PA 15213-3890
*
* any improvements or extensions that they make and grant Carnegie Mellon
* the rights to redistribute these changes.
*/

#include "config.h"
#include "vcode.h"
#include "y.tab.h"
#include "symbol_table.h"
#include "link_list.h"
#include "parse.h"
#include "vstack.h"
#include "program.h"
#include "stack.h"
#include "check_args.h"
#include "rtstack.h"
#include "constant.h"
#include "io.h"





char *nk_nesl_current_parse_ptr=0;

static void main_loop PROTO_((void));
static void initialize PROTO_((void));
static int init_pc PROTO_((void));

int lex_trace = 0;
int stack_trace = 0;
int command_trace = 1;
int value_trace = 0;
int runtime_trace = 0;
int program_dump = 1;
int link_trace = 0;
int timer = 0;
unsigned int vstack_size_init = 1000000;
int garbage_notify = 1;
int check_args = 0;
int debug_flag = 1;
int heap_trace = 0;
int abort_on_error = 0;

static void initialize_parse PROTO_((void))
{
	hash_table_init();
	link_list_init();
	prog_init();
}

static void deinitialize_parse PROTO_((void))
{
	prog_deinit();
	link_list_deinit();
	hash_table_deinit();
}

void CVL_init();

static int nesl_exec(void *vcode_blob)
{

    DEBUG("init for program at %p\n",vcode_blob);

    CVL_init();
    
    //    init_file_list()

    DEBUG("NESL: init parse\n");

    nk_nesl_current_parse_ptr = vcode_blob;
    
    initialize_parse();

    DEBUG("parsing\n");

    /* parse and link the VCODE routine */
    if (yyparse() || parse_error) {
	ERROR("Parse failure\n");
	vinterp_exit (1);
    }

    DEBUG("parsing succeeded, now linking\n");
    
    if (do_link()) {
	ERROR("Link failure\n");
	vinterp_exit (1);
    }

    DEBUG("program dump\n");

    if (program_dump)
	show_program();

    
    DEBUG("initialize\n");
  

    initialize();


    DEBUG("main loop\n");

    main_loop();

    DEBUG("main loop done\n");

    return 0;
}




static char *programstr =
"\n"
"{ Generated by the NESL Compiler, Version 3.1.1, on 11/25/2014 17:11. }\n"
"\n"
"FUNC MAIN1_1\n"
"CONST INT 1\n"
"CONST INT 1\n"
"CALL SEQ_DIST_4\n"
"CONST INT 2\n"
"CALL MAKE_SEQUENCE_7\n"
"CONST INT 3\n"
"CALL MAKE_SEQUENCE_7\n"
"COPY 1 1\n"
"CPOP 1 2\n"
"COPY 1 2\n"
"CPOP 1 3\n"
"CPOP 1 3\n"
"POP 1 0\n"
"* INT\n"
"RET\n"
"\n"
"FUNC SEQ_DIST_4\n"
"MAKE_SEGDES\n"
"COPY 1 0\n"
"CPOP 1 2\n"
"CPOP 1 2\n"
"CALL PRIM-DIST_6\n"
"RET\n"
"\n"
"FUNC PRIM-DIST_6\n"
"CPOP 1 1\n"
"CPOP 1 1\n"
"DIST INT\n"
"RET\n"
"\n"
"FUNC MAKE_SEQUENCE_7\n"
"CPOP 2 1\n"
"CPOP 1 2\n"
"CONST INT 1\n"
"CALL DIST_4\n"
"CALL ++_8\n"
"RET\n"
"\n"
"FUNC DIST_4\n"
"MAKE_SEGDES\n"
"COPY 1 0\n"
"CPOP 1 2\n"
"CPOP 1 2\n"
"CALL PRIM-DIST_6\n"
"RET\n"
"\n"
"FUNC ++_8\n"
"COPY 2 2\n"
"CONST INT 0\n"
"CONST INT 1\n"
"COPY 2 6\n"
"CALL ISEQ-L_9\n"
"COPY 2 4\n"
"CPOP 2 8\n"
"CALL LENGTH_3\n"
"CONST INT 1\n"
"CPOP 2 8\n"
"CALL ISEQ-L_9\n"
"CALL JOIN_12\n"
"RET\n"
"\n"
"FUNC ISEQ-L_9\n"
"POP 1 0\n"
"COPY 1 0\n"
"CPOP 1 3\n"
"CPOP 1 3\n"
"CPOP 1 3\n"
"INDEX\n"
"RET\n"
"\n"
"FUNC LENGTH_3\n"
"POP 1 0\n"
"LENGTHS\n"
"RET\n"
"\n"
"FUNC JOIN_12\n"
"COPY 2 6\n"
"CPOP 2 6\n"
"COPY 2 6\n"
"CPOP 2 6\n"
"CPOP 2 10\n"
"CALL LENGTH_3\n"
"CPOP 2 9\n"
"CALL LENGTH_3\n"
"+ INT\n"
"CALL LEN-PUT-SCALAR_13\n"
"CALL PUT-SCALAR_14\n"
"RET\n"
"\n"
"FUNC LEN-PUT-SCALAR_13\n"
"CPOP 2 3\n"
"CPOP 2 3\n"
"CONST INT 0\n"
"CPOP 1 5\n"
"CALL DIST_4\n"
"CALL PUT-SCALAR_14\n"
"RET\n"
"\n"
"FUNC PUT-SCALAR_14\n"
"CPOP 2 4\n"
"CPOP 2 4\n"
"POP 1 1\n"
"CPOP 2 3\n"
"COPY 1 1\n"
"CPOP 1 4\n"
"CPOP 1 4\n"
"CPOP 1 3\n"
"CPOP 1 5\n"
"CPOP 1 5\n"
"DPERMUTE INT\n"
"RET\n"
;



static void initialize PROTO_((void))
{
    assert(ok_vcode_table());		/* This doesn't do anything yet */
    stack_init();
    vstack_init(vstack_size_init);
    rtstack_init();
    rtstack_push(TOP_LEVEL);	/* End of Program Marker */
    const_install();		/* put constants into memory */
}

/* main interpreter loop */
static void main_loop PROTO_((void))
{
    int pc;
    cvl_timer_t begin_time, 		/* used by -t flag */
	 	start_time, end_time;	/* used by START_TIMER / STOP_TIMER */

    pc = init_pc();		/* initialize program counter */

    if (timer)
	tgt_fos(&begin_time);

    /* continue to execute VCODE instructions until the run_time stack
     * indicates that we've returned to the top-level
     */
    while (pc != TOP_LEVEL) {
	prog_entry_t *instruction = program + pc;	/* current ins */
	int branch_taken = 0;		/* true if don't bump pc at end */

	/* print debugging info before command is executing */
	if (debug_flag) {
	    if (command_trace) {
		DEBUG("Executing : "); show_prog_entry(instruction, instruction->lineno);
	    }
	    if (stack_trace || value_trace) {		/* give a stack dump */
		show_stack();
	    }
	    if (value_trace) {
		DEBUG("\t");
		show_stack_values(stderr);
	    }
	    if (heap_trace) {
		vb_print();
	    }
	}
	/* WARNING: the code in this if-else-if-else block is a mess. */

	if (instruction->vop >= COPY) {		/* non-statements */
	    switch (instruction->vop) {
		case COPY:
		    do_copy(instruction);
		    break;
		case POP:
		    do_pop(instruction);
		    break;
		case CPOP:
		    do_cpop(instruction);
		    break;
		case PAIR:
		    do_pair(instruction);
		    break;
		case UNPAIR:
		    do_unpair(instruction);
		    break;
		case CALL:
		    rtstack_push(pc+1);		/* return to next stmt */
		    pc = instruction->misc.branch;
		    if (pc == UNKNOWN_BRANCH) {
			ERROR("vinterp: internal error: UNKNOWN_BRANCH encountered.\n");
			vinterp_exit(1);
		    }
		    branch_taken = 1;
		    break;
		case RET:
		    pc = rtstack_pop();
		    branch_taken = 1;
		    break;
		case IF:
		    if (!do_cond(instruction)) {
			pc = instruction->misc.branch;
			branch_taken = 1;
		    }
		    break;
		case ELSE:
		    pc = instruction->misc.branch;
		    branch_taken = 1;
		    break;
		case CONST:
		    do_const(instruction);
		    break;
		case EXIT:
    		    abort_on_error = 0;		/* don't dump core */
		    vinterp_exit(0);
		    break;
		case READ:
		    do_read(instruction, stdin);
		    break;
		case WRITE:
		    do_write(instruction, stderr);	
		    break;
		case FOPEN:
		    do_fopen(instruction);
		    break;
		case FCLOSE:
		    do_fclose(instruction);
		    break;
		case FREAD:
		    do_fread(instruction);
		    break;
		case FREAD_CHAR:
		    do_fread_char(instruction);
		    break;
		case FWRITE:
		    do_fwrite(instruction);
		    break;
		case SPAWN:
		    do_spawn(instruction);
		    break;
		case START_TIMER:
		    tgt_fos(&start_time);
		    break;
		case STOP_TIMER: {		/* put value on stack */
		    double elapsed_time;	/* in seconds */
		    tgt_fos(&end_time);
		    elapsed_time = tdf_fos(&end_time, &start_time);

		    stack_push(1, Float, -1);	/* Put vector on stack */
		    assert_mem_size(rep_vud_scratch(1));
		    rep_vud(se_vb(TOS)->vector, 0, elapsed_time, 1, SCRATCH);
		    break;
		    }
		case SRAND: {
		    int srand_arg;
		    assert(se_vb(TOS)->type == Int);
		    assert(se_vb(TOS)->len == 1);
		    assert_mem_size(ext_vuz_scratch(1));
		    srand_arg = ext_vuz(se_vb(TOS)->vector, 0, 1, SCRATCH);
		    rnd_foz(srand_arg);
		    stack_pop(TOS);

		    /* SRAND returns a T.  just make one */
		    stack_push(1, Bool, NOTSEGD);
		    assert_mem_size(rep_vub_scratch(1)); 
		    rep_vub(se_vb(TOS)->vector, 0, 1, 1, SCRATCH);
		    break;
		}
		case FUNC:
		case ENDIF:
		default:
		    ERROR("vinterp: illegal instruction: line %d\n",
			  instruction->lineno);
		    vinterp_exit(1);
		    break;
	    }
	}
	/* Take care of some special cases here */
	else if (instruction->vopdes->cvl_desc == SegOp) {
	    stack_entry_t stack_args[MAX_ARGS];	/* args for call */
	    get_args(instruction, stack_args);
	    if (check_args) {
		if (args_ok(instruction, stack_args) == 0) {
		    ERROR("vinterp: aborting: failed argument check.\n");
		    vinterp_exit(1);
		}
	    }

	    switch (instruction->vop) {
		case MAKE_SEGDES: {
		    if (check_args && se_vb(TOS)->type != Int) {
			ERROR("vinterp: line %d: not applying MAKE_SEGDES to an integer vector.\n",
			      instruction->lineno);
			vinterp_exit(1);
		    } else {
			/* lengths vector is on TOS */
			stack_entry_t lengths_se = TOS;

			/* Find length of result vector */
			int lengths_len = se_vb(lengths_se)->len;
			int vector_len;

			assert_mem_size(add_ruz_scratch(lengths_len));

			vector_len = add_ruz(se_vb(lengths_se)->vector,
					     lengths_len, SCRATCH);

			stack_push(lengths_len, Segdes, vector_len);

			assert_mem_size(mke_fov_scratch(vector_len, lengths_len));
			mke_fov(se_vb(TOS)->vector, se_vb(lengths_se)->vector,
				vector_len, lengths_len, SCRATCH);
			stack_pop(lengths_se);
		    }
		}
		break;
		case LENGTHS: {
		    if (se_vb(TOS)->type != Segdes) {
			ERROR("vinterp: line %d: applying LENGTHS to a non-segment descriptor.\n",
			      instruction->lineno);
			vinterp_exit(1);
		    } else {
			/* segd is on top of stack */
			stack_entry_t segd = TOS;

			/* answer is same length as segd */
			stack_push(se_vb(segd)->len, Int, NOTSEGD);

			assert_mem_size(len_fos_scratch(se_vb(segd)->seg_len, se_vb(segd)->len));
			len_fos(se_vb(TOS)->vector,
				se_vb(segd)->vector,
				se_vb(segd)->seg_len,
				se_vb(segd)->len,
				SCRATCH);
			stack_pop(segd);
		    }
		}
		break;
		case LENGTH: {  /* use REPLACE to put scalar on stack */
		    if (se_vb(TOS)->type == Segdes) {
			ERROR("vinterp: line %d: applying LENGTH to a segment descriptor.\n",
			      instruction->lineno);
			vinterp_exit(1);
		    } else {
			stack_entry_t vector_se = TOS;
			stack_push(1, Int, NOTSEGD);
			assert_mem_size(rep_vuz_scratch(se_vb(vector_se)->len));
			rep_vuz(se_vb(TOS)->vector, 
				0,
				se_vb(vector_se)->len,
				1,
				SCRATCH);
			stack_pop(vector_se);
		    }
		}
		break;
		case PACK: {
		    stack_entry_t vector_se = stack_args[0];
		    stack_entry_t flag_se = stack_args[1];
		    stack_entry_t segd_se = stack_args[2];
		    stack_entry_t len_se;
		    stack_entry_t dest_se;
		    stack_entry_t dest_segd_se;
		    int len;
		    void (*funct)();			/* cvl function */
		    int (*s_funct)();			/* scratch function */

		    /* length result of pk1 */
		    stack_push(se_vb(segd_se)->len, Int, NOTSEGD);
		    len_se = TOS;		

		    assert_mem_size(pk1_lev_scratch(se_vb(segd_se)->seg_len,
						    se_vb(segd_se)->len));

		    /* get the lengths vector of result */
		    pk1_lev(se_vb(len_se)->vector,
			    se_vb(flag_se)->vector,
			    se_vb(segd_se)->vector,
			    se_vb(segd_se)->seg_len,
			    se_vb(segd_se)->len,
			    SCRATCH);
		    
		    /* find length of pack result */
		    assert_mem_size(add_ruz_scratch(se_vb(len_se)->len));
		    len = add_ruz(se_vb(len_se)->vector, 
				  se_vb(len_se)->len, SCRATCH);

		    /* push the destination vector and segd */
		    stack_push(len, se_vb(vector_se)->type, NOTSEGD);
		    dest_se = TOS;
		    stack_push(se_vb(len_se)->len, Segdes, len);
		    dest_segd_se = TOS;

		    /* create segment descriptor for result */
		    assert_mem_size(mke_fov_scratch(len, se_vb(len_se)->len));
		    mke_fov(se_vb(dest_segd_se)->vector, 
			    se_vb(len_se)->vector, 
		    	    len, 
			    se_vb(len_se)->len, SCRATCH);

		    /* finish the pack */
		    funct = cvl_funct(instruction->vop, instruction->type);
		    s_funct = (int (*)())scratch_cvl_funct(instruction->vop, instruction->type);

		    assert_mem_size((*s_funct)(se_vb(segd_se)->seg_len, se_vb(segd_se)->len, se_vb(dest_segd_se)->seg_len, se_vb(dest_segd_se)->len));
		    (*funct)(se_vb(dest_se)->vector,
			     se_vb(vector_se)->vector,
			     se_vb(flag_se)->vector, 
			     se_vb(segd_se)->vector,
			     se_vb(segd_se)->seg_len,
			     se_vb(segd_se)->len,
			     se_vb(dest_segd_se)->vector,
			     se_vb(dest_segd_se)->seg_len,
			     se_vb(dest_segd_se)->len,
			     SCRATCH);
		    /* pop off the arg vectors */
		    stack_pop(len_se);
		    stack_pop(segd_se);
		    stack_pop(flag_se);
		    stack_pop(vector_se);
		    }
		    break;
		default: 
		    ERROR("vinterp: internal error: illegal SegOp in vcode_table.\n");
		    vinterp_exit(1);
	    }
	} else {
	    /* do a vector op */
	    vb_t *dest[MAX_OUT];		/* where answer will go */
	    stack_entry_t stack_args[MAX_ARGS];	/* args for call */
	    void (*funct)();			/* cvl function */

	    get_args(instruction, stack_args);

	    if (check_args) {
	        if (args_ok(instruction, stack_args) == 0) {
		    ERROR("vinterp: aborting: failed argument check.\n");
		    vinterp_exit(1);
		}
	    }

	    /* allocate room for result and scratch */
	    allocate_result(instruction, stack_args, dest);
	    assert_scratch(instruction, stack_args);

	    funct = cvl_funct(instruction->vop, instruction->type);
	    if (funct == NULL) {
		ERROR("vinterp: internal error: missing cvl_funct_list entry for %s, type %s.\n",
		      instruction->vopdes->vopname, type_string(instruction->type));
		vinterp_exit(1);
	    }

	    /* Do the cvl function call.
	     * This is a big case statement to handle the various
	     * possibilities of argument order */
	    switch (instruction->vopdes->cvl_desc) {
		case Elwise1:
		    (*funct)(dest[0]->vector, 
			     se_vb(stack_args[0])->vector,
			     se_vb(stack_args[0])->len,
			     SCRATCH);
		    break;
		case Elwise2:
		    (*funct)(dest[0]->vector, 
			     se_vb(stack_args[0])->vector,
			     se_vb(stack_args[1])->vector,
		             se_vb(stack_args[0])->len,
			     SCRATCH);
		    break;
		case Elwise3:
		    (*funct)(dest[0]->vector, 
			     se_vb(stack_args[0])->vector,
			     se_vb(stack_args[1])->vector,
			     se_vb(stack_args[2])->vector,
		             se_vb(stack_args[0])->len,
			     SCRATCH);
		    break;
		case Scan:
		case Reduce:
		    (*funct)(dest[0]->vector,
			     se_vb(stack_args[0])->vector,
			     se_vb(stack_args[1])->vector,
			     se_vb(stack_args[1])->seg_len,
			     se_vb(stack_args[1])->len,
			     SCRATCH);
		    break;
		case Bpermute:
		    (*funct)(dest[0]->vector,
			     se_vb(stack_args[0])->vector, /* source */
			     se_vb(stack_args[1])->vector, /* index */
			     se_vb(stack_args[2])->vector, /* s_sgd */
			     se_vb(stack_args[2])->seg_len,
			     se_vb(stack_args[2])->len,
			     se_vb(stack_args[3])->vector, /* d_sgd */
			     se_vb(stack_args[3])->seg_len,
			     se_vb(stack_args[2])->len,
			     SCRATCH);
		    break;
		case Permute:
		    (*funct)(dest[0]->vector,
			     se_vb(stack_args[0])->vector, /* src */
			     se_vb(stack_args[1])->vector, /* index */
			     se_vb(stack_args[2])->vector, /* segd */
			     se_vb(stack_args[2])->seg_len,
			     se_vb(stack_args[2])->len,
			     SCRATCH);
		    break;
		case Fpermute:
		case Bfpermute:
		    (*funct)(dest[0]->vector,
			     se_vb(stack_args[0])->vector,  /* source */
			     se_vb(stack_args[1])->vector,  /* index */
			     se_vb(stack_args[2])->vector,  /* flags */
			     se_vb(stack_args[3])->vector,  /* s_sgd */
			     se_vb(stack_args[3])->seg_len,
			     se_vb(stack_args[3])->len,
			     se_vb(stack_args[4])->vector,  /* d_sgd */
			     se_vb(stack_args[4])->seg_len,
			     se_vb(stack_args[4])->len,
			     SCRATCH);
		    break;
		case Dpermute:
		    /* optimize storage for when default can be destroyed */
		    if (se_vb(stack_args[2])->count == 1) {
			vstack_pop(dest[0]);
			dest[0] = se_vb(TOS) = se_vb(stack_args[2]);
			se_vb(stack_args[2])->count++;
		    }
		    (*funct)(dest[0]->vector,
			     se_vb(stack_args[0])->vector,  /* source */
			     se_vb(stack_args[1])->vector,  /* index */
			     se_vb(stack_args[2])->vector,  /* default */
			     se_vb(stack_args[3])->vector,  /* s_sgd */
			     se_vb(stack_args[3])->seg_len,
			     se_vb(stack_args[3])->len,
			     se_vb(stack_args[4])->vector,  /* d_sgd */
			     se_vb(stack_args[4])->seg_len,
			     se_vb(stack_args[4])->len,
			     SCRATCH);
		    break;
		case Dfpermute:
		    /* optimize storage for when default can be destroyed */
		    if (se_vb(stack_args[3])->count == 1) {
			vstack_pop(dest[0]);
			dest[0] = se_vb(TOS) = se_vb(stack_args[2]);
			se_vb(stack_args[3])->count++;
		    }
		    (*funct)(dest[0]->vector,		  /* dest */
			     se_vb(stack_args[0])->vector,  /* source */
			     se_vb(stack_args[1])->vector,  /* index */
			     se_vb(stack_args[2])->vector,  /* flags */
			     se_vb(stack_args[3])->vector,  /* defaults */
			     se_vb(stack_args[4])->vector,  /* s_sgd */
			     se_vb(stack_args[4])->seg_len,
			     se_vb(stack_args[4])->len,
			     se_vb(stack_args[5])->vector,  /* d_sgd */
			     se_vb(stack_args[5])->seg_len,
			     se_vb(stack_args[5])->len,
			     SCRATCH);
		    break;
		case Extract:
		    (*funct)(dest[0]->vector,
			     se_vb(stack_args[0])->vector,  /* source */
			     se_vb(stack_args[1])->vector,  /* index */
			     se_vb(stack_args[2])->vector,  /* sgd */
			     se_vb(stack_args[2])->seg_len,
			     se_vb(stack_args[2])->len,
			     SCRATCH);
		    break;
		case Replace: {
		    /* CVL replace is destructive; vcode's is not.
		     * If the source arg has only one ref, and hence
		     * will be deallocated after the REPLACE, we can
		     * save memory and copy costs by just REPLACEing
		     * directly into the source.
		     * Otherwise, we must make a copy of first arg.
		     */
		    if (se_vb(stack_args[0])->count == 1) { /* only ref */
			se_vb(stack_args[0])->count ++;     /* so won't free */

			vstack_pop(dest[0]);	  /* free storage of dest */
 			/* fix stack */
			dest[0] = se_vb(TOS) = se_vb(stack_args[0]);   
		    } else {				  /* copy source */
			vec_p copy_to = dest[0]->vector; 
			int res_len = dest[0]->len;
			switch (dest[0]->type) {	/* switch on type */
			    case Int:
			      assert_mem_size(cpy_wuz_scratch(res_len));
			      cpy_wuz(copy_to, se_vb(stack_args[0])->vector, res_len, SCRATCH);
			      break;
			    case Bool:
			      assert_mem_size(cpy_wub_scratch(res_len));
			      cpy_wub(copy_to, se_vb(stack_args[0])->vector, res_len, SCRATCH);
			      break;
			    case Float:
			      assert_mem_size(cpy_wud_scratch(res_len));
			      cpy_wud(copy_to, se_vb(stack_args[0])->vector, res_len, SCRATCH);
			      break;
			    default:
 			      ERROR("vinterp: illegal operand for REPLACE.\n");
			      vinterp_exit (1);
			}
		    }
		    (*funct)(dest[0]->vector,  
			     se_vb(stack_args[1])->vector,  /* index */
			     se_vb(stack_args[2])->vector,  /* value */
			     se_vb(stack_args[3])->vector,  /* sgd */
			     se_vb(stack_args[3])->seg_len,
			     se_vb(stack_args[3])->len,
			     SCRATCH);
		    break;
		}
		case Dist:
		    (*funct)(dest[0]->vector,  
			     se_vb(stack_args[0])->vector,
			     se_vb(stack_args[1])->vector,
			     se_vb(stack_args[1])->seg_len,
			     se_vb(stack_args[1])->len,
			     SCRATCH);
		    break;
		case Index:
		    ind_lez (dest[0]->vector, 			/* dest */ 
			     se_vb(stack_args[0])->vector,	/* init */
			     se_vb(stack_args[1])->vector,	/* stride */
			     se_vb(stack_args[2])->vector,	/* d_segd */
			     se_vb(stack_args[2])->seg_len,
			     se_vb(stack_args[2])->len,
			     SCRATCH);
		    break;
		case NotSupported:
		default:
		    ERROR("vinterp: line %d: Operation %s not supported.\n",
			  instruction->lineno, 
			  instruction->vopdes->vopname);
		    vinterp_exit(1);
		}

	    /* remove used up args */
	    pop_args(stack_args, instruction->vopdes->arg_num);
	    }
	/* done executing command */
	if (branch_taken) 			/* adjust the program counter */
	    branch_taken = 0;
	else pc++;

    } /* done with program */
    if (timer) {
	tgt_fos(&end_time);
	PRINT("elapsed time = %f seconds\n",
	     tdf_fos(&end_time, &begin_time));
    }

    /* print out final results */
    show_stack_values(stdout);
}

/* initialize the program counter to location of main, or of first funct */
static int init_pc()
{
    hash_entry_t main_hash;

    /* figure out where to start */
    if (main_fn[0] == '\0') {
	ERROR("vinterp: internal error: init_pc(): no start function.\n");
	vinterp_exit(1);
    } 
    main_hash = hash_table_lookup(main_fn);
    if (main_hash == NULL) {
	ERROR("vinterp: start function %s not found.\n", main_fn);
	vinterp_exit(1);
    } 
    return (main_hash->prog_num);
}

/* exit routine.  this is called if the interpreter finds itself in an 
 * illegal configuration.  create a core dump if the user wants one.
 */
void vinterp_exit(status)
int status;
{
    if (abort_on_error) {
        ERROR("vinterp: dumping core\n");
	abort();
    } else exit(status);
}


void debug_show_stack ()
{
  show_stack_values(stdout);
}


int nk_nesl_init()
{
    INFO("init\n");
    return 0;
}


int nk_nesl_exec(void *vcode)
{
    INFO("execute program at %p\n",vcode);
    if (vcode==0) {
	INFO("Using built-in test\n");
	vcode = programstr;
    }

    // flag config
    check_args   = 0;
    debug_flag   = 1;
    lex_trace    = 0;
    stack_trace  = 0;
    command_trace= 1;
    value_trace  = 0;
    runtime_trace= 0;
    program_dump = 1;
    link_trace   = 0;
    heap_trace   = 0;
    abort_on_error = 0;
    timer        = 0;
    // vstack_size_init = ...

    uint64_t size = strlen(vcode);
    void *tmp = malloc(size+1);
    if (!tmp) {
	ERROR("Failed to allocate temporary copy of program\n");
	return -1;
    }
    strcpy(tmp,vcode);

    nesl_exec(tmp);

    free(tmp);
    
    return 0;
}

void nk_nesl_deinit()
{
    INFO("deinit\n");
}


int yywrap()
{
    return 1;
}




/* int main(argc, argv) */
/* int argc; */
/* char *argv[]; */
/* { */
/*     extern char* optarg; */
/*     extern int optind; */
/*     extern int errno; */
/*     extern __const char *__const sys_errlist[]; */
/*     /\* extern char *sys_errlist[]; *\/ */
/*     int c; */
/*     char *filename = NULL; */
/*     FILE *file; */
/*     extern int getopt PROTO_((int, char *const[], const char *)); */

/*     ndpc_init_preempt_threads(); */

/*     init_file_list(); */

/* #if MPI */
/*     CVL_init(&argc, &argv); */
/* #else */
/* #if CM2 */
/*     CVL_init(0); */
/* #else */
/* #if CM5 */
/*     CVL_init(); */
/* #endif */
/* #endif */
/* #endif */

/*     while ((c = getopt(argc, argv, "cd:gm:r:t")) != EOF) { */
/* 	switch (c) { */
/* 	    case 'c': */
/* 		check_args = 1; */
/* 		break; */
/* 	    case 'd': 	/\* lots of debug options *\/ */
/* 		if (strcmp(optarg, "lex") == 0) */
/* 		    lex_trace = 1; */
/* 		else if (strcmp(optarg, "stack") == 0) */
/* 		    stack_trace = 1; */
/* 		else if (strcmp(optarg, "command") == 0) */
/* 		    command_trace = 1; */
/* 		else if (strcmp(optarg, "values") == 0) */
/* 		    value_trace = 1; */
/* 		else if (strcmp(optarg, "runtime") == 0) */
/* 		    runtime_trace = 1; */
/* 		else if (strcmp(optarg, "program") == 0) */
/* 		    program_dump = 1; */
/* 		else if (strcmp(optarg, "link") == 0) */
/* 		    link_trace = 1; */
/* 		else if (strcmp(optarg, "heap") == 0) */
/* 		    heap_trace = 1; */
/* 		else if (strcmp(optarg, "abort") == 0) */
/* 		    abort_on_error = 1; */
/* 		else  */
/* 		    usage(); */
/* 		debug_flag = 1; */
/* 		break; */
/* 	    case 't':	/\* time it *\/ */
/* 		timer = 1; */
/* 		break; */
/* 	    case 'm':	/\* vstack size *\/ */
/* 		if (! sscanf(optarg,"%u", &vstack_size_init)) */
/* 		    usage(); */
/* 		break; */
/* 	    case 'g':  */
/* 		garbage_notify = 1; */
/* 		break; */

/* 	    default: */
/* 		usage(); */
/* 	} */
/*     } */

/*     /\* read file name: should be only unparsed argument *\/ */
/*     if ((argc != optind + 1) ||  */
/* 	((filename = argv[optind]) == NULL)) { */
/* 	_fprintf(stderr, "vinterp: Please supply file name.\n"); */
/* 	usage(); */
/*     } */
/*     file = fopen(filename, "r"); */
/*     if (file == NULL) { */
/* 	_fprintf(stderr, "vinterp: Error opening input file %s: %s\n", */
/* 			filename, sys_errlist[errno]); */
/* 	vinterp_exit (1); */
/*     } */
/*     initialize_parse(); */
/*     yyin = file; */

/*     /\* parse and link the VCODE routine *\/ */
/*     if (! (yyparse() == 0 && parse_error == 0 && do_link() == 0)) { */
/* 	_fprintf(stderr, "Parse failure\n"); */
/* 	vinterp_exit (1); */
/*     } */

/*     if (program_dump) */
/* 	show_program(); */

/*     initialize(); */
/*     main_loop(); */

/* #if MPI | CM2 | CM5 */
/*     CVL_quit(); */
/* #endif */
/*     ndpc_deinit_preempt_threads(); */

/*     (void) fclose(file); */
/*     return 0; */
/* } */

