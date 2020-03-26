#include "state.h"

using namespace std;

const vector<int> State::getObjects() {
  /*
   * Return a set of all objects occurring in any tuple of any relation in the
   * state
   */
  vector<int> obj;
  for (auto &relation : relations) {
    for (auto &tuple : relation.tuples) {
      for (int object : tuple) {
        obj.push_back(object);
      }
    }
  }
  return obj;
}

void State::addTuple(int relation, const GroundAtom &args) {
  relations[relation].tuples.insert(args);
}
