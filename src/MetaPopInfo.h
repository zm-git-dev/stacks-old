#ifndef METAPOPINFO_H
#define METAPOPINFO_H

#include <string>
#include <vector>
#include <map>

using std::size_t;
using std::pair;
using std::vector;
using std::string;
using std::map;

/*
 * MetaPopInfo
 * Class for reprensenting a metapopulation : its samples, populations,
 * groups of populations, and associated information.
 */
class MetaPopInfo {
public:
    struct Sample {
        string name;
        size_t pop;
        size_t id; // optional, deprecated

        Sample(const string& n) : name(n), pop(-1), id(-1) {}
        inline bool operator<(const Sample& other) const;
    };
    struct Pop {
        string name;
        size_t first_sample;
        size_t last_sample;
        size_t group;

        Pop(const string& n) : name(n), first_sample(-1), last_sample(-1), group(-1) {}
        static const string default_name;
    };
    struct Group {
        string name;
        vector<size_t> pops;

        Group(const string& n) : name(n), pops() {}
        static const string default_name;
    };

private:
    vector<Sample> samples_; //n.b. Samples are sorted primarily by population index, and secondarily by name.
    vector<Pop> pops_;
    vector<Group> groups_;

    map<string,size_t> sample_indexes_; // Links a name with an index in [samples_].
    map<string,size_t> pop_indexes_;
    map<string,size_t> group_indexes_;
    void reset_sample_map(); // Resets [sample_indexes_].
    void reset_pop_map();
    void reset_group_map();

    map<size_t,size_t> sample_indexes_by_id_; // Links a sample ID with an index in [samples_].
    void reset_sample_id_map();

public:
    // Create the representation :
    // -- from a population map file.
    // -- from just a vector of sample names.
    // -- or by looking for "*.tags.tsv(.gz)" files in a directory.
    bool init_popmap(const string& popmap_path);
    bool init_names(const vector<string>& sample_names);
    bool init_directory(const string& dir_path);

    // Delete samples from the metapopulation.
    // (As samples, populations or groups may be deleted, the indexes of
    // the remaining ones change, but the order in which they appear
    // is preserved.)
    void delete_samples(const vector<size_t>& rm_samples);

    // Retrieve information.
    const vector<Sample>& samples() const {return samples_;}
    const vector<Pop>& pops() const {return pops_;}
    const vector<Group>& groups() const {return groups_;}

    size_t get_sample_index(const string& name) const {return sample_indexes_.at(name);}
    size_t get_pop_index(const string& name) const {return pop_indexes_.at(name);}
    size_t get_group_index(const string& name) const {return group_indexes_.at(name);}

    /*
     * Methods for backwards compatibility
     */

    // Sample IDs. (IDs unicity is not enforced.)
    void set_sample_id(size_t index, size_t id) {samples_.at(index).id = id; sample_indexes_by_id_[id] = index;}
    size_t get_sample_index(const size_t& id) const {return sample_indexes_by_id_.at(id);}

    // Fill former globals.
    void fill_files(vector<pair<int, string> >&) const;
    void fill_sample_ids(vector<int>&) const;
    void fill_samples(map<int, string>&) const;
    void fill_pop_key(map<int, string>&) const;
    void fill_pop_indexes(map<int, pair<int, int> >&) const;
    void fill_grp_key(map<int, string>&) const;
    void fill_grp_members(map<int, vector<int> >&) const;
};

inline
bool MetaPopInfo::Sample::operator<(const Sample& other) const {
    if (pop == other.pop)
        return name < other.name;
    else
        return pop < other.pop;
}

#endif // METAPOPINFO_H