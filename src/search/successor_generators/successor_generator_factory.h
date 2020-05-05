#ifndef SEARCH_SUCCESSOR_GENERATOR_FACTORY_H
#define SEARCH_SUCCESSOR_GENERATOR_FACTORY_H

#include <string>

class Task;
class SuccessorGenerator;

class SuccessorGeneratorFactory {
public:
    static SuccessorGenerator *create(const std::string &method,
                                      unsigned seed,
                                      Task &task);
};


#endif //SEARCH_SUCCESSOR_GENERATOR_FACTORY_H
