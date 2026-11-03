#include "ground_novelty.h"

#include "../states/ground_state.h"

#include <iostream>

using namespace std;

namespace {
// Beyond this memory for pair (triple) tables, new partitions get tables of
// smaller width.
const size_t MAX_PAIR_TABLE_BYTES = size_t(512) * 1024 * 1024;
const size_t MAX_TRIPLE_TABLE_BYTES = size_t(1024) * 1024 * 1024;


bool test_and_set(vector<uint64_t> &bits, size_t i)
{
    uint64_t mask = 1ULL << (i & 63);
    uint64_t &word = bits[i >> 6];
    if (word & mask)
        return false;
    word |= mask;
    return true;
}

void sort3(int &a, int &b, int &c)
{
    if (a > b)
        swap(a, b);
    if (b > c)
        swap(b, c);
    if (a > b)
        swap(a, b);
}
}  // namespace

GroundNovelty::GroundNovelty(int width, int num_facts)
    : width(width), num_facts(num_facts), num_words(ground_state::get_num_words(num_facts))
{
    if (this->width > 2 && num_facts > MAX_FACTS_FOR_WIDTH_3) {
        cout << "Too many facts for width-3 novelty; using width 2." << endl;
        this->width = 2;
    }
    if (this->width > 1 && num_facts > MAX_FACTS_FOR_WIDTH_2) {
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
        size_t n = num_facts;
        size_t pair_words = (n * n + 63) / 64;
        size_t pair_bytes = pair_words * sizeof(uint64_t);
        if (width > 1 && pair_table_bytes + pair_bytes <= MAX_PAIR_TABLE_BYTES) {
            table.pairs.assign(pair_words, 0);
            pair_table_bytes += pair_bytes;
            size_t triple_words = (n * n * n + 63) / 64;
            size_t triple_bytes = triple_words * sizeof(uint64_t);
            if (width > 2 && triple_table_bytes + triple_bytes <= MAX_TRIPLE_TABLE_BYTES) {
                table.triples.assign(triple_words, 0);
                triple_table_bytes += triple_bytes;
            }
        }
        it = tables.emplace(partition, std::move(table)).first;
    }
    return it->second;
}

int GroundNovelty::get_novelty(const Table &table,
                               bool novel_fact,
                               bool novel_pair,
                               bool novel_triple) const
{
    if (novel_fact)
        return 1;
    if (novel_pair && !table.pairs.empty())
        return 2;
    if (novel_triple && !table.triples.empty())
        return 3;
    return NOT_NOVEL;
}

int GroundNovelty::compute(const uint64_t *bits, const vector<int> &facts, long long partition)
{
    ++num_checks;
    if (width > 1)
        num_tuples_checked += (long long)facts.size() * facts.size() / 2;
    if (width > 2)
        num_tuples_checked += (long long)facts.size() * facts.size() * facts.size() / 6;
    Table &table = get_table(partition);
    bool novel_fact = false;
    for (int w = 0; w < num_words; ++w) {
        if (bits[w] & ~table.singles[w]) {
            novel_fact = true;
            table.singles[w] |= bits[w];
        }
    }
    bool novel_pair = false;
    bool novel_triple = false;
    if (!table.pairs.empty()) {
        size_t n = num_facts;
        for (size_t i = 0; i < facts.size(); ++i) {
            size_t row = size_t(facts[i]) * n;
            for (size_t j = i + 1; j < facts.size(); ++j) {
                if (test_and_set(table.pairs, row + facts[j]))
                    novel_pair = true;
            }
        }
        if (!table.triples.empty()) {
            for (size_t i = 0; i < facts.size(); ++i) {
                for (size_t j = i + 1; j < facts.size(); ++j) {
                    size_t row = (size_t(facts[i]) * n + facts[j]) * n;
                    for (size_t k = j + 1; k < facts.size(); ++k) {
                        if (test_and_set(table.triples, row + facts[k]))
                            novel_triple = true;
                    }
                }
            }
        }
    }
    return get_novelty(table, novel_fact, novel_pair, novel_triple);
}

int GroundNovelty::compute_for_added_facts(const vector<int> &added,
                                           const vector<int> &facts,
                                           long long partition)
{
    ++num_checks;
    if (width > 1)
        num_tuples_checked += (long long)added.size() * facts.size();
    if (width > 2)
        num_tuples_checked += (long long)added.size() * facts.size() * facts.size() / 2;
    Table &table = get_table(partition);
    bool novel_fact = false;
    for (int f : added) {
        if (test_and_set(table.singles, f))
            novel_fact = true;
    }
    bool novel_pair = false;
    bool novel_triple = false;
    if (!table.pairs.empty()) {
        size_t n = num_facts;
        for (int a : added) {
            for (int f : facts) {
                if (f == a)
                    continue;
                size_t i = f < a ? size_t(f) * n + a : size_t(a) * n + f;
                if (test_and_set(table.pairs, i))
                    novel_pair = true;
            }
        }
        if (!table.triples.empty()) {
            for (int a : added) {
                for (size_t i = 0; i < facts.size(); ++i) {
                    if (facts[i] == a)
                        continue;
                    for (size_t j = i + 1; j < facts.size(); ++j) {
                        if (facts[j] == a)
                            continue;
                        int x = a, y = facts[i], z = facts[j];
                        sort3(x, y, z);
                        if (test_and_set(table.triples, (size_t(x) * n + y) * n + z))
                            novel_triple = true;
                    }
                }
            }
        }
    }
    return get_novelty(table, novel_fact, novel_pair, novel_triple);
}

bool GroundNovelty::is_too_expensive() const
{
    return width > 1 && num_checks > 0 && num_tuples_checked / num_checks > max_tuples_per_check;
}

void GroundNovelty::downgrade()
{
    cout << "Width-" << width << " novelty too expensive (" << num_tuples_checked / num_checks
         << " tuples per check); using width " << width - 1 << "." << endl;
    --width;
    for (auto &entry : tables) {
        vector<uint64_t>().swap(entry.second.triples);
        if (width < 2)
            vector<uint64_t>().swap(entry.second.pairs);
    }
    triple_table_bytes = 0;
    if (width < 2)
        pair_table_bytes = 0;
    num_tuples_checked = 0;
    num_checks = 0;
}

void GroundNovelty::reset()
{
    tables.clear();
    pair_table_bytes = 0;
    triple_table_bytes = 0;
}
