// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// input.cc -- routines to read various formats of data into the XXX data structure.
//
// Julian Catchen
// jcatchen@uoregon.edu
// University of Oregon
//

#include "input.h"

Seq::Seq() { 
    this->id      = NULL;
    this->seq     = NULL;
    this->qual    = NULL;
    this->loc_str = NULL;
    this->chr     = NULL;
    this->bp      = 0;
}

Seq::Seq(const char *id, const char *seq) { 
    this->id      = new char[strlen(id)   + 1];
    this->seq     = new char[strlen(seq)  + 1];
    this->qual    = NULL; 
    this->loc_str = NULL;
    this->chr     = NULL;
    this->bp      = 0;

    strcpy(this->id,   id); 
    strcpy(this->seq,  seq);
}

Seq::Seq(const char *id, const char *seq, const char *qual)  { 
    this->id      = new char[strlen(id)   + 1];
    this->seq     = new char[strlen(seq)  + 1];
    this->qual    = new char[strlen(qual) + 1];
    this->loc_str = NULL;
    this->chr     = NULL;
    this->bp      = 0;

    strcpy(this->id,   id); 
    strcpy(this->seq,  seq); 
    strcpy(this->qual, qual); 
}

Seq::Seq(const char *id, const char *seq, const char *qual, const char *chr, uint bp)  { 
    this->id      = new char[strlen(id)   + 1];
    this->seq     = new char[strlen(seq)  + 1];
    this->qual    = new char[strlen(qual) + 1];
    this->chr     = new char[strlen(chr)  + 1];
    this->loc_str = new char[strlen(chr)  + 11];
    this->bp      = bp;

    strcpy(this->id,   id);
    strcpy(this->seq,  seq);
    strcpy(this->qual, qual);
    strcpy(this->chr,  chr);
    sprintf(this->loc_str, "%s_%d", this->chr, this->bp);
}

Input::Input(const char *path) {
    // Open the file for reading
    this->fh.open(path, ifstream::in);

    if (this->fh.fail()) 
        cerr << "Error opening input file '" << path << "'\n";
}

Input::~Input() {
    // Close the file
    this->fh.close();
}

int parse_tsv(const char *line, vector<string> &parts) {
    const char  *p, *q;
    string part;

    parts.clear();
    p = line;

    do {
	for (q = p; *q != '\t' && *q != '\0'; q++);
	if (q - p == 0) 
	    part = "";
	else
	    part.assign(p, (q - p));
	parts.push_back(part);

	p = q + 1;
    } while (*q != '\0');

    //for (size_t i = 0; i < parts.size(); i++)
    //    cerr << "Parts[" << i << "]: " << parts[i].c_str() << "\n";
    //cerr << "\n";

    return 0;
}
