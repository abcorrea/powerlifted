#include "ground_novelty.h"

#include "../states/ground_state.h"

#include <iostream>

using namespace std;

namespace {
// Beyond this memory for pair tables, new partitions get width-1 tables.
const size_t MAX_PAIR_TABLE_BYTES = size_t(512) * 1024 * 1024;

// Average number of pairs per check beyond which width 2 is too expensive.
const long long MAX_PAIRS_PER_CHECK = 100000;

bool test_and_set(vector<uint64_t> &bits, size_t i)
{
    uint64_t mask = 1ULL << (i & 63);
    uint64_t &word = bits[i >> 6];
    if (word & mask)
        return false;
    word |= mask;
    return true;
}
}  // namespace

GroundNovelty::GroundNovelty(int width, int num_facts)
    : width(width), num_facts(num_facts), num_words(ground_state::get_num_words(num_facts))
{
    if (width > 1 && num_facts > MAX_FACTS_FOR_WIDTH_2) {
        cout << "Too many facts for width-2 novelty; using width 1." << endl;
        this->width = 1;
    }
}

GroundNovelty::Table &GroundNovelty::get_table(long long partition)
{
    auto it = tables.find(partition);
    if (it == tables.end()) {
        Table table;
        table.singles.assign(num_words, 0);
        size_t pair_words = (size_t(num_facts) * num_facts + 63) / 64;
        size_t bytes = pair_words * sizeof(uint64_t);
        if (width > 1 && pair_table_bytes + bytes <= MAX_PAIR_TABLE_BYTES) {
            table.pairs.assign(pair_words, 0);
            pair_table_bytes += bytes;
        }
        it = tables.emplace(partition, std::move(table)).first;
    }
    return it->second;
}

int GroundNovelty::compute(const uint64_t *bits, const vector<int> &facts, long long partition)
{
    ++num_checks;
    if (width > 1)
        num_pairs_checked += (long long)facts.size() * facts.size() / 2;
    Table &table = get_table(partition);
    bool novel = false;
    for (int w = 0; w < num_words; ++w) {
        if (bits[w] & ~table.singles[w]) {
            novel = true;
            table.singles[w] |= bits[w];
        }
    }
    if (width == 1 || table.pairs.empty())
        return novel ? 1 : NOT_NOVEL;
    bool novel_pair = false;
    for (size_t i = 0; i < facts.size(); ++i) {
        size_t row = size_t(facts[i]) * num_facts;
        for (size_t j = i + 1; j < facts.size(); ++j) {
            if (test_and_set(table.pairs, row + facts[j]))
                novel_pair = true;
        }
    }
    return novel ? 1 : (novel_pair ? 2 : NOT_NOVEL);
}

int GroundNovelty::compute_for_added_facts(const vector<int> &added,
                                           const vector<int> &facts,
                                           long long partition)
{
    ++num_checks;
    if (width > 1)
        num_pairs_checked += (long long)added.size() * facts.size();
    Table &table = get_table(partition);
    bool novel = false;
    for (int f : added) {
        if (test_and_set(table.singles, f))
            novel = true;
    }
    if (width == 1 || table.pairs.empty())
        return novel ? 1 : NOT_NOVEL;
    bool novel_pair = false;
    for (int a : added) {
        for (int f : facts) {
            if (f == a)
                continue;
            size_t i = f < a ? size_t(f) * num_facts + a : size_t(a) * num_facts + f;
            if (test_and_set(table.pairs, i))
                novel_pair = true;
        }
    }
    return novel ? 1 : (novel_pair ? 2 : NOT_NOVEL);
}

bool GroundNovelty::is_too_expensive() const
{
    return width > 1 && num_checks > 0 && num_pairs_checked / num_checks > MAX_PAIRS_PER_CHECK;
}

void GroundNovelty::downgrade_to_width_1()
{
    cout << "Width-2 novelty too expensive (" << num_pairs_checked / num_checks
         << " pairs per check); using width 1." << endl;
    width = 1;
    for (auto &entry : tables)
        vector<uint64_t>().swap(entry.second.pairs);
    pair_table_bytes = 0;
}
