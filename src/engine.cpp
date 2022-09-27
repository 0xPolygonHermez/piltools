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
#include "engine.hpp"
#include "expression.hpp"

namespace pil {

void *Engine::mapFile(const std::string &filename)
{
    struct stat sb;

    int fd = open(filename.c_str(), O_RDONLY|O_LARGEFILE);
    if (fd < 0) {
        throw std::runtime_error("Error mapping file " + filename);
    }

    fstat(fd, &sb);
    void *maddr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(maddr != MAP_FAILED);
    close(fd);
    return maddr;
}
uint64_t Engine::getPolValue(const std::string &name, uint64_t w, uint64_t arrayIndex )
{
    auto pos = referencesByName.find(name);
    if (pos == referencesByName.end()) {
        throw std::runtime_error("Reference "+name+" not found");
    }
    const Reference *pRef = pos->second;
    if (arrayIndex) {
        if (arrayIndex >= pRef->len) {
            throw std::runtime_error("Out of index on Reference "+name+"["+std::to_string(pRef->len)+"] with arrayIndex "+std::to_string(arrayIndex));
        }
        pRef += arrayIndex;
    }
    std::cout << pRef->name << " = " << FrToString(pRef->getEvaluation(w)) << std::endl;
    return 0;
}

Engine::Engine(const std::string &pilJsonFilename, const std::string &constFilename, const std::string &commitFilename)
{
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
        Reference ref;
        std::string name = it.key();
        auto value = it.value();
        ref.polDeg = value["polDeg"];
        if (!n) {
            n = ref.polDeg;
        }
        ref.id = value["id"];
        ref.isArray = value["isArray"];
        ref.len = 1;
        if (ref.isArray) {
            ref.len = value["len"];
        }

        if (ref.id > max) {
            max = (ref.id + ref.len - 1);
        }
        ref.type = getReferenceType(name, value["type"]);
        const Reference *pRef;
        switch(ref.type) {
            case ReferenceType::cmP:
                pRef = cmRefs.add(name, ref);
                break;
            case ReferenceType::constP:
                pRef = constRefs.add(name, ref);
                break;
            case ReferenceType::imP:
                pRef = imRefs.add(name, ref);
                break;
        }
        referencesByName[name] = pRef;
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
        int polId = (*it)["polId"];
        int idx = (*it)["idx"];
        int id = (*it)["id"];
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



void Engine::checkConnectionIdentities (nlohmann::json &pil)
{
    auto pilExpressions = pil["expressions"];
    uint64_t id = 0;
    std::cout << pilExpressions[1157] << std::endl;

    Expression e;
    e.compile(pilExpressions[1157]);
    e.dump();
    e.eval(*this);
    return;

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
            int exprId = *pit;
            calculateExpression(exprId);
        }
        auto connections = (*it)["connections"];
        for (nlohmann::json::iterator cit = connections.begin(); cit != connections.end(); ++cit) {
            std::cout << "connections " << (*cit) << std::endl;
        }
    }
}

FrElement Engine::calculateExpression(uint64_t id)
{
    std::cout << "calculateExpression: " << id << std::endl;

}

Engine::~Engine (void)
{

}


}