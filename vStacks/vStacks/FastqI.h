// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// Copyright (c) 2014 University of Oregon
// Created by Julian Catchen <jcatchen@uoregon.edu>
//

#ifndef __FASTQI_H__
#define __FASTQI_H__

#include "input.h"

class Fastq: public Input {

public:
    Fastq(const char *path) : Input(path) { };
    Fastq(string path) : Input(path.c_str()) { };
    ~Fastq() {};
    Seq *next_seq();
    int  next_seq(Seq &s);
};

Seq *Fastq::next_seq() {
    //
    // Check the contents of the line buffer. When we finish reading a FASTQ record
    // the buffer will either contain whitespace or the header of the next FASTQ
    // record.
    //
    while (this->line[0] != '@' && this->fh.good() ) {
	this->fh.getline(this->line, max_len);
    }

    if (!this->fh.good()) {
	return NULL;
    }

    //
    // Check if there is a carraige return in the buffer
    //
    uint len = strlen(this->line);
    if (len > 0 && this->line[len - 1] == '\r') this->line[len - 1] = '\0';

    //
    // Initialize the Seq structure and store the FASTQ ID
    //
    Seq *s = new Seq;
    s->id = new char[strlen(this->line) + 1];
    strcpy(s->id, this->line + 1);

    //
    // Read the sequence from the file
    //
    this->fh.getline(this->line, max_len);

    if (!this->fh.good()) {
	return NULL;
    }

    len = strlen(this->line);
    if (len > 0 && this->line[len - 1] == '\r') this->line[len - 1] = '\0';

    s->seq = new char[len + 1];
    strcpy(s->seq, this->line);

    //
    // Read the repeat of the ID
    //
    this->fh.getline(this->line, max_len);

    if (this->line[0] != '+' || !this->fh.good()) {
	return NULL;
    }

    //
    // Read the quality score from the file
    //
    this->fh.getline(this->line, max_len);

    if (!this->fh.good() && !this->fh.eof()) {
	return NULL;
    }

    len = strlen(this->line);
    if (len > 0 && this->line[len - 1] == '\r') this->line[len - 1] = '\0';

    s->qual = new char[len + 1];
    strcpy(s->qual, this->line);

    //
    // Clear the line buffer so it is set up for the next record. If a '@'
    // appears in the quality scores read, it will break parsing next time 
    // it is called.
    //
    this->line[0] = '\0';

    return s;
}

int Fastq::next_seq(Seq &s) {
    //
    // Check the contents of the line buffer. When we finish reading a FASTQ record
    // the buffer will either contain whitespace or the header of the next FASTQ
    // record.
    //
    while (this->line[0] != '@' && this->fh.good() ) {
	this->fh.getline(this->line, max_len);
    }

    if (!this->fh.good()) {
	return 0;
    }

    //
    // Check if there is a carraige return in the buffer
    //
    uint len = strlen(this->line);
    if (this->line[len - 1] == '\r') this->line[len - 1] = '\0';

    //
    // Store the FASTQ ID
    //
    strcpy(s.id, this->line + 1);

    //
    // Read the sequence from the file
    //
    this->fh.getline(this->line, max_len);

    if (!this->fh.good()) {
	return 0;
    }

    len = strlen(this->line);
    if (this->line[len - 1] == '\r') this->line[len - 1] = '\0';

    strcpy(s.seq, this->line);

    //
    // Read the repeat of the ID
    //
    this->fh.getline(this->line, max_len);

    if (this->line[0] != '+' || !this->fh.good()) {
	return 0;
    }

    //
    // Read the quality score from the file
    //
    this->fh.getline(this->line, max_len);

    if (!this->fh.good() && !this->fh.eof()) {
	return 0;
    }

    len = strlen(this->line);
    if (this->line[len - 1] == '\r') this->line[len - 1] = '\0';

    strcpy(s.qual, this->line);

    //
    // Clear the line buffer so it is set up for the next record. If a '@'
    // appears in the quality scores read, it will break parsing next time 
    // it is called.
    //
    this->line[0] = '\0';

    return 1;
}

#endif // __FASTQI_H__