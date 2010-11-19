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
// pstacks -- search an existing set of stacks for polymorphisms
//
// Julian Catchen
// jcatchen@uoregon.edu
// University of Oregon
//
// $Id$
//

#include "pstacks.h"

//
// Global variables to hold command-line options.
//
file_type in_file_type;
string    in_file;
file_type out_file_type;
string    out_path;
int       sql_id      = 0;
int       batch_id    = 0;
int       num_threads = 1;

//
// For use with the multinomial model to call fixed nucleotides.
//
modelt model_type        = snp;
double p_freq            = 0.5;
double barcode_err_freq  = 0.0;

int main (int argc, char* argv[]) {

    parse_command_line(argc, argv);

    //
    // Set the number of OpenMP parallel threads to execute.
    //
    #ifdef _OPENMP
    omp_set_num_threads(num_threads);
    #endif

    HashMap           radtags;
    set<int>          merge_map;
    map<int, Stack *> unique;

    load_radtags(in_file, radtags);

    reduce_radtags(radtags, unique);

    //dump_stacks(unique);

    map<int, MergedStack *> merged;

    populate_merged_tags(unique, merged);

    //dump_merged_stacks(merged);

    // Call the consensus sequence again, now that remainder tags have been merged.
    call_consensus(merged, unique, true);

    count_raw_reads(unique, merged);

    cerr << "Writing results\n";
    if (out_file_type == sam)
        write_sam(merged, unique);
    else
        write_sql(merged, unique);

    return 0;
}

int call_alleles(MergedStack *mtag, vector<char *> &reads) {
    int     row;
    int     height = reads.size();
    string  allele;
    char   *base;
    vector<SNP *>::iterator snp;

    if (mtag->snps.size() == 0)
	return 0;

    for (row = 0; row < height; row++) {
	allele.clear();

	for (snp = mtag->snps.begin(); snp != mtag->snps.end(); snp++) {
	    base = reads[row];
	    base = base + (*snp)->col;	    

	    //
	    // Check to make sure the nucleotide at the location of this SNP is
	    // of one of the two possible states the multinomial model called.
	    //
	    if (*base == (*snp)->rank_1 || *base == (*snp)->rank_2) 
		allele += *base;
	    else
		break;
	}

	if (allele.size() == mtag->snps.size())
	    mtag->alleles[allele]++;
    }

    return 0;
}

int call_consensus(map<int, MergedStack *> &merged, map<int, Stack *> &unique, bool invoke_model) {
    //
    // OpenMP can't parallelize random access iterators, so we convert
    // our map to a vector of integer keys.
    //
    map<int, MergedStack *>::iterator it;
    vector<int> keys;
    for (it = merged.begin(); it != merged.end(); it++) 
	keys.push_back(it->first);

    int i;
    #pragma omp parallel private(i)
    { 
        #pragma omp for schedule(dynamic) 
	for (i = 0; i < (int) keys.size(); i++) {
	    MergedStack *mtag;
	    Stack *utag;

	    mtag = merged[keys[i]];

	    //
	    // Create a two-dimensional array, each row containing one read. For
	    // each unique tag that has been merged together, add the sequence for
	    // that tag into our array as many times as it originally occurred. 
	    //
	    vector<int>::iterator j;
	    vector<char *> reads;

	    for (j = mtag->utags.begin(); j != mtag->utags.end(); j++) {
		utag = unique[*j];

		for (uint k = 0; k < utag->count; k++) {
		    reads.push_back(utag->seq);
		}
	    }

	    //
	    // Iterate over each column of the array and call the consensus base.
	    //
	    int row, col;
	    int length = strlen(reads[0]);
	    int height = reads.size();
	    string con;
	    map<char, int> nuc;
	    map<char, int>::iterator max, n;
	    char *base, c;

	    for (col = 0; col < length; col++) {
		nuc['A'] = 0;
		nuc['C'] = 0;
		nuc['G'] = 0;
		nuc['T'] = 0;

		for (row = 0; row < height; row++) {
		    base = reads[row];
		    c    = toupper(*(base + col));
		    //cerr << "    Row: " << row << " Col: " << col << " Base: " << c << "\n";

                    if (c != 'A' && c != 'C' && c != 'G' && c != 'T')
                        continue;

		    nuc[c]++;
		}

		//
		// Find the base with a plurality of occurances and call it.
		//
		max = nuc.end();

		for (n = nuc.begin(); n != nuc.end(); n++) {

		    if (max == nuc.end() || n->second > max->second)
			max = n;
		}
		con += max->first;

		// Search this column for the presence of a SNP
		if (invoke_model) 
		    model_type == snp ? 
                        call_multinomial_snp(mtag, col, nuc) :
                        call_multinomial_fixed(mtag, col, nuc);
	    }

	    if (invoke_model) {
		call_alleles(mtag, reads);

                if (model_type == fixed) {
                    //
                    // Mask nucleotides that are not fixed.
                    //
                    vector<SNP *>::iterator s;
                    for (s = mtag->snps.begin(); s != mtag->snps.end(); s++) {
                        con.replace((*s)->col, 1, "N");
                    }
                }
            }

	    mtag->add_consensus(con.c_str());
	}
    }

    return 0;
}

int count_raw_reads(map<int, Stack *> &unique, map<int, MergedStack *> &merged) {
    map<int, MergedStack *>::iterator it;
    vector<int>::iterator k;
    Stack *tag;
    long int m = 0;

    for (it = merged.begin(); it != merged.end(); it++) {
	for (k = it->second->utags.begin(); k != it->second->utags.end(); k++) {
	    tag  = unique[*k];
	    m   += tag->count;
	}
        m += it->second->remtags.size();
    }

    cerr << "  Number of utilized reads " << m << "\n";

    return 0;
}

int write_sql(map<int, MergedStack *> &m, map<int, Stack *> &u) {
    map<int, MergedStack *>::iterator i;
    vector<SeqId *>::iterator     j;
    vector<int>::iterator           k;
    vector<SNP *>::iterator         s;
    map<string, int>::iterator      t;
    MergedStack *tag_1;
    Stack *tag_2;

    //
    // Parse the input file name to create the output files
    //
    size_t pos_1 = in_file.find_last_of("/");
    size_t pos_2 = in_file.find_last_of(".");
    string tag_file = out_path + in_file.substr(pos_1 + 1, (pos_2 - pos_1 - 1)) + ".tags.tsv";
    string snp_file = out_path + in_file.substr(pos_1 + 1, (pos_2 - pos_1 - 1)) + ".snps.tsv";
    string all_file = out_path + in_file.substr(pos_1 + 1, (pos_2 - pos_1 - 1)) + ".alleles.tsv";

    // Open the output files for writing.
    std::ofstream tags(tag_file.c_str());
    std::ofstream snps(snp_file.c_str());
    std::ofstream alle(all_file.c_str());
    int id;

    for (i = m.begin(); i != m.end(); i++) {
	float total = 0;
	tag_1 = i->second;

	// First write the consensus sequence
	tags << "0" << "\t" 
	     << sql_id << "\t" 
	     << tag_1->id << "\t" 
             << tag_1->loc.chr << "\t"
             << tag_1->loc.bp << "\t"
	     << "consensus\t" << "\t\t" 
	     << tag_1->con << "\t" 
	     << tag_1->deleveraged << "\t" 
	     << tag_1->blacklisted << "\t"
	     << tag_1->lumberjackstack << "\n";

	// Now write out the components of each unique tag merged into this one.
	id = 0;
	for (k = tag_1->utags.begin(); k != tag_1->utags.end(); k++) {
	    tag_2  = u[*k];
	    total += tag_2->count;

	    for (j = tag_2->map.begin(); j != tag_2->map.end(); j++) {
		tags << "0" << "\t" << sql_id << "\t" << tag_1->id << "\t\t\t" << "primary\t" << id << "\t" << (*j)->id << "\t" << tag_2->seq << "\t\t\t\n";
	    }

	    id++;
	}

	// Write out any SNPs detected in this unique tag.
	for (s = tag_1->snps.begin(); s != tag_1->snps.end(); s++) {
	    snps << "0" << "\t" << sql_id << "\t" << tag_1->id << "\t" << (*s)->col << "\t" << (*s)->lratio << "\t" << (*s)->rank_1 << "\t" << (*s)->rank_2 << "\n";
	}

	// Write the expressed alleles seen for the recorded SNPs and
	// the percentage of tags a particular allele occupies.
        char pct[id_len];
	for (t = tag_1->alleles.begin(); t != tag_1->alleles.end(); t++) {
            sprintf(pct, "%.2f", ((t->second/total) * 100));
	    alle << "0" << "\t" << sql_id << "\t" << tag_1->id << "\t" << t->first << "\t" << pct << "\t" << t->second << "\n";
	}
    }

    tags.close();
    snps.close();
    alle.close();

    return 0;
}

int write_sam(map<int, MergedStack *> &m, map<int, Stack *> &u) {
    map<int, MergedStack *>::iterator i;
    vector<SeqId *>::iterator     j;
    vector<int>::iterator           k;
    vector<SNP *>::iterator         s;
    map<string, int>::iterator      t;
    MergedStack *tag_1;

    //
    // Parse the input file name to create the output files
    //
    size_t pos_1 = in_file.find_last_of("/");
    size_t pos_2 = in_file.find_last_of(".");
    string sam_file = out_path + in_file.substr(pos_1 + 1, (pos_2 - pos_1 - 1)) + ".pstacks.sam";

    // Open the output files for writing.
    std::ofstream sam(sam_file.c_str());

    for (i = m.begin(); i != m.end(); i++) {
	tag_1 = i->second;

	// First write the consensus sequence
	sam << tag_1->id << "\t"
            << "\t"  // direction
            << tag_1->loc.chr << "\t"
            << tag_1->loc.bp<< "\t"
            << "\t"  // 
            << "\t"  // 
            << "\t"  // 
            << "\t"  // 
            << "\t"  // 
            << tag_1->con << "\t"
            << "\t"  // quality
            << "\t";  // 
        //
        // Write out any SNPs detected in this unique tag.
        //
        stringstream snps;
	for (s = tag_1->snps.begin(); s != tag_1->snps.end(); s++)
	    snps << (*s)->col << ":" << (*s)->rank_1 << ">" << (*s)->rank_2 << ",";

        sam << snps.str().substr(0, snps.str().length() - 1) << "\n"; //             
    }

    sam.close();

    return 0;
}

int populate_merged_tags(map<int, Stack *> &unique, map<int, MergedStack *> &merged) {
    map<int, Stack *>::iterator i;
    map<int, MergedStack *>::iterator it_new, it_old;
    vector<SeqId *>::iterator j;
    map<string, set<int> > locations;
    map<string, set<int> >::iterator k;
    set<int>::iterator s;
    char         id[id_len];
    Stack       *u;
    MergedStack *m;
    int global_id = 0;

    //
    // Create a map of each unique Stack that has been aligned to the same genomic location.
    //
    for (i = unique.begin(); i != unique.end(); i++) {
        snprintf(id, id_len - 1, "%s_%d", i->second->loc.chr, i->second->loc.bp);
        locations[id].insert(i->second->id);
    }

    it_old = merged.begin();

    for (k = locations.begin(); k != locations.end(); k++) {
	m = new MergedStack;
	m->id = global_id;

        // 
        // Record the consensus and physical location for this stack.
        //
        s = k->second.begin();
	m->add_consensus(unique[*s]->seq);
        strncpy(m->loc.chr, unique[*s]->loc.chr, id_len - 1);
        m->loc.chr[id_len] = '\0';
        m->loc.bp = unique[*s]->loc.bp;

        //
        // Record the individual stacks that were aligned together.
        //
        for (; s != k->second.end(); s++) {
            u = unique[*s];
            m->count += u->count;
            m->utags.push_back(u->id);
        }

	// Insert the new MergedStack giving a hint as to which position
	// to insert it at.
	it_new = merged.insert(it_old, pair<int, MergedStack *>(global_id, m));
	it_old = it_new;
	global_id++;
    }

    cerr << "  Merged " << unique.size() << " unique Stacks into " << merged.size() << " loci.\n";

    return 0;
}

//
// This function assumes that there may be identical reads, mapped to multiple
// places in the genome. In this case, reads are broken down by read ID
// and split into different Stack objects.
//
int reduce_radtags(HashMap &radtags, map<int, Stack *> &unique) {
    HashMap::iterator it;
    vector<Seq *>::iterator sit;
    
    Stack *u;
    int    global_id = 1;

    for (it = radtags.begin(); it != radtags.end(); it++) {
        //
        // Make sure there aren't any reads of identical sequence that have been mapped to
        // different genomic locations.
        //
        map<string, int> locations;
        map<string, int>::iterator lit;
        for (sit = (*it).second.begin(); sit != (*it).second.end(); sit++)
            locations[(*sit)->loc_str]++;

        for (lit = locations.begin(); lit != locations.end(); lit++) {
            //
            // Populate a Stack object for this unique radtag.
            //
            u        = new Stack;
            u->id    = global_id;
            u->count = lit->second;
            u->add_seq(it->first);

            //
            // Record the physical location of this stack.
            //
            for (sit = (*it).second.begin(); sit != (*it).second.end(); sit++) {
                if (strcmp((*sit)->loc_str, lit->first.c_str()) == 0) {
                    u->add_id((*sit)->id);
                    strncpy(u->loc.chr, (*sit)->chr, id_len - 1);
                    u->loc.chr[id_len] = '\0';
                    u->loc.bp = (*sit)->bp;
                }
            }

            unique[u->id] = u;
            global_id++;
        }
    }

    return 0;
}

//
// We expect tags to have already been aligned to a reference genome. Therefore, the tags
// are identified by their chromosome and basepair location.
//
int load_radtags(string in_file, HashMap &radtags) {
    Input *fh;
    Seq *c;

    if (in_file_type == tsv)
        fh = new Tsv(in_file.c_str());
    else if (in_file_type == bowtie)
        fh = new Bowtie(in_file.c_str());
    else if (in_file_type == sam)
        fh = new Sam(in_file.c_str());
    else if (in_file_type == fasta)
        fh = new Fasta(in_file.c_str());
    else if (in_file_type == fastq)
        fh = new Fastq(in_file.c_str());

    cerr << "  Parsing " << in_file.c_str() << "\n";

    int i = 1;
    while ((c = fh->next_seq()) != NULL) {
        if (i % 10000 == 0) cerr << "Loading aligned sequence " << i << "       \r";

	radtags[c->seq].push_back(c);
        i++;
    }

    cerr << "  " <<
        "Analyzed " << i - 1 << " sequences reads; " <<
        "Identified " << radtags.size() << " unique Stacks from those reads.\n";

    //
    // Close the file and delete the Input object.
    //
    delete fh;

    return 0;
}

int dump_stacks(map<int, Stack *> &u) {
    map<int, Stack *>::iterator it;
    vector<SeqId *>::iterator fit;
    vector<pair<int, int> >::iterator pit;
    vector<int>::iterator mit;

    for (it = u.begin(); it != u.end(); it++) {

	cerr << "Stack ID: " << (*it).second->id << "\n"
	     << "  Seq:    " << (*it).second->seq << "\n"
	     << "  IDs:    "; 

	for (fit = (*it).second->map.begin(); fit != (*it).second->map.end(); fit++)
	    cerr << (*fit)->id << " ";

	cerr << "\n\n";
    }

    return 0;
}

int dump_merged_stacks(map<int, MergedStack *> &m) {
    map<int, MergedStack *>::iterator it;
    vector<pair<int, int> >::iterator pit;
    vector<int>::iterator fit;

    for (it = m.begin(); it != m.end(); it++) {

	cerr << "MergedStack ID: " << it->second->id << "\n"
	     << "  Consensus:  ";
	if (it->second->con != NULL)
	    cerr << it->second->con << "\n";
	else 
	    cerr << "\n";
	cerr << "  IDs:        "; 

	for (fit = it->second->utags.begin(); fit != it->second->utags.end(); fit++)
	    cerr << (*fit) << " ";

	cerr << "\n"
	     << "  Distances: ";

	for (pit = it->second->dist.begin(); pit != it->second->dist.end(); pit++)
	    cerr << (*pit).first << ": " << (*pit).second << ", ";

	cerr << "\n\n";
    }

    return 0;
}

int parse_command_line(int argc, char* argv[]) {
    int c;
     
    while (1) {
	static struct option long_options[] = {
	    {"help",         no_argument,       NULL, 'h'},
            {"version",      no_argument,       NULL, 'v'},
	    {"infile_type",  required_argument, NULL, 't'},
	    {"outfile_type", required_argument, NULL, 'y'},
	    {"file",         required_argument, NULL, 'f'},
	    {"outpath",      required_argument, NULL, 'o'},
	    {"id",           required_argument, NULL, 'i'},
	    {"batch_id",     required_argument, NULL, 'b'},
	    {"num_threads",  required_argument, NULL, 'p'},
	    {"bc_err_freq",  required_argument, NULL, 'e'},
	    {0, 0, 0, 0}
	};
	
	// getopt_long stores the option index here.
	int option_index = 0;

	c = getopt_long(argc, argv, "hvnf:o:i:b:e:p:s:f:t:y:", long_options, &option_index);
     
	// Detect the end of the options.
	if (c == -1)
	    break;
     
	switch (c) {
	case 'h':
	    help();
	    break;
     	case 't':
            if (strcmp(optarg, "tsv") == 0)
                in_file_type = tsv;
            else if (strcmp(optarg, "bowtie") == 0)
                in_file_type = bowtie;
            else if (strcmp(optarg, "sam") == 0)
                in_file_type = sam;
            else if (strcmp(optarg, "fasta") == 0)
                in_file_type = fasta;
            else if (strcmp(optarg, "fastq") == 0)
                in_file_type = fastq;
            else
                in_file_type = unknown;
	    break;
     	case 'y':
            if (strcmp(optarg, "sam") == 0)
                out_file_type = sam;
            else
                out_file_type = sql;
	    break;
     	case 'f':
	    in_file = optarg;
	    break;
	case 'o':
	    out_path = optarg;
	    break;
	case 'i':
	    sql_id = atoi(optarg);
	    break;
	case 'b':
	    batch_id = atoi(optarg);
	    break;
	case 'e':
	    barcode_err_freq = atof(optarg);
	    break;
	case 'p':
	    num_threads = atoi(optarg);
	    break;
        case 'v':
            version();
            break;
	case '?':
	    // getopt_long already printed an error message.
	    help();
	    break;
     
	default:
	    cerr << "Unknown command line option '" << (char) c << "'\n";
	    help();
	    abort();
	}
    }

    if (in_file.length() == 0 || in_file_type == unknown) {
	cerr << "You must specify an input file of a supported type.\n";
	help();
    }

    if (out_path.length() == 0) 
	out_path = ".";

    if (out_path.at(out_path.length() - 1) != '/') 
	out_path += "/";

    if (model_type == fixed && barcode_err_freq == 0) {
	cerr << "You must specify the barcode error frequency.\n";
	help();
    }

    return 0;
}

void version() {
    std::cerr << "pstacks " << VERSION << "\n\n";

    exit(0);
}

void help() {
    std::cerr << "pstacks " << VERSION << "\n"
              << "pstacks -t file_type -f file_path [-o path] [-i id] [-b batch_id] [-e errfreq] [-p num_threads] [-h]" << "\n"
              << "  p: enable parallel execution with num_threads threads.\n"
	      << "  t: input file Type. Supported types: fasta, fastq, bowtie, sam, tsv.\n"
              << "  f: input file path.\n"
	      << "  y: output file type.\n"
	      << "  o: output path to write results.\n"
	      << "  i: SQL ID to insert into the output to identify this sample.\n"
	      << "  b: SQL Batch ID to insert into the output to identify a group of samples.\n"
	      << "  e: specify the barcode error frequency (0 < e < 1) if using the 'fixed' model.\n"
              << "  n: reads are NOT uniquely mapped.\n" 
	      << "  h: display this help messsage." << "\n\n";

    exit(0);
}