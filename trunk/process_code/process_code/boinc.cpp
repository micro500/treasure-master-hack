#include <cstdarg>
#include <stdio.h>

#include "boinc.h"

#ifdef BOINCAPP
char buf[256], output_path[512];
MFILE output_file;
#endif

void initialize_boinc()
{
#ifdef BOINCAPP
	// Initialize BOINC
	int retval = boinc_init();
	if (retval) {
		fprintf(stderr, "%s boinc_init returned %d\n",
			boinc_msg_prefix(buf, sizeof(buf)), retval
		);
		exit(retval);
	}

	boinc_resolve_filename(OUTPUT_FILENAME, output_path, sizeof(output_path));

	retval = output_file.open(output_path, "wb");
	if (retval) {
        fprintf(stderr, "%s APP: bruteforcer output open failed:\n",
            boinc_msg_prefix(buf, sizeof(buf))
        );
        fprintf(stderr, "%s resolved name %s, retval %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), output_path, retval
        );
        perror("open");
        exit(1);
    }
#endif
}

void finish_boinc()
{
#ifdef BOINCAPP
	int retval = output_file.flush();
    if (retval) {
        fprintf(stderr, "%s APP: upper_case flush failed %d\n",
            boinc_msg_prefix(buf, sizeof(buf)), retval
        );
        exit(1);
    }

	boinc_finish(0);
#endif
}

void boinc_log(char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);

#ifdef BOINCAPP
	output_file.printf(fmt, args);
#else
	printf(fmt, args);
#endif

	va_end(args);
}

void fraction_done(double percentage)
{
#ifdef BOINCAPP
	boinc_fraction_done(percentage);
#else
	
#endif
}