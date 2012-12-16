// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// Copyright 2010, Julian Catchen <jcatchen@uoregon.edu>
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

//
// utils.cc -- common routines needed in multiple object files.
//
// Julian Catchen
// jcatchen@uoregon.edu
// University of Oregon
//
// $Id: utils.cc 1989 2010-11-03 01:00:23Z catchen $
//
#include "utils.h"

int is_integer(char *str) {
    //
    // Adapted from the strtol manpage.
    //
    char *endptr;

    // To distinguish success/failure after call
    errno = 0;
    long val = strtol(str, &endptr, 10);

    //
    // Check for various possible errors
    //
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
	|| (errno != 0 && val == 0)) {
	return -1;
    }

    if (endptr == str || *endptr != '\0')
	return -1;

    return (int) val;
}

double is_double(char *str) {
    //
    // Adapted from the strtol manpage.
    //
    char *endptr;

    // To distinguish success/failure after call
    errno = 0;
    long val = strtod(str, &endptr);

    //
    // Check for various possible errors
    //
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
	|| (errno != 0 && val == 0)) {
	return -1;
    }

    if (endptr == str || *endptr != '\0')
	return -1;

    return val;
}

double factorial(double n) {
    double fact = 1;

    for (double i = n; i > 1; i--)
        fact *= i;

    return fact;
}

double reduced_factorial(double n, double d) {
    double f = n - d;

    if (f < 0) 
        return 0;
    else if (f == 0) 
        return 1;
    else if (f == 1)
        return n;

    f = n;
    n--;
    while (n > d) {
        f *= n;
        n--;
    }

    return f;
}

double log_factorial(double n) {
    double fact = 0;

    for (double i = n; i > 1; i--)
        fact += log(i);

    return fact;
}

double reduced_log_factorial(double n, double d) {
    double f = n - d;

    if (f < 0) 
        return 0;
    else if (f == 0) 
        return 0;
    else if (f == 1)
        return log(n);

    f = log(n);
    n--;
    while (n > d) {
        f += log(n);
        n--;
    }

    return f;
}

bool compare_pair(pair<char, int> a, pair<char, int> b) {
    return (a.second > b.second);
}

bool compare_pair_intdouble(pair<int, double> a, pair<int, double> b) {
    return (a.second < b.second);
}

bool compare_ints(int a, int b) {
    return (a > b);
}

bool compare_pair_snp(pair<string, SNP *> a, pair<string, SNP *> b) {
    return (a.second->col < b.second->col);
}
