#ifndef __PIL__REFERENCES__HPP__
#define __PIL__REFERENCES__HPP__

#include <nlohmann/json.hpp>
#include <map>
#include <list>
#include <functional>
#include "fr_element.hpp"
#include "types.hpp"

namespace pil {
    class References;
}

#include "reference.hpp"

namespace pil {

class References {
    public:
        const Reference *add(const std::string &name, uid_t id, dim_t len);
        const Reference *get(uid_t id);
        void map(void *data);
        FrElement getEvaluation(uid_t id, omega_t w, index_t index = 0);
        void setExternalEvaluator(std::function<FrElement(uid_t, omega_t, index_t)> evaluator) { externalEvaluator = evaluator; };
        std::string getName(uid_t id) const;
        References (ReferenceType type);
        ~References (void);
    protected:
        ReferenceType type;
        std::function<FrElement(uid_t, omega_t, index_t)> externalEvaluator;
        std::map<uid_t, Reference *> values;
        FrElement *evaluations;
};

}

#endif