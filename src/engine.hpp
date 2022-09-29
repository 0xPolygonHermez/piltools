#ifndef __PIL__ENGINE__HPP__
#define __PIL__ENGINE__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>
#include <goldilocks_base_field.hpp>
namespace pil {
    class Engine;
}

#include "references.hpp"
#include "public.hpp"
#include "expressions.hpp"
#include "types.hpp"
#include "operation.hpp"
#include "dependencies.hpp"

namespace pil {

class Engine {
    public:
        dim_t n;
        dim_t nCommitments;
        dim_t nConstants;
        dim_t nExpressions;

        References constRefs;
        References cmRefs;
        References imRefs;
        PublicValues publics;
        Expressions expressions;

        std::map<std::string, const Reference *> referencesByName;
        FrElement *constPols;
        FrElement *cmPols;

        Engine(const std::string &pilJsonFilename, const std::string &constFilename, const std::string &commitFilename);
        ~Engine (void);
        FrElement getEvaluation(const std::string &name, omega_t w = 0, index_t index = 0);
        const FrElement getConst(uid_t id, omega_t w = 0) { return constPols[w % nConstants]; };
        const FrElement getCommited(uid_t id, omega_t w = 0) { return cmPols[w % nConstants]; };
        const FrElement getPublic(uid_t id, omega_t w = 0) { return Goldilocks::fromU64((uint64_t)99); };
    protected:
        void loadReferences(nlohmann::json &pil);
        void loadPublics(nlohmann::json &pil);
        void checkConnectionIdentities(nlohmann::json &pil);
        const FrElement calculateExpression(uid_t id);
        ReferenceType getReferenceType(const std::string &name, const std::string &type);
        void *mapFile(const std::string &filename);
        void precompileExpression(nlohmann::json& node);
        void loadAndCompileExpressions(nlohmann::json& node);
        void foundAllExpressions(nlohmann::json& node);

};

}
#endif