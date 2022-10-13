#ifndef __PIL__ENGINE__HPP__
#define __PIL__ENGINE__HPP__

#include <stdint.h>
#include <nlohmann/json.hpp>
#include <goldilocks_base_field.hpp>
#include <unordered_set>
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

enum class EvaluationMapMode { BY_OMEGAS, BY_POLS };

class EngineOptions {
    public:
        std::string pilJsonFilename = "";
        std::string constFilename = "";
        std::string commitFilename = "";
        bool loadExpressions = false;
        bool saveExpressions = false;
        std::string expressionsFilename = "";
        std::string expressionsVerifyFilename = "";
        EvaluationMapMode expressionsVerifyFileMode = EvaluationMapMode::BY_OMEGAS;
};

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
        EngineOptions options;

        enum class PermutationError { notFound, notEnought, remainingValues };
        Engine(const EngineOptions options);
        ~Engine (void);

        FrElement getEvaluation (const std::string &name, omega_t w = 0, index_t index = 0);
        const FrElement getConst (uid_t id, omega_t w = 0) { return constPols[(uint64_t) w * nConstants + id]; };
        const FrElement getCommited (uid_t id, omega_t w = 0) { return cmPols[(uint64_t)w * nCommitments + id]; };
        const FrElement getPublic (uid_t id, omega_t w = 0) { return publics.getValue(id); };
        const FrElement getExpression (uid_t id, omega_t w, uid_t evalGroupId) { return expressions.getEvaluation(id, w, evalGroupId); };
        std::string getConstName (uid_t id) { return constRefs.getName(id); };
        std::string getCommitedName (uid_t id) { return cmRefs.getName(id); };
        std::string getPublicName (uid_t id) { return publics.getName(id); };
        omega_t next (omega_t w)  { return w % n; };

        void dump (uid_t id);
    protected:
        nlohmann::json pil;
        std::map<void *,size_t> mappings;

        void checkOptions (void);

        void loadConstantsFile (void);
        void loadCommitedFile (void);
        void loadJsonPil (void);
        void mapExpressionsFile (bool wr = false);

        void loadReferences (void);
        void loadPublics (void);
        void checkConnectionIdentities (void);
        void checkPolIdentities (void);
        void checkPlookupIdentities (void);
        void checkPermutationIdentities (void);
        const FrElement calculateExpression(uid_t id);
        ReferenceType getReferenceType(const std::string &name, const std::string &type);
        void precompileExpression(nlohmann::json& node);
        void loadAndCompileExpressions (void);
        void calculateAllExpressions (void);
        bool verifyExpressionsWithFile (void);

        void *mapFile(const std::string &filename, dim_t size = 0, bool wr = false);
        void unmap (void *);
        void unmapAll (void);
        bool checkFilename (const std::string &filename, bool toWrite = false, bool exceptionOnFail = false);
        template<typename SetFunc>
        void prepareT (nlohmann::json& identities, const std::string &label, SetFunc set);
        template<typename GetFunc>
        void verifyF (nlohmann::json& identities, const std::string &label, GetFunc get);
        inline void updatePercentT ( const std::string &title, uint64_t &done, uint64_t total );
        inline void updatePercentF ( const std::string &title, uint64_t &done, uint64_t &lastdone, uint64_t delta, uint64_t doneStep, dim_t index, dim_t count );
        void generateConnectionMap ( void );
        int onErrorNotFoundPlookupValue(dim_t index, const std::string &value, omega_t w);
        int onErrorPermutationValue(dim_t index, const std::string &value, omega_t w, PermutationError e);
        uint getMaxConnectionColumns (void);
};

}
#endif