#include <sys/time.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include "engine.hpp"
#include "expression.hpp"
#include "reference.hpp"
#include "tools.hpp"

namespace pil {

Engine::Engine(EngineOptions options)
    :constRefs(ReferenceType::constP), cmRefs(ReferenceType::cmP), imRefs(ReferenceType::imP), expressions(*this), options(options)
{
    n = 0;
    nCommitments = 0;
    nConstants = 0;

    checkOptions();
    loadJsonPil();
    loadReferences();
    loadConstantsFile();
    loadCommitedFile();
    loadPublics();

    loadAndCompileExpressions();
    calculateAllExpressions();

    checkPolIdentities();
    checkPlookupIdentities();
    // checkConnectionIdentities();
    std::cout << "done" << std::endl;
}

void *Engine::mapFile(const std::string &filename, dim_t size, bool wr )
{
    int fd;

    if (wr) {
        fd = open(filename.c_str(), O_RDWR|O_CREAT, 0666);
        ftruncate(fd, size);
    } else {
        fd = open(filename.c_str(), O_RDONLY);
    }

    if (fd < 0) {
        throw std::runtime_error("Error mapping file " + filename);
    }

    if (!size) {
        struct stat sb;
        fstat(fd, &sb);
        size = sb.st_size;
    }

    std::cout << "mapping file " << filename << " with " << size << " bytes" << std::endl;
    void *maddr = mmap(NULL, size, wr ? PROT_WRITE:PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    assert(maddr != MAP_FAILED);

    mappings[maddr] = size;
    return maddr;
}

void Engine::unmap (void *addr)
{
    auto it = mappings.find(addr);
    if (it != mappings.end()) {
        munmap(addr, it->second);
        mappings.erase(addr);
    }
}

void Engine::unmapAll (void)
{
    for (auto it = mappings.begin(); it != mappings.end(); ++it) {
        munmap(it->first, it->second);
    }
    mappings.clear();
}

FrElement Engine::getEvaluation(const std::string &name, omega_t w, index_t index )
{
    auto pos = referencesByName.find(name);
    if (pos == referencesByName.end()) {
        throw std::runtime_error("Reference "+name+" not found");
    }
    const Reference *pRef = pos->second;
    if (index) {
        if (index >= pRef->len) {
            throw std::runtime_error("Out of index on Reference "+name+"["+std::to_string(pRef->len)+"] with arrayIndex "+std::to_string(index));
        }
    }
    FrElement evaluation = pRef->getEvaluation(w, index);
    // std::cout << pRef->name << " = " << FrToString(evaluation) << std::endl;
    return evaluation;
}


void Engine::calculateAllExpressions (void)
{
    expressions.calculateGroup();
    if (options.loadExpressions || options.saveExpressions) {
        mapExpressionsFile(options.saveExpressions);
    }

    if (options.loadExpressions) {
        expressions.afterEvaluationsLoaded();
    } else {
        expressions.evalAll();
        if (!options.expressionsVerifyFilename.empty()) {
            if (!verifyExpressionsWithFile()) {
                std::cerr << "verification with file " << options.expressionsVerifyFilename << " fails !!" << std::endl;
                exit(1);
            }
        }
        expressions.afterEvaluationsLoaded();
    }
}

void Engine::loadJsonPil (void)
{
    std::ifstream pilFile(options.pilJsonFilename);
    pil = nlohmann::json::parse(pilFile);
}

void Engine::loadConstantsFile (void)
{
    std::cout << "mapping constant file " << options.constFilename << std::endl;
    constPols = (FrElement *)mapFile(options.constFilename);
    constRefs.map(constPols);
}

void Engine::loadCommitedFile (void)
{
    std::cout << "mapping commited file " << options.commitFilename << std::endl;
    cmPols = (FrElement *)mapFile(options.commitFilename);
    cmRefs.map(cmPols);
}

void Engine::mapExpressionsFile (bool wr)
{
    std::cout << "mapping constant file " << options.expressionsFilename << std::endl;
    expressions.setEvaluations((FrElement *)mapFile(options.expressionsFilename, expressions.getEvaluationsSize(), wr));
}

ReferenceType Engine::getReferenceType (const std::string &name, const std::string &type)
{
    if (type == "cmP") return ReferenceType::cmP;
    if (type == "constP") return ReferenceType::constP;
    if (type == "imP") return ReferenceType::imP;
    throw std::runtime_error("Reference " + name + " has invalid type '" + type + "'");
}

void Engine::loadReferences (void)
{
    for (nlohmann::json::iterator it = pil.begin(); it != pil.end(); ++it) {
        std::cout << it.key() << "\n";
    }
    nConstants = pil["nConstants"];
    nCommitments = pil["nCommitments"];
    auto pilReferences = pil["references"];
    uint64_t max = 0;
    for (nlohmann::json::iterator it = pilReferences.begin(); it != pilReferences.end(); ++it) {
        std::string name = it.key();
        auto value = it.value();
        const dim_t polDeg = value["polDeg"];
        if (!n) {
            n = polDeg;
            expressions.n = n;
        }
        const uid_t id = value["id"];
        bool isArray = value["isArray"];
        index_t len = 1;
        if (isArray) {
            len = value["len"];
        }

        if (id > max) {
            max = (id + len - 1);
        }
        const ReferenceType type = getReferenceType(name, value["type"]);
        const Reference *ref;
        switch(type) {
            case ReferenceType::cmP:
                ref = cmRefs.add(name, id, len);
                break;
            case ReferenceType::constP:
                ref = constRefs.add(name, id, len);
                break;
            case ReferenceType::imP:
                ref = imRefs.add(name, id, len);
                break;
            default:
                ref = NULL;
                break;
        }
        const bool found = referencesByName.find(name) != referencesByName.end();
        if (found) {
            std::cout << "DUPLICATED " << name << std::endl;
            throw std::runtime_error("Duplicated "+name);
        }
        if (ref) {
            referencesByName[name] = ref;
        }
    }

    std::cout << "size:" << pilReferences.size() << "\n";
    std::cout << "max:" << max << "\n";
    std::cout << "nConstants:" << nConstants << " nCommitments:"  << nCommitments << " total:" << (nConstants+nCommitments) << "\n";
    std::cout << "expressions:" << pil["expressions"].size() << std::endl;
}

void Engine::loadPublics (void)
{
    auto pilPublics = pil["publics"];
    for (nlohmann::json::iterator it = pilPublics.begin(); it != pilPublics.end(); ++it) {
        std::string name = (*it)["name"];
        uid_t polId = (*it)["polId"];
        index_t idx = (*it)["idx"];
        uid_t id = (*it)["id"];
        std::string polType = (*it)["polType"];
        std::cout << "name:" << *it << " value:" << std::endl;
        auto type = getReferenceType(name, polType);
        FrElement value;
        switch(type) {
            case ReferenceType::cmP:
                value = cmRefs.getEvaluation(polId, idx);
                break;
            case ReferenceType::imP:
                // TODO: calculateValues
                // value = imRefs.getPolValue(polId, idx);
                break;
            default:
                throw std::runtime_error("Invalid type "+polType+" for public "+name);
        }
        publics.add(id, name, value);
    }
}

void Engine::loadAndCompileExpressions (void)
{
    expressions.loadAndCompile(pil["expressions"]);
    expressions.reduceAliasExpressions();
    #ifdef __DEBUG__
    expressions.dumpDependencies();
    #endif
    // reduceNumberAliasExpressions();
}

void Engine::checkConnectionIdentities (void)
{
    std::cout << "connectionIdentities ....." << std::endl;
    auto connectionIdentities = pil["connectionIdentities"];
    for (auto it = connectionIdentities.begin(); it != connectionIdentities.end(); ++it) {
        for (auto ipols = (*it)["pols"].begin(); ipols != (*it)["pols"].end(); ++ipols) {
            uid_t id = *ipols;
        }
        for (auto iconnections = (*it)["connections"].begin(); iconnections != (*it)["connections"].end(); ++iconnections) {
            uid_t id = *iconnections;
        }
    }
}


void Engine::checkPlookupIdentities (void)
{
    std::cout << "plookupIdentities ....." << std::endl;
    auto identities = pil["plookupIdentities"];
    dim_t identitiesCount = identities.size();
    std::unordered_set<std::string> tt[identitiesCount];
    const uint64_t doneStep = n / 20;

    uint64_t done = 0;
    uint64_t lastdone = 0;
    auto startT = Tools::startCrono();
    #pragma omp parallel for
    for (dim_t identityIndex = 0; identityIndex < identitiesCount; ++identityIndex) {
        auto identity = identities[identityIndex];

        bool hasSelT = identity["selT"].is_null() == false;
        uid_t selT = hasSelT ? (uid_t)(identity["selT"]) : 0;
        dim_t tCount = identity["t"].size();
        uid_t ts[tCount];

        for (uid_t index = 0; index < tCount; ++index) {
            ts[index] = identity["t"][index];
        }

        for (omega_t w = 0; w < n; ++w) {
            if (hasSelT && Goldilocks::isZero(expressions.getEvaluation(selT, w))) continue;
            tt[identityIndex].insert(expressions.valuesToBinString(ts, tCount, w));
        }
        #pragma omp critical
        {
            done += 1;
            std::cout << "preparing plookup selT/T " << pil::Tools::percentBar(done, identitiesCount, false)
                      << done << "/" << identitiesCount;
            if (done == identitiesCount) {
                std::cout << std::endl << std::flush;
            } else {
                std::cout << "\t\r" << std::flush;
            }
        }
    }
    std::cout << std::endl << std::flush;

    const uint64_t totaln = n * identitiesCount;
    for (dim_t identityIndex = 0; identityIndex < identitiesCount; ++identityIndex) {
        const std::string label = "plookup[" + std::to_string(identityIndex+1) + "/" + std::to_string(identitiesCount) + "] ";
        const std::string location = (std::string)(identities[identityIndex]["fileName"]) + ":" + std::to_string((uint)(identities[identityIndex]["line"]));
        auto identity = identities[identityIndex];
        uint64_t done = 0;
        uint64_t lastdone = 0;

        bool hasSelF = identity["selF"].is_null() == false;
        uid_t selF = hasSelF ? (uid_t)(identity["selF"]) : 0;
        dim_t fCount = identity["f"].size();
        uid_t fs[fCount];


        for (uid_t index = 0; index < fCount; ++index) {
            fs[index] = identity["f"][index];
        }
        uint chunks = 8192;
        uint wn = n / chunks;

        #pragma omp parallel for
        for (uint64_t ichunk = 0; ichunk < chunks; ++ichunk) {
            omega_t w1 = wn * ichunk;
            omega_t w2 = (ichunk == (chunks - 1)) ? n: w1 + wn;
            for (omega_t w = w1; w < w2; ++w) {
                if (hasSelF && Goldilocks::isZero(expressions.getEvaluation(selF, w))) continue;
                if (tt[identityIndex].count(expressions.valuesToBinString(fs, fCount, w)) == 0) {
                    std::cerr << "problem on plookup #" << identityIndex << " " << location << " w:" << w << " not found " << expressions.valuesToString(fs, fCount, w) << std::endl;
                }
            }
            #pragma omp critical
            {
                done += (w2 - w1);
                if ((done - lastdone) > doneStep || done == n || !lastdone) {
                    std::cout << label << pil::Tools::percentBar(done + identityIndex * n, totaln);

                    if (done == n && identityIndex == (identitiesCount - 1)) {
                        std::cout << std::endl << std::flush;
                    } else {
                        std::cout << "\t\r" << std::flush;
                    }
                    lastdone = done;
                }
            }
        }
    }
    std::cout << std::endl << std::flush;
    Tools::endCronoAndShowIt(startT);
}

void Engine::checkPermutationIdentities (void)
{
    std::cout << "permutationIdentities ....." << std::endl;
    auto permutationIdentities = pil["permutationIdentities"];
    dim_t permutationIdentitiesCount = permutationIdentities.size();
    std::unordered_map<std::string, int64_t> tt[permutationIdentitiesCount];

    auto startT = Tools::startCrono();
    for (dim_t index = 0; index < permutationIdentitiesCount; ++index) {
        std::cout << "plookup " << (index+1) << "/" << permutationIdentitiesCount << std::endl;
        auto permutation = permutationIdentities[index];

        bool hasSelT = permutation["selT"].is_null() == false;
        bool hasSelF = permutation["selF"].is_null() == false;
        uid_t selT = hasSelT ? (uid_t)(permutation["selT"]) : 0;
        uid_t selF = hasSelF ? (uid_t)(permutation["selF"]) : 0;
        dim_t tCount = permutation["t"].size();
        dim_t fCount = permutation["f"].size();
        uid_t ts[tCount];
        uid_t fs[fCount];

        for (uid_t index = 0; index < tCount; ++index) {
            ts[index] = permutation["t"][index];
        }

        for (omega_t w = 0; w < n; ++w) {
            if ((w % 100000) == 0) std::cout << "polsT " << w << std::endl;
            if (hasSelT && Goldilocks::isZero(expressions.getEvaluation(selT, w))) continue;
            const std::string key = expressions.valuesToBinString(ts, tCount, w);
            auto it = tt[index].find(key);
            if (it == tt[index].end()) {
                tt[index][key] = 1;
            } else {
                ++it->second;
            }
        }

        for (uid_t index = 0; index < fCount; ++index) {
            fs[index] = permutation["f"][index];
        }
        uint chunks = 8192;
        uint wn = n / chunks;
        uint done = 0;
        #pragma omp parallel for
        for (uint64_t ichunk = 0; ichunk < chunks; ++ichunk) {
            omega_t w1 = wn * ichunk;
            omega_t w2 = (ichunk == (chunks - 1)) ? n: w1 + wn;
            for (omega_t w = w1; w < w2; ++w) {
                if (hasSelF && Goldilocks::isZero(expressions.getEvaluation(selF, w))) continue;
                const std::string key = expressions.valuesToBinString(fs, fCount, w);
                auto it = tt[index].find(key);
                if (it == tt[index].end()) {
                    std::cerr << "problem on w:" << w << " " << std::endl;
                } else {
                    if (it->second <= 0) {
                        std::cerr << "problem on w:" << w << " " << std::endl;
                    }
                    --it->second;
                }
            }
            #pragma omp critical
            {
                done += (w2 - w1);
                std::cout << done << "/" << n << std::endl;
            }
        }
    }
    Tools::endCronoAndShowIt(startT);
}

void Engine::checkPolIdentities (void)
{
    auto polIdentities = pil["polIdentities"];
    dim_t polIdentitiesCount = polIdentities.size();
    dim_t errorCount = 0;

    std::cout << "verify " << polIdentitiesCount << " polIdentities ....." << std::endl;

    for (dim_t index = 0; index < polIdentitiesCount; ++index) {
        uid_t exprid = polIdentities[index]["e"];
        if (expressions.isZero(exprid)) continue;
        ++errorCount;
        omega_t w = expressions.getFirstNonZeroEvaluation(exprid);
        std::cerr << "Fail polynomial identity #" << index << " expr:" << exprid << " " << polIdentities[index]["fileName"] << ":"
                  << polIdentities[index]["line"] << " on w:" << w << " value:" << Goldilocks::toString(expressions.getEvaluation(exprid,w)) << std::endl;
    }

    std::cout << "> polIdentities " << polIdentitiesCount << " verified, ";
    if (!errorCount) {
        std::cout << "no errors found ... [\x1B[1;32mOK\x1B[0m]" << std::endl;
    } else {
        std::cout << "found " << errorCount << " expressions with errors ... [\x1B[1;31mFAIL\x1B[0m]" << std::endl;
    }
}

bool Engine::verifyExpressionsWithFile (void)
{
    uint64_t *exprs = (uint64_t *)mapFile(options.expressionsVerifyFilename);

    uint differences = 0;
    const uint maxDifferences = 2000;
    for (uint w = 0; w < n && differences < maxDifferences; ++w) {
        if (w && (w % 1000 == 0)) std::cout << "w:" << w << std::endl;
        for (uint index = 0; index < expressions.count && differences < maxDifferences; ++index) {
            const uint64_t evaluation = Goldilocks::toU64(expressions.getEvaluation(index, w, Expressions::GROUP_NONE));
            const uint64_t offset = (options.expressionsVerifyFileMode == EvaluationMapMode::BY_OMEGAS) ? (index + expressions.count * w) : (index * n +  w);
            const uint64_t _evaluation = exprs[offset ];
            if (evaluation != _evaluation) {
                ++differences;
                std::cout << "w:" << w << " [" << index << "/" << w << "=" << (index + expressions.count * w) << "] "  << _evaluation << " " << evaluation
                          << (expressions.isAlias(index)? "ALIAS":"") << std::endl;
            }
        }
    }
    return differences == 0;
}

Engine::~Engine (void)
{
    unmapAll();
}

void Engine::checkOptions (void)
{
    uint errorCount = 0;

    if (options.pilJsonFilename.empty()) {
        ++errorCount;
        std::cerr << "required pilJsonFilename not specified" << std::endl;
    } else {
        if (!checkFilename(options.pilJsonFilename)) ++errorCount;
    }

    if (options.constFilename.empty()) {
        ++errorCount;
        std::cerr << "required constFilename not specified" << std::endl;
    } else {
        if (!checkFilename(options.constFilename)) ++errorCount;
    }

    if (options.commitFilename.empty()) {
        ++errorCount;
        std::cerr << "required commitFilename not specified" << std::endl;
    } else {
        if (!checkFilename(options.commitFilename)) ++errorCount;
    }

    if (options.loadExpressions && options.saveExpressions) {
        ++errorCount;
        std::cerr << "incompatible options loadExpressions and saveExpressions" << std::endl;
    }

    if (options.loadExpressions || options.saveExpressions) {
        if (options.expressionsFilename.empty()) {
            ++errorCount;
            std::cerr << "expressionsFilename must be specified when loadExpressions or saveExpressions was true" << std::endl;
        } else if (options.loadExpressions) {
            if (!checkFilename(options.expressionsFilename)) ++errorCount;
        }
    }

    if (errorCount > 0) {
        std::cerr << "Found " << errorCount << " errors on options" << std::endl;
        exit(1);
    }
}

bool Engine::checkFilename (const std::string &filename, bool toWrite, bool exceptionOnFail)
{
    bool result = true;
    if (access(filename.c_str(), toWrite ? (R_OK|W_OK) : (F_OK|R_OK)) != 0) {
        int _errno = errno;
        result = false;
        std::string msg = "File "+filename+(toWrite ? " couldn't be used to write" : " not exists or not accesible");
        std::string edesc = strerror(_errno);
        msg += ". E:" + std::to_string(_errno)+" "+edesc;
        if (exceptionOnFail) throw std::invalid_argument(msg);
        std::cerr << msg << std::endl;
    }
    return result;
}

}