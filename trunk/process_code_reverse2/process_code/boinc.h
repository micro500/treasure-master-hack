#ifndef BOINC_H
#define BOINC_H

#ifdef BOINCAPP

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cstdio>
#include <cctype>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#endif

#include "str_util.h"
#include "util.h"
#include "filesys.h"
#include "boinc_api.h"
#include "mfile.h"

#define OUTPUT_FILENAME "out"

extern char buf[256], output_path[512];
extern MFILE output_file;

#endif // BOINCAPP

void initialize_boinc();

void finish_boinc();

#ifdef BOINCAPP
#define boinc_log(...) output_file.printf(__VA_ARGS__)
#else
#define boinc_log(...) printf(__VA_ARGS__)
#endif

void fraction_done(double percentage);


#endif // BOINC_H