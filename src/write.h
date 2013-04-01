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

#ifndef __WRITE_H__
#define __WRITE_H__

#include <string>
using std::string;
#include <iostream>
#include <fstream>
using std::ofstream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#include "input.h"
#include "clean.h"

int  write_fastq(ofstream *, Read *, bool);
int  write_fastq(ofstream *, Seq *);
int  write_fastq(ofstream *, Seq *, string);
int  write_fasta(ofstream *, Read *, bool);
int  write_fasta(ofstream *, Seq *);
int  write_fasta(ofstream *, Seq *, string);

#endif // __WRITE_H__