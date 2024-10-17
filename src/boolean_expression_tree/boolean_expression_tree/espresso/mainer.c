/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
/*
 *  Main driver for espresso
 *
 *  Old style -do xxx, -out xxx, etc. are still supported.
 */

#include "espresso.h"
#include "main.h"		/* table definitions for options */

static FILE *last_fp;
static int input_type = FD_type;

void getPLA(int opt, int argc, char **argv, int option, pPLA *PLA, int out_type);
void delete_arg(int *argc, register char **argv, int num);
void init_runtime(void);
void backward_compatibility_hack(int *argc, char **argv, int *option, int *out_type);
void runtime(void);
void usage(void);
bool check_arg(int *argc, register char **argv, register char *s);



void do_something()
{
	cube.num_vars = 1;
}

void do_something_else()
{
	cdata.best = 17;
}

void getPLA(int opt, int argc, char **argv, int option, pPLA *PLA, int out_type)
{
    FILE *fp;
    int needs_dcset, needs_offset;
    char *fname;

    if (opt >= argc) {
	fp = stdin;
	fname = "(stdin)";
    } else {
	fname = argv[opt];
	if (strcmp(fname, "-") == 0) {
	    fp = stdin;
	} else if ((fp = fopen(argv[opt], "r")) == NULL) {
	    fprintf(stderr, "%s: Unable to open %s\n", argv[0], fname);
	    exit(1);
	}
    }
    if (option_table[option].key == KEY_echo) {
	needs_dcset = (out_type & D_type) != 0;
	needs_offset = (out_type & R_type) != 0;
    } else {
	needs_dcset = option_table[option].needs_dcset;
	needs_offset = option_table[option].needs_offset;
    }

    if (read_pla(fp, needs_dcset, needs_offset, input_type, PLA) == EOF) {
	fprintf(stderr, "%s: Unable to find PLA on file %s\n", argv[0], fname);
	exit(1);
    }
    (*PLA)->filename = strdup(fname);
    filename = (*PLA)->filename;
/*    (void) fclose(fp);*/
/* hackto support -Dmany */
    last_fp = fp;
}


void runtime(void)
{
    int i;
    long total = 1, temp;

    for(i = 0; i < TIME_COUNT; i++) {
	total += total_time[i];
    }
    for(i = 0; i < TIME_COUNT; i++) {
	if (total_calls[i] != 0) {
	    temp = 100 * total_time[i];
	    printf("# %s\t%2d call(s) for %s (%2ld.%01ld%%)\n",
		total_name[i], total_calls[i], print_time(total_time[i]),
		    temp/total, (10 * (temp%total)) / total);
	}
    }
}


void init_runtime(void)
{
    total_name[READ_TIME] =     "READ       ";
    total_name[WRITE_TIME] =    "WRITE      ";
    total_name[COMPL_TIME] =    "COMPL      ";
    total_name[REDUCE_TIME] =   "REDUCE     ";
    total_name[EXPAND_TIME] =   "EXPAND     ";
    total_name[ESSEN_TIME] =    "ESSEN      ";
    total_name[IRRED_TIME] =    "IRRED      ";
    total_name[GREDUCE_TIME] =  "REDUCE_GASP";
    total_name[GEXPAND_TIME] =  "EXPAND_GASP";
    total_name[GIRRED_TIME] =   "IRRED_GASP ";
    total_name[MV_REDUCE_TIME] ="MV_REDUCE  ";
    total_name[RAISE_IN_TIME] = "RAISE_IN   ";
    total_name[VERIFY_TIME] =   "VERIFY     ";
    total_name[PRIMES_TIME] =   "PRIMES     ";
    total_name[MINCOV_TIME] =   "MINCOV     ";
}


void subcommands(void)
{
    int i, col;
    printf("                ");
    col = 16;
    for(i = 0; option_table[i].name != 0; i++) {
	if ((col + strlen(option_table[i].name) + 1) > 76) {
	    printf(",\n                ");
	    col = 16;
	} else if (i != 0) {
	    printf(", ");
	}
	printf("%s", option_table[i].name);
	col += strlen(option_table[i].name) + 2;
    }
    printf("\n");
}


void usage(void)
{
    printf("%s\n\n", VERSION);
    printf("SYNOPSIS: espresso [options] [file]\n\n");
    printf("  -d        Enable debugging\n");
    printf("  -e[opt]   Select espresso option:\n");
    printf("                fast, ness, nirr, nunwrap, onset, pos, strong,\n");
    printf("                eat, eatdots, kiss, random\n");
    printf("  -o[type]  Select output format:\n");
    printf("                f, fd, fr, fdr, pleasure, eqntott, kiss, cons\n");
    printf("  -rn-m     Select range for subcommands:\n");
    printf("                d1merge: first and last variables (0 ... m-1)\n");
    printf("                minterms: first and last variables (0 ... m-1)\n");
    printf("                opoall: first and last outputs (0 ... m-1)\n");
    printf("  -s        Provide short execution summary\n");
    printf("  -t        Provide longer execution trace\n");
    printf("  -x        Suppress printing of solution\n");
    printf("  -v[type]  Verbose debugging detail (-v '' for all)\n");
    printf("  -D[cmd]   Execute subcommand 'cmd':\n");
    subcommands();
    printf("  -Sn       Select strategy for subcommands:\n");
    printf("                opo: bit2=exact bit1=repeated bit0=skip sparse\n");
    printf("                opoall: 0=minimize, 1=exact\n");
    printf("                pair: 0=algebraic, 1=strongd, 2=espresso, 3=exact\n");
    printf("                pairall: 0=minimize, 1=exact, 2=opo\n");
    printf("                so_espresso: 0=minimize, 1=exact\n");
    printf("                so_both: 0=minimize, 1=exact\n");
}

/*
 *  Hack for backward compatibility (ACK! )
 */

void backward_compatibility_hack(int *argc, char **argv, int *option, int *out_type)
{
    int i, j;

    /* Scan the argument list for something to do (default is ESPRESSO) */
    *option = 0;
    for(i = 1; i < (*argc)-1; i++) {
	if (strcmp(argv[i], "-do") == 0) {
	    for(j = 0; option_table[j].name != 0; j++)
		if (strcmp(argv[i+1], option_table[j].name) == 0) {
		    *option = j;
		    delete_arg(argc, argv, i+1);
		    delete_arg(argc, argv, i);
		    break;
		}
	    if (option_table[j].name == 0) {
		fprintf(stderr,
		 "espresso: bad keyword \"%s\" following -do\n",argv[i+1]);
		exit(1);
	    }
	    break;
	}
    }

    for(i = 1; i < (*argc)-1; i++) {
	if (strcmp(argv[i], "-out") == 0) {
	    for(j = 0; pla_types[j].key != 0; j++)
		if (strcmp(pla_types[j].key+1, argv[i+1]) == 0) {
		    *out_type = pla_types[j].value;
		    delete_arg(argc, argv, i+1);
		    delete_arg(argc, argv, i);
		    break;
		}
	    if (pla_types[j].key == 0) {
		fprintf(stderr,
		   "espresso: bad keyword \"%s\" following -out\n",argv[i+1]);
		exit(1);
	    }
	    break;
	}
    }

    for(i = 1; i < (*argc); i++) {
	if (argv[i][0] == '-') {
	    for(j = 0; esp_opt_table[j].name != 0; j++) {
		if (strcmp(argv[i]+1, esp_opt_table[j].name) == 0) {
		    delete_arg(argc, argv, i);
		    *(esp_opt_table[j].variable) = esp_opt_table[j].value;
		    break;
		}
	    }
	}
    }

    if (check_arg(argc, argv, "-fdr")) input_type = FDR_type;
    if (check_arg(argc, argv, "-fr")) input_type = FR_type;
    if (check_arg(argc, argv, "-f")) input_type = F_type;
}


/* delete_arg -- delete an argument from the argument list */
void delete_arg(int *argc, register char **argv, int num)
{
    register int i;
    (*argc)--;
    for(i = num; i < *argc; i++) {
	argv[i] = argv[i+1];
    }
}


/* check_arg -- scan argv for an argument, and return TRUE if found */
bool check_arg(int *argc, register char **argv, register char *s)
{
    register int i;
    for(i = 1; i < *argc; i++) {
	if (strcmp(argv[i], s) == 0) {
	    delete_arg(argc, argv, i);
	    return TRUE;
	}
    }
    return FALSE;
}
