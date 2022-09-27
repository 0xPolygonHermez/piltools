#ifndef __PIL__ENGINE__HPP__
#define __PIL__ENGINE__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>

namespace pil {
    class Engine;
}

#include "references.hpp"
#include "public.hpp"

namespace pil {

class Engine {
    public:
        uint64_t n = 0;
        uint64_t nCommitments = 0;
        uint64_t nConstants = 0;

        References cmRefs;
        References constRefs;
        References imRefs;
        PublicValues publics;

        std::map<std::string, const Reference *> referencesByName;
        uint64_t *constPols;
        uint64_t *cmPols;

        Engine(const std::string &pilJsonFilename, const std::string &constFilename, const std::string &commitFilename);
        ~Engine (void);
        uint64_t getPolValue(const std::string &name, uint64_t w = 0, uint64_t arrayIndex = 0);
    protected:
        void loadReferences(nlohmann::json &pil);
        void loadPublics(nlohmann::json &pil);
        void checkConnectionIdentities(nlohmann::json &pil);
        FrElement calculateExpression(uint64_t id);
        ReferenceType getReferenceType(const std::string &name, const std::string &type);
        void *mapFile(const std::string &filename);
        void precompileExpression(nlohmann::json& node);
};

}
#endif