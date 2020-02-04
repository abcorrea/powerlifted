#ifndef SEARCH_PREDICATE_H
#define SEARCH_PREDICATE_H

#include <string>
#include <utility>
#include <vector>

class Predicate {
public:
  Predicate(std::string &&name, int index, int arity, bool static_predicate,
            std::vector<int> &&types)
      : name(std::move(name)), index(index), arity(arity),
        static_predicate(static_predicate), types(std::move(types)) {
    // Constructor
  }

  void addArgument(int index) { types.push_back(index); }

  const std::string &getName() const;

  int getArity() const;

  const std::vector<int> &getTypes() const;

  bool isStaticPredicate() const;

private:
  std::string name;
  int index;
  int arity;
  bool static_predicate;
  std::vector<int> types;
};

#endif // SEARCH_PREDICATE_H
