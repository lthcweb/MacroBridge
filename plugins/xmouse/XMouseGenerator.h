#pragma once

#include "AIR.h"
#include <string>
#include <vector>

class XMouseGenerator {
public:
    explicit XMouseGenerator(std::vector<AIR::AIRDiagnostic>& diags);
    std::string generate(const AIR::AIRNode& root);

private:
    void emitNode(const AIR::AIRNode& node, std::string& out);
    static std::string keyToToken(AIR::AIRKey key, const std::string& raw);

    std::vector<AIR::AIRDiagnostic>& m_diags;
};
