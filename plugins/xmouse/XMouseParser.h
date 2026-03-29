#pragma once

#include "AIR.h"
#include <string>
#include <vector>

class XMouseParser {
public:
    XMouseParser(const std::string& source, std::vector<AIR::AIRDiagnostic>& diags);
    AIR::AIRNodePtr parse();

private:
    AIR::AIRNodePtr parseButtonLine(const std::string& line);
    AIR::AIRNodePtr parseSimulatedKeys(const std::string& keysExpr);

    static std::string attr(const std::string& line, const std::string& key);
    static std::string trim(std::string s);
    static AIR::AIRKey tokenToKey(const std::string& token);
    static bool isNumber(const std::string& s);

    void warn(const std::string& msg);
    void error(const std::string& msg);

    std::string m_source;
    std::vector<AIR::AIRDiagnostic>& m_diags;
};
