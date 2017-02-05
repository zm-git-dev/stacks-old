// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// Copyright 2013-2016, Julian Catchen <jcatchen@illinois.edu>
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

#ifndef __BAMI_H__
#define __BAMI_H__

//
// Code to parse binary BAM format. This format is created for
// reads that have been aligned to a reference genome.
//

#ifdef HAVE_BAM

#include <algorithm>
#include <string>
#include <vector>
#include <array>

#include "sam.h" //htslib

#include "stacks.h"
#include "input.h"
#include "DNASeq4.h"

int bam_find_start_bp(int, strand_type, const vector<pair<char, uint> > &);
int bam_edit_gaps(vector<pair<char, uint> > &, char *);

class BamRecord {
    bam1_t* r_;
    bam1_core_t& c_; // r_->core

public:
    BamRecord() : r_(bam_init1()), c_(r_->core) {}
    ~BamRecord() {bam_destroy1(r_);}
    bam1_t* r() {return r_;}

    string qname() const {return string(bam_get_qname(r_), c_.l_qname-1);}
    uint16_t flag() const {return c_.flag;}
    int32_t chrom() const {return c_.tid;}
    int32_t pos() const {return c_.pos;}
    uint8_t mapq() const {return c_.qual;}
    vector<pair<char, uint>> cigar() const;
    // (rnext)
    // (pnext)
    // (tlen)
    DNASeq4 seq() const {return DNASeq4((uchar*) bam_get_seq(r_), c_.l_qseq);}
    // (qual)
    // (aux)

    bool is_mapped() const {return c_.flag & BAM_FUNMAP;}
    bool is_rev_compl() const {return c_.flag & BAM_FREVERSE;}
    bool is_paired() const {return c_.flag & BAM_FPAIRED;}
    bool is_fw_read() const {return c_.flag & BAM_FREAD1;}
    bool is_rev_read() const {return c_.flag & BAM_FREAD2;}

    AlnT aln_type() const;
    const char* read_group() const;

private:
    // Moves the pointer to the start of the next AUX field. Doesn't actually read
    // anything, the point is just to be able to scan until field(s) of interest.
    // Returns `ptr`.
    static void skip_one_aux(const uint8_t* ptr);
};

class Bam: public Input {
private:
    htsFile   *bam_fh;
    bam_hdr_t *bamh;
    BamRecord rec;

    map<uint, string> chrs;

    int parse_header();
    void parse_cigar(vector<pair<char, uint> > &);

    bool next_record() {return sam_read1(bam_fh, bamh, rec.r()) >= 0;}

public:
    Bam(const char *path) : Input(), bam_fh(NULL), bamh(NULL), rec() {
        this->path   = string(path);
        bam_fh = hts_open(path, "r");

        parse_header();
    };
    ~Bam() {
        hts_close(bam_fh);
        bam_hdr_destroy(bamh);
    };
    Seq *next_seq();
    int  next_seq(Seq&);
};

//
// Inline definitions
// ----------
//

inline
int
Bam::parse_header()
{
    this->bamh = bam_hdr_init();
    this->bamh = sam_hdr_read(this->bam_fh);

    for (uint j = 0; j < (uint) this->bamh->n_targets; j++) {
        //
        // Record the mapping from integer ID to chromosome name that we will see in BAM records.
        //
        this->chrs[j] = string(this->bamh->target_name[j]);
    }

    return 0;
}

inline
Seq *
Bam::next_seq()
{
    Seq* s = new Seq();
    if(next_seq(*s) != 1) {
        delete s;
        s = NULL;
    }
    return s;
}

inline
int
Bam::next_seq(Seq& s)
{
    //
    // Read a record
    //
    if (!next_record())
        return false;

    //
    // Fetch the sequence.
    //
    string  seq;
    seq.reserve(rec.r()->core.l_qseq);
    for (int i = 0; i < rec.r()->core.l_qseq; i++) {
        uint8_t j = bam_seqi(bam_get_seq(rec.r()), i);
        switch(j) {
        case 1:
            seq += 'A';
            break;
        case 2:
            seq += 'C';
            break;
        case 4:
            seq += 'G';
            break;
        case 8:
            seq += 'T';
            break;
        case 15:
            seq += 'N';
            break;
        default:
            // assert(false);
            break;
        }
    }

    //
    // Fetch the quality score.
    //
    string   qual;
    uint8_t *q = bam_get_qual(rec.r());
    for (int i = 0; i < rec.r()->core.l_qseq; i++) {
        qual += char(int(q[i]) + 33);
    }

    if (!rec.is_mapped()) {
        s = Seq(rec.qname().c_str(), seq.c_str(), qual.c_str());
    } else {
        //
        // Check which strand this is aligned to:
        //   SAM reference: FLAG bit 0x10 - sequence is reverse complemented
        //
        strand_type strand = rec.is_rev_compl() ? strand_minus : strand_plus;

        //
        // Parse the alignment CIGAR string.
        // If aligned to the negative strand, sequence has been reverse complemented and
        // CIGAR string should be interpreted in reverse.
        //
        vector<pair<char, uint>> cigar = rec.cigar();
        if (strand == strand_minus)
            std::reverse(cigar.begin(), cigar.end());

        //
        // If the read was aligned on the reverse strand (and is therefore reverse complemented)
        // alter the start point of the alignment to reflect the right-side of the read, at the
        // end of the RAD cut site.
        //
        uint bp = bam_find_start_bp(rec.pos(), strand, cigar);

        //
        // Calculate the percentage of the sequence that was aligned to the reference.
        //
        uint clipped = 0;
        for (auto& op : cigar)
            if (op.first == 'S')
                clipped += op.second;
        double pct_clipped = (double) clipped / seq.length();

        s = Seq(rec.qname().c_str(), seq.c_str(), qual.c_str(),
                chrs[rec.chrom()].c_str(), bp, strand, rec.aln_type(), pct_clipped, rec.mapq());

        if (cigar.size() > 0)
            bam_edit_gaps(cigar, s.seq);
    }

    return true;
}

inline
int
bam_find_start_bp(int aln_bp, strand_type strand, const vector<pair<char, uint> > &cigar)
{
    if (strand == strand_plus) {
        if (cigar.at(0).first == 'S')
            aln_bp -= cigar.at(0).second;
    } else {
        // assert(strand == strand_minus);
        for (uint i = 0; i < cigar.size(); i++)  {
            char op   = cigar[i].first;
            uint dist = cigar[i].second;

            switch(op) {
            case 'I':
            case 'H':
                break;
            case 'S':
                if (i < cigar.size() - 1)
                    aln_bp += dist;
                break;
            case 'M':
            case '=':
            case 'X':
            case 'D':
            case 'N':
                aln_bp += dist;
                break;
            default:
                break;
            }
        }
        aln_bp -= 1;
    }

    return aln_bp;
}

inline
int
bam_edit_gaps(vector<pair<char, uint> > &cigar, char *seq)
{
    char *buf;
    uint  size = cigar.size();
    char  op;
    uint  dist, bp, len, buf_len, buf_size, j, k, stop;

    len = strlen(seq);
    bp  = 0;

    buf      = new char[len + 1];
    buf_size = len + 1;

    for (uint i = 0; i < size; i++)  {
        op   = cigar[i].first;
        dist = cigar[i].second;

        switch(op) {
        case 'S':
            stop = bp + dist;
            stop = stop > len ? len : stop;
            while (bp < stop) {
                seq[bp] = 'N';
                bp++;
            }
            break;
        case 'D':
            //
            // A deletion has occured in the read relative to the reference genome.
            // Pad the read with sufficent Ns to match the deletion, shifting the existing
            // sequence down. Trim the final length to keep the read length consistent.
            //
            k = bp >= len ? len : bp;

            strncpy(buf, seq + k, buf_size - 1);
            buf[buf_size - 1] = '\0';
            buf_len         = strlen(buf);

            stop = bp + dist;
            stop = stop > len ? len : stop;
            while (bp < stop) {
                seq[bp] = 'N';
                bp++;
            }

            j = bp;
            k = 0;
            while (j < len && k < buf_len) {
                seq[j] = buf[k];
                k++;
                j++;
            }
            break;
        case 'I':
            //
            // An insertion has occurred in the read relative to the reference genome. Delete the
            // inserted bases and pad the end of the read with Ns.
            //
            if (bp >= len) break;

            k = bp + dist > len ? len : bp + dist;
            strncpy(buf, seq + k, buf_size - 1);
            buf[buf_size - 1] = '\0';
            buf_len           = strlen(buf);

            j = bp;
            k = 0;
            while (j < len && k < buf_len) {
                seq[j] = buf[k];
                k++;
                j++;
            }

            stop = j + dist;
            stop = stop > len ? len : stop;
            while (j < stop) {
                seq[j] = 'N';
                j++;
            }
            break;
        case 'M':
        case '=':
        case 'X':
            bp += dist;
            break;
        default:
            break;
        }
    }

    delete [] buf;

    return 0;
}

inline
vector<pair<char, uint>> BamRecord::cigar() const {
    vector<pair<char, uint>> cig;

    uint32_t* hts_cigar = bam_get_cigar(r_);
    for (size_t i = 0; i < c_.n_cigar; i++) {
        char op;
        switch(hts_cigar[i] &  BAM_CIGAR_MASK) {
        case BAM_CMATCH:
            op = 'M';
            break;
        case BAM_CEQUAL:
            op = '=';
            break;
        case BAM_CDIFF:
            op = 'X';
            break;
        case BAM_CINS:
            op = 'I';
            break;
        case BAM_CDEL:
            op = 'D';
            break;
        case BAM_CSOFT_CLIP:
            op = 'S';
            break;
        case BAM_CREF_SKIP:
            op = 'N';
            break;
        case BAM_CHARD_CLIP:
            op = 'H';
            break;
        case BAM_CPAD:
            op = 'P';
            break;
        default:
            cerr << "Warning: Unknown CIGAR operation (current record name is '" << bam_get_qname(r_) << "').\n";
            break;
        }

        cig.push_back({op, hts_cigar[i] >> BAM_CIGAR_SHIFT});
    }

    return cig;
}

inline
AlnT BamRecord::aln_type() const {
    if (c_.flag & BAM_FUNMAP)
        return AlnT::null;
    else if (c_.flag & BAM_FSECONDARY)
        return AlnT::secondary;
    else if (c_.flag & BAM_FSUPPLEMENTARY)
        return AlnT::supplementary;
    else
        return AlnT::primary;
}

inline
const char* BamRecord::read_group() const {
    uint8_t* ptr = bam_get_aux(r_);
    while (ptr < bam_get_aux(r_) + bam_get_l_aux(r_) - 3) { // minimum field size is 4
        if (*(char*)ptr == 'R' && *(char*)(ptr+1) == 'G') {
            return (char*)ptr+3;
        } else {
            skip_one_aux(ptr);
        }
    }

    return NULL;
}

inline
void BamRecord::skip_one_aux(const uint8_t* ptr) {
    using namespace std;

    // The library doesn't provide much for handling the AUX fields.

    // Make sure that the tag matches [A-Za-z][A-Za-z0-9].
    if (!isalpha(*ptr) || !isalnum(*(ptr+1))) {
        cerr << "Warning: Illegal BAM AUX tag '"
             << *(char*)ptr << *(char*)(ptr+1) << "' ("
             << *ptr << "," << *(ptr+1) << ").\n" << flush;
    }
    ptr += 2;

    char t = *(char*)ptr; // byte 3 of the field gives the type
    ptr += 1;

    if (t == 'i'        // int32_t
            || t == 'I' // uint32_t
            || t == 'f' // float
            ) {
        ptr += 4;
    } else if (t == 'Z' // string
            || t == 'H' // hex string
            ) {
        ptr += strlen((char*)ptr) + 1;
    } else if (t == 'A' // character
            || t == 'c' // int8_t
            || t == 'C' // uint8_t
            ) {
        ptr += 1;
    } else if (t == 's' // int16_t
            || t == 'S' // uint16_t
            ) {
        ptr += 2;
    } else if (t == 'B') { // array
        ptr += 1;
        char t2 = *(char*)ptr; // byte 4 gives the array contents type
        ptr += 1;
        int32_t len = *(int32_t*)ptr; // bytes 5-9 give the array length
        ptr += 4;

        // Type should be an integer type or float.
        if (t2 == 'c' || t2 == 'C') {
            ptr += len;
        } else if (t2 == 's' || t2 == 'S') {
            ptr += 2 * len;
        } else if (t2 == 'i' || t2 == 'I' || t2 == 'f') {
            ptr += 4 * len;
        } else {
            cerr << "Error: Unexpected BAM AUX field array type '" << t2 << "' (#" << *(uchar*)&t2 << ").\n";
            throw exception();
        }
    } else {
        cerr << "Error: Unexpected BAM AUX field type '" << t << "' (#" << *(uchar*)&t << ").\n";
        throw exception();
    }
}

#else  // If HAVE_BAM is undefined and BAM library is not present.

#include "input.h"

class Bam: public Input {
 public:
    Bam(const char *path) : Input() { cerr << "BAM support was not enabled when Stacks was compiled.\n"; };
    ~Bam() {};
    Seq *next_seq()      { return NULL; };
    int  next_seq(Seq &) { return 0; };
};

#endif // HAVE_BAM

#endif // __BAMI_H__
