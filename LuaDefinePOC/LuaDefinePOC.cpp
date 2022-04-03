#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <map>

struct Define {
    std::string functionName;
    std::vector<std::string> params;
    std::string macroContent;
};

struct MacroDefinition {
    int length;
    std::vector<std::string> params;
};

static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

void Exit(bool isError = false, LPCWSTR error = L"") {
    MessageBox(0, error, L"Error", 0);
    exit(0);
}

Define ParseDefine(std::string define) {
    Define def = Define();

    // Start parsing function name
    std::string defName;
    int curPos = strlen("#DEFINE ");
    while (true) {
        if (curPos > define.size()) {
            std::cout << "Incorrect #define definition" << std::endl;
            return def;
        }

        char curChar = define.at(curPos);
        if (curChar == '(') {
            curPos++;
            break;
        } else {
            defName += curChar;
            curPos++;
        }
    }
    

    // Start parsing function arguments
    std::string curArg;
    std::vector<std::string> curArgs;
    while (true) {
        if (curPos > define.size()) {
            std::cout << "Incorrect #define definition" << std::endl;
            return def;
        }

        char curChar = define.at(curPos);
        if (curChar == ')') {
            if (!curArg.empty()) {
                ltrim(curArg);
                curArgs.push_back(curArg);
                curArg = "";
            }
            curPos++;
            break;
        } else if (curChar == ',') {
            ltrim(curArg);
            curArgs.push_back(curArg);
            curArg = "";
            curPos++;
        } else {
            curArg += curChar;
            curPos++;
        }
    }

    // Finish parsing the define content
    std::string macroContent;
    while (curPos < define.size()) {
        macroContent += define.at(curPos);
        curPos++;
    }
    ltrim(macroContent);

    def.functionName = defName;
    def.params = curArgs;
    def.macroContent = macroContent;

    return def;
}

MacroDefinition ParseMacro(std::string str, int pos = 0) {
    MacroDefinition def = MacroDefinition();

    std::string parseStr = str.substr(pos);

    int curPos = 0;
    bool startedDef = false;
    std::string curArg;
    std::vector<std::string> curArgs;
    while (true) {
        if (curPos > parseStr.size()) {
            std::cout << "Incorrect #define definition" << std::endl;
            return def;
        }

        char curChar = parseStr.at(curPos);
        if (!startedDef)
            if (curChar == '(') {
                startedDef = true;
                curPos++;
            } else
                curPos++;
        else {
            if (curChar == ')') {
                if (!curArg.empty()) {
                    ltrim(curArg);
                    curArgs.push_back(curArg);
                    curArg = "";
                }
                curPos++;
                break;
            }
            else if (curChar == ',') {
                ltrim(curArg);
                curArgs.push_back(curArg);
                curArg = "";
                curPos++;
            }
            else {
                curArg += curChar;
                curPos++;
            }
        }
    }

    def.length = curPos;
    def.params = curArgs;

    return def;
}

std::string ProcessDefines(std::string path) {
    std::ifstream in(path);

    if (!in.is_open()) Exit(true, L"File was not able to be opened!");

    std::string out;
    std::vector<Define> defines;

    bool nextIsMacro = false;
    std::string macroContent;
    for (std::string line; std::getline(in, line); ) {
        if (!strcmp(line.substr(0, strlen("#DEFINE")).c_str(), "#DEFINE") || nextIsMacro) {
            if (!nextIsMacro) {
                if (!strcmp(line.substr(line.size() - 1, line.size()).c_str(), "\\")) {
                    nextIsMacro = true;
                    macroContent = line;
                } else {
                    defines.push_back(ParseDefine(line));
                }
            } else {
                std::cout << line << std::endl;
                ltrim(line);
                macroContent = macroContent.substr(0, macroContent.size() - 1) + line;
                nextIsMacro = !strcmp(line.substr(line.size() - 1, line.size()).c_str(), "\\");
                if (!nextIsMacro) defines.push_back(ParseDefine(macroContent));
            }
        } else {
            out += line + "\n";
        }
    }

    for (size_t i = 0; i < defines.size(); i++) {
        int lastFound = std::string::npos;
        do {
            auto name = defines[i].functionName;
            size_t curFind = out.find(name, lastFound == std::string::npos ? 0 : lastFound + 1);
            if (curFind == std::string::npos) break;
            
            Define define = defines[i];
            MacroDefinition macro = ParseMacro(out, curFind);

            if (define.params.size() != macro.params.size()) {
                std::cout << "Invalid macro usage of " << define.functionName << std::endl;
                break;
            }

            std::string macroText = define.macroContent;
            std::map<std::string, std::string> argValues;
            for (size_t i = 0; i < define.params.size(); i++) {
                argValues[define.params[i]] = macro.params[i];

                std::regex regex(std::string("\\b(" + std::string(define.params[i]) + ")\\b"),
                    std::regex_constants::ECMAScript | std::regex_constants::icase);

                macroText = std::regex_replace(macroText, regex, macro.params[i]);
            }

            out.replace((int)curFind, (int)macro.length, macroText);
        } while (true);
    }
    
    return out;
}

int main() {
    std::string inputFile;
    std::cout << "Input LUA file:" << std::endl;
    std::cin >> inputFile;

    std::string input = ProcessDefines(inputFile);
    ltrim(input);

    std::ofstream outputFile(inputFile + "d");
    outputFile << input;
    outputFile.close();

    std::cout << "Wrote to output file: " + (inputFile + "d") << std::endl;
}