#ifndef SEARCH_DATALOG_ANNOTATIONS_ANNOTATION_H_
#define SEARCH_DATALOG_ANNOTATIONS_ANNOTATION_H_

#include <functional>

#include "../ground_rule.h"

#include "../../task.h"

namespace datalog {

class Annotation {
public:
    virtual ~Annotation() = default;

    virtual void operator()(GroundRule gr) = 0;

    virtual bool operator==(const Annotation &other) = 0;

    bool operator!=(const Annotation &other) {return !(*this == other);}
};


using AnnotationGenerator = std::function<std::unique_ptr<Annotation>(int, const Task&)>;

}

#endif //SEARCH_DATALOG_ANNOTATIONS_ANNOTATION_H_
