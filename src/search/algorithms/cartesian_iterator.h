
#pragma once

#include <cassert>
#include <numeric>
#include <ostream>
#include <vector>


namespace utils {

template <typename T> 
class cartesian_iterator {
protected:
	const std::vector<std::vector<T>> _values;
	
	std::vector<typename std::vector<T>::const_iterator> _iterators;
	
	std::vector<T> _element; // The current element

	bool _ended;

public:
	explicit cartesian_iterator(std::vector<std::vector<T>> values) :
        _values(std::move(values)),
        _iterators(),
        _element(),
        _ended(_values.empty())
    {
        // Initialize the iterator vector and check that all the sets of the cartesian product
        // have at least one element (otherwise the product will be empty)
        _iterators.reserve(_values.size());
        _element.reserve(_values.size());
        for (const auto& domain:_values) {
            if (domain.empty()) {
                _ended = true;
                break;
            }
            _iterators.push_back(domain.begin());
            _element.push_back(domain[0]);
        }
    }

	cartesian_iterator(const cartesian_iterator&) = default;
	
	//! Compute the size of the cartesian product without computing the product itself
	unsigned long size() const {
        return std::accumulate(_values.begin(), _values.end(), (unsigned long) 1,
                               [](int a, const std::vector<T>& b) { return a * b.size(); });
    }
	
	//! Advances the iterator at position 'idx' or, if it has reached the end, resets its and tries with the one at the left, recursively.
	void advanceIterator(unsigned idx) {
        assert(idx < _iterators.size());
        if (++_iterators[idx] != _values[idx].end()) {
            updateElement(idx);
        } else {
            if (idx == 0) { // Base case: We're done with all the elements in the cartesian product.
                _ended = true;
                return;
            }

            // otherwise: reset the current idx to zero and try incrementing the previous one.
            _iterators[idx] = _values[idx].begin();
            updateElement(idx);
            advanceIterator(idx-1);
        }
    }
	
	const std::vector<T>& operator*() const { return _element; }
	
	const cartesian_iterator& operator++() {
		advanceIterator(_iterators.size() - 1);
		return *this;
	}

	const cartesian_iterator operator++(int) {
        cartesian_iterator tmp(*this);
        operator++();
        return tmp;
    }
	
	bool ended() const { return _ended; }

protected:
    void updateElement(unsigned idx) {
        assert(_iterators[idx] != _values[idx].end());
        _element[idx] = *(_iterators[idx]);
    }
};


} // namespaces
