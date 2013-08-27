// -*-mode:c++; c-style:k&r; c-basic-offset:4;-*-
//
// Copyright 2011-2013, Julian Catchen <jcatchen@uoregon.edu>
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
// clean.cc -- common routines for processing and cleaning raw seqeunce data.
//
// Julian Catchen
// jcatchen@uoregon.edu
// University of Oregon
//
// $Id: clean.cc 2146 2011-08-02 22:11:50Z catchen $
//

#include "clean.h"

int parse_illumina_v1(const char *file) {
    const char *p, *q;

    //
    // Parse a file name that looks like: s_7_1_0001_qseq.txt ... s_7_1_0120_qseq.txt
    // but exclude the paired-end files:  s_7_2_0001_qseq.txt ... s_7_2_0120_qseq.txt
    //
    if (file[0] != 's') 
	return 0;

    int underscore_cnt = 0;
    for (p = file; *p != '\0'; p++) {
	if (*p == '_') {
	    underscore_cnt++;
	    q = p;
	}
    }

    if (underscore_cnt != 4)
	return 0;

    // Check the file suffix.
    if (strncmp(q, "_qseq.txt", 8) != 0)
	return 0;

    // Make sure it is not the paired-end file
    p  = file;
    p += 3;
    if (strncmp(p, "_1_", 3) != 0)
	return 0;

    //
    // Return the position of the paired-end number, so the other file name can be generated.
    //
    return (p + 1 - file);
}

int parse_illumina_v2(const char *file) {
    const char *p, *q;

    //
    // Parse a file name that looks like: lane6_NoIndex_L006_R1_003.fastq
    // but exclude the paired-end files:  lane6_NoIndex_L006_R2_003.fastq
    //
    // Another example could be:          GfddRAD1_001_ATCACG_L008_R1_001.fastq.gz
    // and excluding the paired-end file: GfddRAD1_001_ATCACG_L008_R2_001.fastq.gz

    //
    // Make sure it ends in "fastq" or "fastq.gz"
    //
    for (q = file; *q != '\0'; q++);
    for (p = q; *p != '.' && p > file; p--);
    if (strncmp(p, ".gz", 3) == 0)
	for (p--; *p != '.' && p > file; p--);
    if (strncmp(p, ".fastq", 6) != 0)
	return 0;

    //
    // Find the part of the name marking the pair, "_R1_", make sure it is not the paired-end file.
    //
    p = file;
    while (p != '\0') {
	for (; *p != '_' && *p != '\0'; p++);
	if (*p == '\0') return 0;
	if (strncmp(p, "_R1_", 4) == 0) {
	    //
	    // Return the position of the paired-end number, so the other file name can be generated.
	    //
	    return (p + 2 - file); 
	}
	p++;
    }

    return 0;
}

int parse_input_record(Seq *s, Read *r) {
    char *p, *q;
    //
    // Count the number of colons to differentiate Illumina version.
    // CASAVA 1.8+ has a FASTQ header like this:
    //  @HWI-ST0747:155:C01WHABXX:8:1101:6455:26332 1:N:0:
    // Or, with the embedded barcode:
    //  @HWI-ST1233:67:D12GNACXX:7:2307:14604:78978 1:N:0:ATCACG
    //
    //
    // Or, parse FASTQ header from previous versions that looks like this:
    //  @HWI-ST0747_0141:4:1101:1240:2199#0/1
    //  @HWI-ST0747_0143:2:2208:21290:200914#0/1
    //
    char *stop = s->id + strlen(s->id);
    int   colon_cnt = 0;
    int   hash_cnt  = 0;

    for (p = s->id, q = p; q < stop; q++) {
	colon_cnt += *q == ':' ? 1 : 0;
	hash_cnt  += *q == '#' ? 1 : 0;
    }

    if (colon_cnt == 9 && hash_cnt == 0) {
	r->fastq_type = illv2_fastq;
	//
	// According to Illumina manual, "CASAVA v1.8 User Guide" page 41:
	// @<instrument>:<run number>:<flowcell ID>:<lane>:<tile>:<x-pos>:<y-pos> <read>:<is filtered>:<control number>:<index sequence>
	//
	for (p = s->id, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	strcpy(r->machine, p);
	*q = ':';

	// Run number.
	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	//*q = '\0';

	// Flowcell ID.
	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	//*q = '\0';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->lane = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->tile = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->x = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != ' ' && q < stop; q++);
	*q = '\0';
	r->y = atoi(p);
	*q = ' ';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->read = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->filter = *p == 'Y' ? true : false;
	*q = ':';

	// Control Number.
	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	//*q = '\0';

	//
	// Index barcode
	//
	for (p = q+1, q = p; q < stop; q++);

	if (*p != '\0' && r->read == 1) {
	    switch (barcode_type) {
	    case index_null:
	    case index_index:
	    case index_inline:
		strncpy(r->index_bc, p,  bc_size_1);
		r->index_bc[bc_size_1] = '\0';
		break;
	    case inline_index:
		strncpy(r->index_bc, p,  bc_size_2);
		r->index_bc[bc_size_2] = '\0';
		break;
	    default:
		break;
	    }
	} else if (*p != '\0' && r->read == 2) {
	    switch (barcode_type) { 
	    case index_index:
	    case inline_index:
		strncpy(r->index_bc, p,  bc_size_2);
		r->index_bc[bc_size_2] = '\0';
		break;
	    default:
		break;
	    }
	}

    } else if (colon_cnt == 4 && hash_cnt == 1) {
	r->fastq_type = illv1_fastq;

	for (p = s->id, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	strcpy(r->machine, p);
	*q = ':';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->lane = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->tile = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != ':' && q < stop; q++);
	*q = '\0';
	r->x = atoi(p);
	*q = ':';

	for (p = q+1, q = p; *q != '#' && q < stop; q++);
	*q = '\0';
	r->y = atoi(p);
	*q = '#';

	for (p = q+1, q = p; *q != '/' && q < stop; q++);
	*q = '\0';
	r->index = atoi(p);
	*q = '/';

	for (p = q+1, q = p; *q != '\0' && q < stop; q++);
	r->read = atoi(p);

    } else {
	r->fastq_type = generic_fastq;

	strncpy(r->machine, s->id, id_len);
	r->machine[id_len] = '\0';
    }

    uint len = strlen(s->seq);

    //
    // Resize the sequence/phred buffers if necessary.
    //
    if (truncate_seq == 0 && len > r->size - 1)
	r->resize(len + 1);
    else if (truncate_seq == 0)
	r->set_len(len);

    strncpy(r->seq,   s->seq,  r->len); 
    r->seq[r->len]   = '\0';
    strncpy(r->phred, s->qual, r->len);
    r->phred[r->len] = '\0';

    if (r->read == 1) {
	    switch (barcode_type) {
	    case inline_null:
	    case inline_inline:
	    case inline_index:
		strncpy(r->inline_bc, r->seq, bc_size_1);
		r->inline_bc[bc_size_1] = '\0';
		break;
	    case index_inline:
		strncpy(r->inline_bc, r->seq, bc_size_2);
		r->inline_bc[bc_size_2] = '\0';
		break;
	    default:
		break;
	    }
    } else if (r->read == 2 &&
	       (barcode_type == inline_inline ||
		barcode_type == index_inline)) {
	strncpy(r->inline_bc, r->seq, bc_size_2);
	r->inline_bc[bc_size_2] = '\0';
    }

    r->retain = 1;

    return 0;
}

int 
rev_complement(char *seq, int offset, bool overhang) 
{
    char *p, *q;

    offset += overhang ? 1 : 0;
    q       = seq + offset;

    int len   = strlen(q);
    int j     = 0;
    char *com = new char[len + 1]; 
   
    for (p = q + len - 1; p >= q; p--) {
        switch (*p) {
        case 'A':
        case 'a':
            com[j] = 'T';
            break;
        case 'C':
        case 'c':
            com[j] = 'G';
            break;
        case 'G':
        case 'g':
            com[j] = 'C';
            break;
        case 'T':
        case 't':
            com[j] = 'A';
            break;
        }
        j++;
    }
    com[len] = '\0';

    for (j = 0; j < len; j++)
	q[j] = com[j];

    delete [] com;

    return 0;
}

int 
reverse_qual(char *qual, int offset, bool overhang) 
{
    char *p, *q;

    offset += overhang ? 1 : 0;
    q       = qual + offset;

    int len   = strlen(q);
    int j     = 0;
    char *com = new char[len + 1]; 
   
    for (p = q + len - 1; p >= q; p--) {
	com[j] = *p;
        j++;
    }
    com[len] = '\0';

    for (j = 0; j < len; j++)
	q[j] = com[j];

    delete [] com;

    return 0;
}

//
// Functions for quality filtering based on phred scores.
//
int 
check_quality_scores(Read *href, int qual_offset, int score_limit, int len_limit, int offset) 
{
    //
    // Phred quality scores are discussed here:
    //  http://en.wikipedia.org/wiki/FASTQ_format
    //
    // Illumina 1.3+ encodes phred scores between ASCII values 64 (0 quality) and 104 (40 quality)
    //
    //   @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefgh
    //   |         |         |         |         |
    //  64        74        84        94       104
    //   0        10(90%)   20(99%)   30(99.9%) 40(99.99%)
    //
    //
    // Drop sequence if the average phred quality score drops below a threshold within a sliding window.
    //

    //
    // Convert the encoded quality scores to their integer values
    //
    // cerr << "Integer scores: ";
    for (uint j = 0; j < href->len; j++) {
        href->int_scores[j] = href->phred[j] - qual_offset;
	// cerr << href->int_scores[j] << " ";
    }
    // cerr << "\n";

    double mean        = 0.0;
    double working_sum = 0.0;
    int *p, *q, j;

    // cerr << "Window length: " << href->win_len << "; Stop position: " << href->stop_pos << "\n";
    //
    // Populate the sliding window.
    //
    for (j = offset; j < href->win_len + offset; j++)
    	working_sum += href->int_scores[j];

    // cerr << "Populating the sliding window using position " << offset << " to " << href->win_len + offset - 1 << "; initial working sum: " << working_sum << "\n";

    //
    // Set p pointer to the first element in the window, and q to one element past the last element in the window.
    //
    p = href->int_scores + offset;
    q = p + (int) href->win_len;
    j = offset;

    // cerr << "Setting pointers; P: " << (href->int_scores + offset) - href->int_scores << "; Q: " << p + (int) href->win_len - p << "; J: " << j << "\n";

    do {
    	mean = working_sum / href->win_len;

	// cerr << "J: " << j << "; Window contents: ";
	// for (int *r = p; r < q; r++)
	//     cerr << *r << " ";
	// cerr << "\n";
	// cerr << "    Mean: " << mean << "\n";

    	if (mean < score_limit) {

	    if (j < len_limit) {
		return 0;
	    } else {
		href->len      = j + 1;
		href->seq[j]   = '\0';
		href->phred[j] = '\0';
		return -1;
	    }
    	}

	//
    	// Advance the window:
    	//   Add the score from the front edge of the window, subtract the score
    	//   from the back edge of the window.
    	//
    	working_sum -= (double) *p;
    	working_sum += (double) *q;

	// cerr << "  Removing value of p: " << *p << " (position: " << p - (href->int_scores) << ")\n";
	// cerr << "  Adding value of q: " << *q << " (position: " << q - (href->int_scores) << ")\n";

    	p++;
    	q++;
    	j++;
    } while (j <= href->stop_pos);

    return 1;
}

//
// Funtion to process barcodes
//
int 
process_barcode(Read *href_1, Read *href_2, BarcodePair &bc, 
		map<BarcodePair, ofstream *> &fhs,
		set<string> &se_bc, set<string> &pe_bc, 
		map<BarcodePair, map<string, long> > &barcode_log, map<string, long> &counter) 
{
    if (barcode_type == null_null)
	return 0;

    //
    // Log the barcodes we receive.
    //
    if (barcode_log.count(bc) == 0) {
	barcode_log[bc]["noradtag"] = 0;
	barcode_log[bc]["total"]    = 0;
	barcode_log[bc]["low_qual"] = 0;
	barcode_log[bc]["retained"] = 0;
    }
    barcode_log[bc]["total"] += paired ? 2 : 1;

    bool se_correct, pe_correct;

    //
    // Is this a legitimate barcode?
    //
    if (fhs.count(bc) == 0) {
	BarcodePair old_barcode = bc;

    	//
    	// Try to correct the barcode.
    	//
	if (paired) {
	    if (se_bc.count(bc.se) == 0)
		se_correct = correct_barcode(se_bc, href_1, single_end);
	    if (pe_bc.size() > 0 && pe_bc.count(bc.pe) == 0)
		pe_correct = correct_barcode(pe_bc, href_2, paired_end);

	    if (se_correct)
		bc.se = string(href_1->se_bc);
	    if (pe_bc.size() > 0 && pe_correct)
		bc.pe = string(href_2->pe_bc);
	    //
	    // After correcting the individual barcodes, check if the combination is valid.
	    //
	    if (fhs.count(bc) == 0) {
		counter["ambiguous"] += 2;
		href_1->retain = 0;
		href_2->retain = 0;
	    }

	} else {
	    if (se_bc.count(bc.se) == 0)
		se_correct = correct_barcode(se_bc, href_1, single_end);
	    if (pe_bc.size() > 0 && pe_bc.count(bc.pe) == 0)
		pe_correct = correct_barcode(pe_bc, href_1, paired_end);

	    if (se_correct)
		bc.se = string(href_1->se_bc);
	    if (pe_bc.size() > 0 && pe_correct)
		bc.pe = string(href_1->pe_bc);

	    if (fhs.count(bc) == 0) {
		counter["ambiguous"]++;
		href_1->retain = 0;
	    }
	}

	if (href_1->retain) {
	    counter["recovered"] += paired ? 2 : 1;
	    barcode_log[old_barcode]["total"] -= paired ? 2 : 1;
	    if (barcode_log.count(bc) == 0) {
		barcode_log[bc]["total"]    = 0;
		barcode_log[bc]["retained"] = 0;
		barcode_log[bc]["low_qual"] = 0;
		barcode_log[bc]["noradtag"] = 0;
	    }
	    barcode_log[bc]["total"] += paired ? 2 : 1;
	}
    }

    return 0;
}

bool 
correct_barcode(set<string> &bcs, Read *href, seqt type) 
{
    if (recover == false)
	return 0;

    //
    // The barcode_dist variable specifies how far apart in sequence space barcodes are. If barcodes
    // are off by two nucleotides in sequence space, than we can correct barcodes that have a single
    // sequencing error. 
    // 
    // If the barcode sequence is off by no more than barcodes_dist-1 nucleotides, correct it.
    //
    const char *p; char *q;
    int d, close;
    string b;
    set<string>::iterator it;

    int num_errs = barcode_dist - 1;
    close = 0;

    for (it = bcs.begin(); it != bcs.end(); it++) {

	d = 0; 
	for (p = it->c_str(), q = type == single_end ? href->se_bc : href->pe_bc; *p != '\0'; p++, q++)
	    if (*p != *q) d++;

	if (d <= num_errs) {
	    close++;
	    b = *it;
	    break;
	}
    }

    if (close == 1) {
	//
	// Correct the barcode.
	//
	if (type == single_end)
	    strcpy(href->se_bc, b.c_str());
	else 
	    strcpy(href->pe_bc, b.c_str());

	return true;
    }

    return false;
}

//
// Functions for filtering adapter sequence
//
int
init_adapter_seq(int kmer_size, char *adapter, int &adp_len, AdapterHash &kmers)
{
    string kmer;
    adp_len = strlen(adapter);

    int   num_kmers = adp_len - kmer_size + 1;
    char *p = adapter;
    for (int i = 0; i < num_kmers; i++) {
	kmer.assign(p, kmer_size);
	kmers[kmer].push_back(i);
	p++;
    }

    return 0;
}

int 
filter_adapter_seq(Read *href, char *adapter, int adp_len, AdapterHash &adp_kmers,
		   int kmer_size, int distance, int len_limit) 
{
    vector<pair<int, int> > hits;
    int   num_kmers = href->len - kmer_size + 1;
    const char *p   = href->seq;
    string kmer;

    //
    // Identify matching kmers and their locations of occurance.
    //
    for (int i = 0; i < num_kmers; i++) {
	kmer.assign(p, kmer_size);

	if (adp_kmers.count(kmer) > 0) {
	    for (uint j = 0; j < adp_kmers[kmer].size(); j++) {
		// cerr << "Kmer hit " << kmer << " at query position " << i << " at hit position " << adp_kmers[kmer][j] << "\n";
		hits.push_back(make_pair(i, adp_kmers[kmer][j]));
	    }
	}
	p++;
    }

    //
    // Scan backwards from the position of the k-mer and then scan forwards 
    // counting the number of mismatches.
    //
    int mismatches, i, j, start_pos;

    for (uint k = 0; k < hits.size(); k++) {
	mismatches = 0;
	i = hits[k].first;  // Position in query sequence
	j = hits[k].second; // Position in adapter hit

	// cerr << "Starting comparison at i: "<< i << "; j: " << j << "\n";

	while (i >= 0 && j >= 0) {
	    if (href->seq[i] != adapter[j])
		mismatches++;
	    i--;
	    j--;
	}

	if (mismatches > distance)
	    continue;

	start_pos = i + 1;
	i = hits[k].first;
	j = hits[k].second;

	while (i < (int) href->len && j < adp_len && mismatches <= distance) {
	    if (href->seq[i] != adapter[j])
		mismatches++;
	    i++;
	    j++;
	}

	// cerr << "Starting position: " << start_pos << "; Query end (i): " << i << "; adapter end (j): " << j 
	//      << "; number of mismatches: " << mismatches << "; Seq Len: " << href->len << "; SeqSeq Len: " << strlen(href->seq) << "\n";

	if (mismatches <= distance && (i == (int) href->len || j == adp_len)) {
	    // cerr << "  Trimming or dropping.\n";
	    if (start_pos < len_limit) {
		return 0;
	    } else {
		href->len = start_pos + 1;
		href->seq[start_pos]   = '\0';
		href->phred[start_pos] = '\0';
		return -1;
	    }
	}
    }

    return 1;
}
