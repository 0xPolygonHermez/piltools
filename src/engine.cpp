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
#include "connection_map.hpp"
#include "block.hpp"
#include "cyclic.hpp"

#ifdef PIL_VERIFY
#include "interactive.hpp"
#endif

namespace pil {

Engine::Engine(EngineOptions options)
    :constRefs(ReferenceType::constP), cmRefs(ReferenceType::cmP), imRefs(ReferenceType::imP), expressions(*this), options(options)
{
    Block::init();

    n = 0;
    nCommitments = 0;
    nConstants = 0;
    publicsLoaded = false;

    checkOptions();
    loadJsonPil();
    loadReferences();
    loadConstantsFile();
    loadCommitedFile();
    loadPublicsFile();
    loadPublics();

    loadAndCompileExpressions();
    if (options.calculateExpressions) {
        calculateAllExpressions();
        imRefs.setExternalEvaluator([this](uid_t id, omega_t w, index_t index = 0) {return this->expressions.getEvaluation(id, w);});
    }

    if (options.checkIdentities) {
        checkPolIdentities();
    }
    if (options.checkPlookups) {
        checkPlookupIdentities();
    }
    if (options.checkPermutations) {
        checkPermutationIdentities();
    }
    if (options.checkConnections) {
        checkConnectionIdentities();
    }
    std::cout << "done in " << Block::getTotalTime() << " ms" << std::endl;

    #ifdef PIL_VERIFY
    if (options.interactive) {
        Interactive interactive(*this);
        interactive.execute();
    }
    #endif
}

void *Engine::mapFile(const std::string &title, const std::string &filename, dim_t size, bool wr )
{
    int fd;

    if (wr) {
        fd = open(filename.c_str(), O_RDWR|O_CREAT, 0666);
        if (ftruncate(fd, size) < 0) {
            throw std::runtime_error("Error on trucate of mapping(wr) "+filename);
        }
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

    std::cout << "mapping " << title << " file " << filename << " with " << Tools::humanSize(size) << std::endl;
    void *maddr = mmap(NULL, size, wr ? (PROT_WRITE|PROT_READ):PROT_READ, wr ? MAP_SHARED:MAP_PRIVATE, fd, 0);
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
        msync(it->first, it->second, MS_SYNC);
        munmap(it->first, it->second);
    }
    mappings.clear();
}

const Reference *Engine::getDirectReference(const std::string &name)
{
    auto pos = name.find('[');
    if (pos != std::string::npos) {
        auto epos = name.find(']', pos);
        if (epos != std::string::npos && epos > (pos + 1)) {
            uint index = strtoull(name.substr(pos+1, epos-pos-1).c_str(), NULL, 10);
            auto reference = getReference(name.substr(0, pos));
            return reference->getIndex(index);
        }
    }
    return getReference(name);
}

const Reference *Engine::getReference(const std::string &name, index_t index )
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
    return pRef;
}

FrElement Engine::getEvaluation(const std::string &name, omega_t w, index_t index )
{
    const Reference *pRef = getReference(name, index);
    return pRef->getEvaluation(w, index);
}


void Engine::calculateAllExpressions (void)
{
    Block block("Calculate expressions");
    expressions.calculateGroup();
    if (options.saveExpressions && !options.overwrite && access(options.expressionsFilename.c_str(), F_OK) == 0) {
        std::cerr << "ERROR: expressions file " << options.expressionsVerifyFilename << " exists, use -o to overwrite" << std::endl;
        exit(1);
    }
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

void Engine::loadPublicsFile (void)
{
    std::cout << "loadPublicsFile(" << options.publicsFilename << ")" << std::endl;
    if (!options.publicsFilename.empty()) {
        std::ifstream jsonFile(options.publicsFilename);
        publicsJson = nlohmann::json::parse(jsonFile);
        publicsLoaded = true;
        std::cout << "Loaded publics from " << options.publicsFilename << std::endl;
    }
}

void Engine::loadConstantsFile (void)
{
    constPols = (FrElement *)mapFile("constant", options.constFilename);
    constRefs.map(constPols);
}

void Engine::loadCommitedFile (void)
{
    cmPols = (FrElement *)mapFile("commited", options.commitFilename);
    cmRefs.map(cmPols);
}

void Engine::mapExpressionsFile (bool wr)
{
    expressions.setEvaluations((FrElement *)mapFile("expression", options.expressionsFilename, expressions.getEvaluationsSize(), wr));
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
    Block block("Loading references");
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
            auto pos = name.find_first_of('.');
            if (pos != std::string::npos) {
                std::string ns = name.substr(0, pos);
                if (std::find(namespaces.begin(), namespaces.end(), ns) == namespaces.end()) {
                    namespaces.push_back(ns);
                }
            }
        }
    }

    std::cout << "References: " << pilReferences.size() << "  (max id:" << max << ")" << std::endl;
    std::cout << "nConstants: " << nConstants << "  nCommitments: "  << nCommitments << "  TOTAL: " << (nConstants+nCommitments) << std::endl;
    std::cout << "Expressions: " << pil["expressions"].size() << std::endl;
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

        FrElement value;
        auto type = getReferenceType(name, polType);
        switch(type) {
            case ReferenceType::cmP:
                value = cmRefs.getEvaluation(polId, idx);
                break;
            case ReferenceType::imP:
                if (publicsLoaded && !publicsJson[id].is_null()) {
                    value = Goldilocks::fromU64(publicsJson[id]);
                } else {
                    value = imRefs.getEvaluation(polId, idx);
                }
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
}

void Engine::checkConnectionIdentities (void)
{
    Block block("Connections");

    if (pil["connectionIdentities"].size() == 0) {
        std::cout << "No connections defined" << std::endl;
        return;
    }

    uint nk = getMaxConnectionColumns();
    std::cout << "Columns(max): " << nk << std::endl;
    ConnectionMap cm(n, nk);

    auto connectionIdentities = pil["connectionIdentities"];
    for (auto it = connectionIdentities.begin(); it != connectionIdentities.end(); ++it) {
        const std::string where = (*it)["fileName"].get<std::string>(); + ":" + std::to_string((*it)["line"].get<uint64_t>());
        std::cout << "connection " << where << std::endl;
        assert((*it)["pols"].size() == (*it)["connections"].size());
        dim_t polsCount = (*it)["pols"].size();
        uid_t pols[polsCount];
        uid_t connections[polsCount];

        for (uid_t index = 0; index < polsCount; ++index) {
            pols[index] = (*it)["pols"][index];
            connections[index] = (*it)["connections"][index];
        }
        uint chunks = 64;
        omega_t wchunk = n / chunks;
        #pragma omp parallel for
        for (uint chunk = 0; chunk < chunks; ++chunk) {
            omega_t w1 = chunk * wchunk;
            omega_t w2 = (chunk == (chunks - 1)) ? n: w1 + wchunk;
            for (omega_t w = w1; w < w2; ++w) {
                for (uid_t j = 0; j < polsCount; ++j) {
                    auto v1 = expressions.getEvaluation(pols[j], w);
                    auto a = Goldilocks::toU64(expressions.getEvaluation(connections[j], w));
                    uint64_t _ij = cm.get(a);
                    if (_ij == ConnectionMap::NONE) {

                    }
                    uint64_t _i = cm.ij2i(_ij);
                    uint64_t _j = cm.ij2j(_ij);
                    auto v2 = expressions.getEvaluation(pols[_j], _i);
                    if (!Goldilocks::equal(v1, v2)) {
                        const std::string p1name = expressions.getName(pols[j]);
                        const std::string p2name = expressions.getName(pols[_j]);
                        std::cout << where << " connection does not match " << (p1name.empty() ? "p1":p1name) << "(id:" << j << ")[w=" << w << "] "
                                  << (p2name.empty() ? "p2":p2name) << "(id:" << _j << ")[w=" << _i << "] "
                                  << " v1:" << Goldilocks::toString(v1) << " v2:" << Goldilocks::toString(v2) << std::endl;
                    }
                }
            }
        }
    }
}


uint Engine::getMaxConnectionColumns (void)
{
    uint maxPolsCount = 0;
    auto connectionIdentities = pil["connectionIdentities"];
    for (auto it = connectionIdentities.begin(); it != connectionIdentities.end(); ++it) {
        const uint polsCount = (*it)["pols"].size();
        if (polsCount > maxPolsCount) maxPolsCount = polsCount;
    }
    return maxPolsCount;
}

int Engine::onErrorNotFoundPlookupValue(dim_t index, const std::string &value, omega_t w)
{
    const std::string location = (std::string)(pil["plookupIdentities"][index]["fileName"]) + ":" + std::to_string((uint)(pil["plookupIdentities"][index]["line"]));
    std::stringstream ss;
    ss << "Plookup #" << index << " " << location << " w:" << w << " not found " << expressions.valuesBinToString(value) << std::endl;
    #pragma omp critical
    {
        std::cerr << ss.str() << std::endl;
    }
    return 0;
}

void Engine::checkPlookupIdentities (void)
{
    Block block("Plookups");

    auto identities = pil["plookupIdentities"];
    const dim_t identitiesCount = identities.size();

    if (identitiesCount == 0) {
        std::cout << "No plookups defined" << std::endl;
        return;
    }

    auto tt = new std::unordered_set<std::string>[identitiesCount];
    auto tCount = new dim_t[identitiesCount]();
    auto fCount = new dim_t[identitiesCount]();
    auto cyclic = new Cyclic[identitiesCount];

    auto startT = Tools::startCrono();

    prepareT(identities, "Plookup", [tt, tCount, cyclic](dim_t index, const std::string &value, omega_t w) {   cyclic[index].cycle(w);
                                                                                                               ++tCount[index];
                                                                                                               tt[index].insert(value);
                                                                                                               return 0; });
    verifyF(identities, "Plookup", [tt, fCount,this](dim_t index, const std::string &value, omega_t w) {
                                                                        ++fCount[index];
                                                                        int result = tt[index].count(value);
                                                                        if (result == 0) result = onErrorNotFoundPlookupValue(index, value, w);
                                                                        return result; });
    Tools::endCronoAndShowIt(startT);

    std::cout << "#   |SOURCE                                  |SELF(left)          |#F        |SELT(right)         |#T        |#T UNIQS  |NOTES" << std::endl;
    std::cout << "----+----------------------------------------+--------------------+----------+--------------------+----------+----------+----------" << std::endl;
    for (dim_t index = 0; index < identitiesCount; ++index) {
        const bool hasSelT = identities[index]["selT"].is_null() == false;
        const std::string selector = hasSelT ? expressions.getName(identities[index]["selT"]):"(none)";
        std::stringstream notes;

        if (cyclic[index].isCyclic() && hasSelT) {
            notes << (cyclic[index].isCyclic(n) ? "full-":"") << "cyclic +" << cyclic[index].offset() << "/" << cyclic[index].period() << "  ";
        }
        printf("%-4ld|%-40s|%20s|%10ld|%20s|%10ld|%10ld|%s\n", index, (((std::string)identities[index]["fileName"])+ ":" +std::to_string((uint)(identities[index]["line"]))).c_str(),
            "", (uint64_t)fCount[index], selector.c_str(), (uint64_t)tCount[index], (uint64_t)tt[index].size(), notes.str().c_str());
    }

    startT = Tools::startCrono();
    std::cout << "start to delete" << std::endl;
    #pragma omp parallel for
    for (dim_t index = 0; index < identitiesCount + 3; ++index) {
        if (index < identitiesCount) tt[index].clear();
        if (index == (identitiesCount + 0)) delete [] tCount;
        if (index == (identitiesCount + 1)) delete [] fCount;
        if (index == (identitiesCount + 2)) delete [] cyclic;
    }
    delete [] tt;
    Tools::endCronoAndShowIt(startT);
}

int Engine::onErrorPermutationValue(dim_t index, const std::string &value, omega_t w, PermutationError e)
{
    const std::string location = (std::string)(pil["permutationIdentities"][index]["fileName"]) + ":" + std::to_string((uint)(pil["permutationIdentities"][index]["line"]));
    std::stringstream ss;
    ss << "Permutation #" << index << " " << location;
    switch (e) {
        case PermutationError::notFound: ss << " w:" << w << " not found "; break;
        case PermutationError::notEnought: ss << " w:" << w << " not enougth value "; break;
        case PermutationError::remainingValues: ss << " remaning " << w << " values "; break;
    }
    ss << expressions.valuesBinToString(value) << std::endl;
    #pragma omp critical
    {
        std::cerr << ss.str() << std::endl;
    }
    return 0;
}

void Engine::checkPermutationIdentities (void)
{
    Block block("Permutations checks");
    auto identities = pil["permutationIdentities"];
    const dim_t identitiesCount = identities.size();

    if (identitiesCount == 0) {
        std::cout << "No permutation checks defined" << std::endl;
        return;
    }

    auto tt = new std::unordered_map<std::string, int64_t>[identitiesCount];

    prepareT(identities, "Permutation", [tt](dim_t index, const std::string &value, omega_t w) {    auto it = tt[index].find(value);
                                                                                                    if (it == tt[index].end()) {
                                                                                                        tt[index][value] = 1;
                                                                                                    } else {
                                                                                                        ++it->second;
                                                                                                    }
                                                                                                    return 0; });
    verifyF(identities, "Permutation", [tt, this](dim_t index, const std::string &value, omega_t w) {
                                                                                     auto it = tt[index].find(value);
                                                                                     if (it == tt[index].end()) {
                                                                                        return onErrorPermutationValue(index, value, w, PermutationError::notFound);
                                                                                     }
                                                                                     if (it->second > 0) {
                                                                                        --it->second;
                                                                                        return 1;
                                                                                     }
                                                                                     return onErrorPermutationValue(index, value, w, PermutationError::notEnought);});
    for (dim_t index = 0; index < identitiesCount; ++index) {
        for (auto it = tt[index].begin(); it != tt[index].end(); ++it) {
            if (it->second > 0 && onErrorPermutationValue(index, it->first, it->second, PermutationError::remainingValues) == 0) {
                break;
            }
        }
    }

/*
    for (dim_t index = 0; index < identitiesCount; ++index) {
        const std::string selector = identities[index]["selT"].is_null() ? "(none)":expressions.getName(identities[index]["selT"]);
        printf("%-40s|%20s|%10ld|%20s|%10ld|%10ld\n", (((std::string)identities[index]["fileName"])+ ":" +std::to_string((uint)(identities[index]["line"]))).c_str(),
            selector.c_str(), (uint64_t)tCount[index], "", (uint64_t)fCount[index], (uint64_t)tt[index].size());
    }*/

    #pragma omp parallel for
    for (dim_t index = 0; index < identitiesCount; ++index) {
        if (index < identitiesCount) tt[index].clear();
    }
    delete [] tt;
}

template<typename SetFunc>
void Engine::prepareT (nlohmann::json& identities, const std::string &label, SetFunc set)
{
    uint64_t done = 0;
    const dim_t identitiesCount = identities.size();

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
            // if (identityIndex == 30) std::cout << "T;30;" << w << ";" << expressions.valuesToString(ts, tCount, w) << std::endl;
            set(identityIndex, expressions.valuesToBinString(ts, tCount, w), w);
        }
        updatePercentT("preparing "+label+" selT/T ", done, identitiesCount);
    }
}

template<typename CheckFunc>
void Engine::verifyF (nlohmann::json& identities, const std::string &label, CheckFunc check)
{
    const dim_t identitiesCount = identities.size();
    uint64_t lastdone = 0;
    uint64_t done = 0;

    const uint64_t doneStep = n / 20;

    #pragma omp parallel for
    for (dim_t identityIndex = 0; identityIndex < identitiesCount; ++identityIndex) {
        const std::string plabel = label + "[" + std::to_string(identityIndex+1) + "/" + std::to_string(identitiesCount) + "] ";
        auto identity = identities[identityIndex];

        bool hasSelF = identity["selF"].is_null() == false;
        uid_t selF = hasSelF ? (uid_t)(identity["selF"]) : 0;
        dim_t fCount = identity["f"].size();
        uid_t fs[fCount];

        for (uid_t index = 0; index < fCount; ++index) {
            fs[index] = identity["f"][index];
        }
        uint chunks = 16; // 8192;
        uint wn = n / chunks;

        for (uint64_t ichunk = 0; ichunk < chunks; ++ichunk) {
            omega_t w1 = wn * ichunk;
            omega_t w2 = (ichunk == (chunks - 1)) ? n: w1 + wn;
            for (omega_t w = w1; w < w2; ++w) {
                if (hasSelF && Goldilocks::isZero(expressions.getEvaluation(selF, w))) continue;
                #pragma omp critical
                {
                    if (identityIndex == 30) std::cout << "F;30;" << w << ";" << expressions.valuesToString(fs, fCount, w) << std::endl;
                }
                if (check(identityIndex, expressions.valuesToBinString(fs, fCount, w), w) == 0 ) {
                    ichunk = chunks;
                    w2 = n;
                    break;
                }
            }
            updatePercentF(label, done, lastdone, w2-w1, doneStep, identityIndex, identitiesCount);
        }
    }
    std::cout << std::endl << std::flush;
}

inline void Engine::updatePercentT ( const std::string &title, uint64_t &done, uint64_t total )
{
    #pragma omp critical
    {
        done += 1;
        std::cout << title << pil::Tools::percentBar(done, total, false)
                    << done << "/" << total;
        if (done == total) {
            std::cout << std::endl << std::endl << std::flush;
        } else {
            std::cout << "\t\r" << std::flush;
        }
    }
}

inline void Engine::updatePercentF ( const std::string &title, uint64_t &done, uint64_t &lastdone, uint64_t delta, uint64_t doneStep, dim_t index, dim_t count )
{
    #pragma omp critical
    {
        done += delta;
        if ((done - lastdone) > doneStep || (done % n) == 0 || !lastdone) {
            std::cout << title << pil::Tools::percentBar(done, n * count);

            if (done == (n * count)) {
                std::cout << std::endl << std::flush;
            } else {
                std::cout << "\t\r" << std::flush;
            }
            lastdone = done;
        }
    }
}


void Engine::checkPolIdentities (void)
{
    Block block("Identities");
    auto polIdentities = pil["polIdentities"];
    dim_t polIdentitiesCount = polIdentities.size();
    dim_t errorCount = 0;

    if (polIdentitiesCount == 0) {
        std::cout << "No identities defined" << std::endl;
        return;
    }

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
    uint64_t *exprs = (uint64_t *)mapFile("verify", options.expressionsVerifyFilename);

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
/*
template<typename T>
void Engine::listReferences (T &names)
{
    constRefs.list(names);
    cmRefs.list(names);
    // imRefs.list(names);
}
*/

void Engine::listReferences (std::list<std::string> &names, bool expandArrays)
{
    for (auto it = referencesByName.begin(); it != referencesByName.end(); ++it) {
        const uint len = it->second->len;
        if (expandArrays && len > 1) {
            uint maxtmp = it->first.size() + 23;
            char tmp[maxtmp];
            for (uint index = 0; index < len; ++index) {
                snprintf(tmp, maxtmp, "%s[%lu]", it->first.c_str(), index);
                names.push_back(tmp);
            }
        } else {
            names.push_back(it->first);
        }
    }
    // imRefs.list(names);
}

}
