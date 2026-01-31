#pragma once
#include <string>
#include <vector>
#include <regex>
#include <pugixml.hpp>
#include <iostream>

class parser {
public:
    // parse html using pugixml
    // note: pugixml is for xml, but handles some html. 
    // for real-world html, we often need regex or libxml2's html parser.
    static std::string get_text_by_xpath(const std::string& html_content, const std::string& xpath_query) {
        pugi::xml_document doc;
        // parse with loose options
        pugi::xml_parse_result result = doc.load_buffer(html_content.c_str(), html_content.size(), 
            pugi::parse_default | pugi::parse_ws_pcdata);
        
        if (!result) return "";

        pugi::xpath_node node = doc.select_node(xpath_query.c_str());
        if (node) {
            return node.node().child_value();
        }
        return "";
    }

    // fallback: clean extraction using regex
    static std::string get_by_regex(const std::string& content, const std::string& pattern) {
        std::regex re(pattern, std::regex::icase);
        std::smatch match;
        if (std::regex_search(content, match, re)) {
            if (match.size() > 1) return match[1].str();
            return match[0].str();
        }
        return "";
    }

    // extract multiple matches
    static std::vector<std::string> get_all_by_regex(const std::string& content, const std::string& pattern) {
        std::vector<std::string> results;
        std::regex re(pattern, std::regex::icase); // case insensitive
        auto begin = std::sregex_iterator(content.begin(), content.end(), re);
        auto end = std::sregex_iterator();
        
        for (std::sregex_iterator i = begin; i != end; ++i) {
            std::smatch match = *i;
            if (match.size() > 1) results.push_back(match[1].str());
            else results.push_back(match[0].str());
        }
        return results;
    }
    // extract multiple matches using xpath
    static std::vector<std::string> get_all_by_xpath(const std::string& html_content, const std::string& xpath_query, const std::string& attribute = "") {
        std::vector<std::string> results;
        pugi::xml_document doc;
        // use extremely loose parsing options for html
        // pugi::parse_default includes wschars | escchars | eol | indent | offset (- ws_pcdata + wconv_attribute...)
        // we want to accept anything
        unsigned int options = pugi::parse_default | pugi::parse_ws_pcdata;
        pugi::xml_parse_result result = doc.load_buffer(html_content.c_str(), html_content.size(), options);
        
        // even if result is partial (status_ok or status_append_restart), we try to query
        // typically pugixml returns status_no_document if completely failed
        
        try {
            pugi::xpath_node_set nodes = doc.select_nodes(xpath_query.c_str());
            for (pugi::xpath_node node : nodes) {
                if (attribute.empty()) {
                    // get text content (recurse handled by pugixml text())
                    std::string val = node.node().text().get();
                    // if empty, maybe try child_value() or value()
                    if (val.empty()) val = node.node().child_value();
                    results.push_back(val);
                } else {
                    // get attribute
                    results.push_back(node.node().attribute(attribute.c_str()).value());
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] XPath Error: " << e.what() << std::endl;
        }
        return results;
    }
    // extract emails using regex
    static std::vector<std::string> get_emails(const std::string& html_content) {
        std::vector<std::string> emails;
        // regex for email
        // 1. mailto
        auto mailtos = get_all_by_regex(html_content, "mailto:([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,})");
        emails.insert(emails.end(), mailtos.begin(), mailtos.end());

        // 2. plain text (simple)
        // avoiding complex regex to prevent stack issues, keeping it simple
        auto plain = get_all_by_regex(html_content, "([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,})");
        emails.insert(emails.end(), plain.begin(), plain.end());

        return emails;
    }
};
