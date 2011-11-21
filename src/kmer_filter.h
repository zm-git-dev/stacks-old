// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// Copyright 2011, Julian Catchen <jcatchen@uoregon.edu>
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

#ifndef __KMER_FILTER_H__
#define __KMER_FILTER_H__

#include <stdlib.h>
#include <getopt.h> // Process command-line options
#include <dirent.h> // Open/Read contents of a directory
#include <string.h>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
using std::stringstream;
using std::istream;
using std::ofstream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#include <vector>
using std::vector;
#include <map>
using std::map;
#include <set>
using std::set;
#include <utility>
using std::pair;

#ifdef __GNUC__
#include <ext/hash_map>
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;
#else
#include <hash_map>
#endif

#include "constants.h" 
#include "clean.h"
#include "kmers.h"
#include "Bustard.h"   // Reading input files in Tab-separated Bustard format
#include "Fasta.h"     // Reading input files in FASTA format
#include "Fastq.h"     // Reading input files in FASTQ format

typedef hash_map<char *, long, hash<const char *>, eqstr> SeqKmerHash;

void help( void );
void version( void );
int  parse_command_line(int, char**);
int  build_file_list(vector<string> &, vector<pair<string, string> > &);
int  process_reads(string, string, SeqKmerHash &, map<string, long> &);
int  process_paired_reads(string, string, string, string, SeqKmerHash &, map<string, long> &);
int  print_results(map<string, map<string, long> > &);

//
// Functions for finding and removing reads with rare kmers
//
int  populate_kmers(vector<pair<string, string> > &, vector<pair<string, string> > &, SeqKmerHash &);
int  write_rare_abundant_kmers(SeqKmerHash &);
int  process_file_kmers(string, SeqKmerHash &);
int  generate_kmer_dist(SeqKmerHash &);
int  kmer_map_cmp(pair<char *, long>, pair<char *, long>);
int  kmer_lookup(SeqKmerHash &, char *, char *, int, int *, int *);

#endif // __KMER_FILTER_H__