// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// Copyright 2013, Julian Catchen <jcatchen@uoregon.edu>
//
// This file is part of Stacks.
//
// Stacks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Stacks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Stacks.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __RXSTACKS_H__
#define __RXSTACKS_H__

#ifdef _OPENMP
#include <omp.h>    // OpenMP library
#endif
#include <getopt.h> // Process command-line options
#include <dirent.h> // Open/Read contents of a directory
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utility>
using std::pair;
using std::make_pair;
#include <string>
using std::string;
#include <iostream>
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
#include <iomanip> // std::setprecision
#include <sstream>
using std::stringstream;
#include <vector>
using std::vector;
#include <queue>
using std::queue;
#include <map>
using std::map;
#include <set>
using std::set;

#include "constants.h"
#include "stacks.h"
#include "locus.h"
#include "PopMap.h"
#include "PopSum.h"
#include "utils.h"
#include "sql_utilities.h"
#include "models.h"

void    help( void );
void    version( void );
int     parse_command_line(int, char**);
int     build_file_list(vector<pair<int, string> > &);
int     prune_nucleotides(CSLocus *, Locus *, ofstream &);
int     invoke_model(Locus *, int, map<char, int> &, ofstream &);
int     call_alleles(Locus *, set<int> &); 
int     fill_catalog_snps(map<int, CSLocus *> &);
int     log_model_calls(ofstream &, Locus *);
int     write_results(string, map<int, Locus *> &);

bool    hap_compare(pair<string, int>, pair<string, int>);

#endif // __RXSTACKS_H__
