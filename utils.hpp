#pragma once
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>

// utility class for helper functions
class utils {
public:
    // random number generator
    static int random_int(int min, int max) {
        // use thread_local to avoid data race on static generator
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(min, max);
        return dist(gen);
    }

    // sleep for random seconds
    static void random_sleep(int min_ms, int max_ms) {
        int ms = random_int(min_ms, max_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    // string to lower case
    static std::string to_lower(const std::string& str) {
        std::string s = str;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    // trim whitespace from both ends
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    // simple fuzzy string matching score (0.0 to 1.0)
    // using levenshtein distance concept simplified
    static double fuzzy_match(const std::string& s1, const std::string& s2) {
        const std::size_t m = s1.size();
        const std::size_t n = s2.size();
        if (m == 0) return n == 0 ? 1.0 : 0.0;
        if (n == 0) return 0.0;

        std::vector<std::vector<std::size_t>> costs(m + 1, std::vector<std::size_t>(n + 1));
        for (std::size_t i = 0; i <= m; ++i) costs[i][0] = i;
        for (std::size_t j = 0; j <= n; ++j) costs[0][j] = j;

        for (std::size_t i = 1; i <= m; ++i) {
            for (std::size_t j = 1; j <= n; ++j) {
                std::size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                costs[i][j] = (std::min)({ costs[i - 1][j] + 1, costs[i][j - 1] + 1, costs[i - 1][j - 1] + cost });
            }
        }
        std::size_t max_len = (std::max)(m, n);
        return 1.0 - (double)costs[m][n] / max_len;
    }

    // load file lines into vector
    static std::vector<std::string> load_lines(const std::string& filepath) {
        std::vector<std::string> lines;
        std::ifstream file(filepath);
        std::string line;
        while (std::getline(file, line)) {
            // trim whitespace
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            if (!line.empty() && line[0] != '#') lines.push_back(line);
        }
        return lines;
    }
};
