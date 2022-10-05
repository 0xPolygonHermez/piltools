#include <sys/time.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <errno.h>
#include <list>
#include <map>
#include <set>
#include "engine.hpp"
#include "expression.hpp"
#include "reference.hpp"

namespace pil {

void *Engine::mapFile(const std::string &filename)
{
    struct stat sb;

    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("Error mapping file " + filename);
    }

    fstat(fd, &sb);
    std::cout << "SIZE:" << sb.st_size;
    void *maddr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(maddr != MAP_FAILED);
    close(fd);
    return maddr;
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

Engine::Engine(const std::string &pilJsonFilename, const std::string &constFilename, const std::string &commitFilename)
    :constRefs(ReferenceType::constP), cmRefs(ReferenceType::cmP), imRefs(ReferenceType::imP), expressions(*this)
{
    n = 0;
    nCommitments = 0;
    nConstants = 0;
    std::ifstream pilFile(pilJsonFilename);
    nlohmann::json pil = nlohmann::json::parse(pilFile);
    loadReferences(pil);
    std::cout << "mapping constant file " << constFilename << std::endl;
    constPols = (FrElement *)mapFile(constFilename);
    constRefs.map(constPols);
    std::cout << "mapping commited file " << commitFilename << std::endl;
    std::cout << "sizeof(FrElement): " << sizeof(FrElement) << "\n";
    cmPols = (FrElement *)mapFile(commitFilename);
    cmRefs.map(cmPols);
    loadPublics(pil);
    checkConnectionIdentities(pil);
    std::cout << "done" << std::endl;
}

ReferenceType Engine::getReferenceType(const std::string &name, const std::string &type)
{
    if (type == "cmP") return ReferenceType::cmP;
    if (type == "constP") return ReferenceType::constP;
    if (type == "imP") return ReferenceType::imP;
    throw std::runtime_error("Reference " + name + " has invalid type '" + type + "'");
}

void Engine::loadReferences(nlohmann::json &pil)
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

void Engine::loadPublics(nlohmann::json &pil)
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

void Engine::loadAndCompileExpressions(nlohmann::json &pil)
{
    expressions.loadAndCompile(pil["expressions"]);
}

void Engine::foundAllExpressions (nlohmann::json &pil)
{
    std::set<uid_t> eids;
    std::cout << "SET " << &eids << std::endl;

    std::cout << "connectionIdentities ....." << std::endl;
    auto connectionIdentities = pil["connectionIdentities"];
    for (auto it = connectionIdentities.begin(); it != connectionIdentities.end(); ++it) {
        for (auto ipols = (*it)["pols"].begin(); ipols != (*it)["pols"].end(); ++ipols) {
            uid_t id = *ipols;
            std::cout << "adding pols expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto iconnections = (*it)["connections"].begin(); iconnections != (*it)["connections"].end(); ++iconnections) {
            uid_t id = *iconnections;
            std::cout << "adding connections expression: " << id << std::endl;
            eids.insert(id);
        }
    }

    std::cout << "plookupIdentities ....." << std::endl;
    auto plookupIdentities = pil["plookupIdentities"];
    for (auto it = plookupIdentities.begin(); it != plookupIdentities.end(); ++it) {
        for (auto t = (*it)["t"].begin(); t != (*it)["t"].end(); ++t) {
            uid_t id = *t;
            std::cout << "adding t expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto selT = (*it)["selT"].begin(); selT != (*it)["selT"].end(); ++selT) {
            uid_t id = *selT;
            std::cout << "adding selT expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto f = (*it)["f"].begin(); f != (*it)["f"].end(); ++f) {
            uid_t id = *f;
            std::cout << "adding f expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto selF = (*it)["selF"].begin(); selF != (*it)["selF"].end(); ++selF) {
            uid_t id = *selF;
            std::cout << "adding selF expression: " << id << std::endl;
            eids.insert(id);
        }
    }

    std::cout << "permutationIdentities ....." << std::endl;
    auto permutationIdentities = pil["permutationIdentities"];
    for (auto it = permutationIdentities.begin(); it != permutationIdentities.end(); ++it) {
        for (auto t = (*it)["t"].begin(); t != (*it)["t"].end(); ++t) {
            uid_t id = *t;
            std::cout << "adding t expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto selT = (*it)["selT"].begin(); selT != (*it)["selT"].end(); ++selT) {
            uid_t id = *selT;
            std::cout << "adding selT expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto f = (*it)["f"].begin(); f != (*it)["f"].end(); ++f) {
            uid_t id = *f;
            std::cout << "adding f expression: " << id << std::endl;
            eids.insert(id);
        }
        for (auto selF = (*it)["selF"].begin(); selF != (*it)["selF"].end(); ++selF) {
            uid_t id = *selF;
            std::cout << "adding selF expression: " << id << std::endl;
            eids.insert(id);
        }
    }

    std::cout << "polIdentities ....." << std::endl;
    auto polIdentities = pil["polIdentities"];
    for (auto it = polIdentities.begin(); it != polIdentities.end(); ++it) {
        uid_t id = (*it)["e"];
        std::cout << "adding identity expression: " << id << std::endl;
        eids.insert(id);
    }

    /*
    auto pilExpressions = pil["expressions"];
    Dependencies dependencies;
    const dim_t exprsCount = pilExpressions.size();
    Expression *exprs = new Expression[exprsCount];

    std::cout << "pilExpressions.size():" << pilExpressions.size() << std::endl;

    uid_t index = 0;
    dim_t aliasCount = 0;
    dim_t aliasNextCount = 0;
    for (auto it = pilExpressions.begin(); it != pilExpressions.end(); ++it) {
        std::cout << "========== compiling " << index << "[" << eids.count(index) << "] ==========" << std::endl;
        exprs[index].compile(*it);
        // exprs[id].dump();
        if (exprs[index].alias) ++aliasCount;
        if (exprs[index].next) ++aliasNextCount;
        ++index;
    }

    for (auto index = 0; index < exprsCount; ++index) {
        exprs[index].getDependencies(dependencies);
    }
    */
    loadAndCompileExpressions(pil);
    expressions.dumpExpression(351);
    expressions.dumpExpression(383);
    expressions.dumpExpression(415);
    // reduceNumberAliasExpressions();

    expressions.reduceAliasExpressions();
    #ifdef __DEBUG__
    expressions.dumpDependencies();
    #endif
    expressions.calculateGroup();
    expressions.evalAll();

    // uint64_t *exprs = (uint64_t *)mapFile("../../data/v0.4.0.0-rc.1-basic/zkevm.expr");
    // uint64_t *exprs = (uint64_t *)mapFile("/home/ubuntu/zkevm-proverjs/build/v0.4.0.0-rc.1-basic/zkevm.expr");
    uint64_t *exprs = (uint64_t *)mapFile("/home/ubuntu/zkevm-proverjs/build/v0.3.0.0-rc.1/zkevm.expr");

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
    /*
    std::cout << "EXPR67" << pil["expressions"][67] << std::endl;
    expressions.dumpExpression(67);
    expressions.debugEval(*this, 67, 0);
    expressions.dumpExpression(67);
    std::cout << "EXPR84" << pil["expressions"][84] << std::endl;
    expressions.dumpExpression(84);

    */
    std::cout << "expressions:" << nExpressions << " referenced:" << eids.size() /* << " alias:" << aliasCount
              << " aliasNextCount:" << aliasNextCount*/ << " dependencies:" << expressions.dependencies.size() << "\n";

}

void Engine::checkConnectionIdentities (nlohmann::json &pil)
{
    foundAllExpressions(pil);
//    return;

/*
    for (nlohmann::json::iterator it = pilExpressions.begin(); it != pilExpressions.end(); ++it) {
        if (it->contains("id")) {
            std::cout << "#### " << id << " ID: " << (*it)["id"] << std::endl;
        } else if (it->contains("idQ")) {
            std::cout << "#### " << id << " IQD: " << (*it)["idQ"] << std::endl;
        } else {
            std::cout << "#### " << id << " NOTHING: " << *it << std::endl;
        }
        ++id;
    }

    auto pilConnectionIdentities = pil["connectionIdentities"];
    for (nlohmann::json::iterator it = pilConnectionIdentities.begin(); it != pilConnectionIdentities.end(); ++it) {
        auto pols = (*it)["pols"];
        for (nlohmann::json::iterator pit = pols.begin(); pit != pols.end(); ++pit) {
            std::cout << "pols " << (*pit) << std::endl;
            uid_t exprId = *pit;
            calculateExpression(exprId);
        }
        auto connections = (*it)["connections"];
        for (nlohmann::json::iterator cit = connections.begin(); cit != connections.end(); ++cit) {
            std::cout << "connections " << (*cit) << std::endl;
        }
    } */
    auto polIdentities = pil["polIdentities"];
    for (auto it = polIdentities.begin(); it != polIdentities.end(); ++it) {
        const uid_t eid = (*it)["e"];
        std::cout << "identity " << eid << " " << expressions.isZero(eid) << std::endl;
/*        for (nlohmann::json::iterator pit = pols.begin(); pit != pols.end(); ++pit) {
            std::cout << "pols " << (*pit) << std::endl;
            uid_t exprId = *pit;
            calculateExpression(exprId);
        }
        auto connections = (*it)["connections"];
        for (nlohmann::json::iterator cit = connections.begin(); cit != connections.end(); ++cit) {
            std::cout << "connections " << (*cit) << std::endl;
        }*/
    }
}

const FrElement Engine::calculateExpression(uid_t id)
{
    std::cout << "calculateExpression: " << id << std::endl;
    return Goldilocks::zero();
}

Engine::~Engine (void)
{
}

}