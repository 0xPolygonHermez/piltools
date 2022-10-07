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
    }
    // expressions.dumpExpression(351);
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
    auto plookupIdentities = pil["plookupIdentities"];
    for (auto it = plookupIdentities.begin(); it != plookupIdentities.end(); ++it) {
        std::cout << *it << std::endl;
        uid_t selT = (*it)["selT"];
        uid_t selF = (*it)["selF"];
        dim_t tCount = (*it)["t"].size();
        dim_t fCount = (*it)["f"].size();
        uid_t ts[tCount];
        uid_t fs[fCount];

        for (uid_t index = 0; index < tCount; ++index) {
            ts[index] = (*it)["t"][index];
        }

        std::unordered_multiset<std::string> tt;

        for (omega_t w = 0; w < n; ++w) {
            if (Goldilocks::isZero(expressions.getEvaluation(selT, w))) continue;
            tt.insert(expressions.valuesToString(ts, tCount, w));
        }
    }
}

void Engine::checkPermutationIdentities (void)
{
    std::cout << "permutationIdentities ....." << std::endl;
    auto permutationIdentities = pil["permutationIdentities"];
    for (auto it = permutationIdentities.begin(); it != permutationIdentities.end(); ++it) {
        for (auto t = (*it)["t"].begin(); t != (*it)["t"].end(); ++t) {
            uid_t id = *t;
            std::cout << "adding t expression: " << id << std::endl;
        }
        for (auto selT = (*it)["selT"].begin(); selT != (*it)["selT"].end(); ++selT) {
            uid_t id = *selT;
            std::cout << "adding selT expression: " << id << std::endl;
        }
        for (auto f = (*it)["f"].begin(); f != (*it)["f"].end(); ++f) {
            uid_t id = *f;
            std::cout << "adding f expression: " << id << std::endl;
        }
        for (auto selF = (*it)["selF"].begin(); selF != (*it)["selF"].end(); ++selF) {
            uid_t id = *selF;
            std::cout << "adding selF expression: " << id << std::endl;
        }
    }
}

void Engine::checkPolIdentities (void)
{
    std::cout << "polIdentities ....." << std::endl;
    auto polIdentities = pil["polIdentities"];
    for (auto it = polIdentities.begin(); it != polIdentities.end(); ++it) {
        const uid_t eid = (*it)["e"];
        std::cout << "identity " << eid << " " << expressions.isZero(eid) << std::endl;
    }
}

void Engine::verifyExpressionsWithFile (const std::string &filename)
{
    /*
       ../../data/v0.4.0.0-rc.1-basic/zkevm.expr
       /home/ubuntu/zkevm-proverjs/build/v0.4.0.0-rc.1-basic/zkevm.expr
       /home/ubuntu/zkevm-proverjs/build/v0.3.0.0-rc.1/zkevm.expr
    */
    uint64_t *exprs = (uint64_t *)mapFile(filename);

    uint differences = 0;
    for (uint w = 0; w < n && differences < 5000; ++w) {
        if (w && (w % 1000 == 0)) std::cout << "w:" << w << std::endl;
        for (uint index = 0; index < expressions.count && differences < 5000; ++index) {
            const uint64_t evaluation = Goldilocks::toU64(expressions.getEvaluation(index, w, Expressions::GROUP_NONE));
            const uint64_t _evaluation = exprs[index + expressions.count * w];
            if (evaluation != _evaluation) {
                ++differences;
                std::cout << "w:" << w << " [" << index << "/" << w << "=" << (index + expressions.count * w) << "] "  << _evaluation << " " << evaluation << std::endl;
            }
        }
    }

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