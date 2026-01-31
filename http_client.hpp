#pragma once
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <optional>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "utils.hpp"

using json = nlohmann::json;

// manages proxies and headers
class http_client {
private:
    std::deque<std::string> proxies;
    std::vector<std::string> user_agents;
    std::mutex proxy_mutex;
    int request_count = 0;

public:
    http_client(const std::string& proxy_file, const std::string& headers_file) {
        // load proxies
        if (std::ifstream(proxy_file).good()) {
             proxies = std::deque<std::string>(std::deque<std::string>::allocator_type());
             auto loaded_proxies = utils::load_lines(proxy_file);
             for(auto p : loaded_proxies) {
                 if (p.empty() || p[0] == '#') continue;
                 
                 // auto-convert ip:port:user:pass format to http://user:pass@ip:port
                 // verify by checking for 3 colons and no protocol prefix
                 int colons = 0;
                 for(char c : p) if(c == ':') colons++;
                 
                 if (colons == 3 && p.find("://") == std::string::npos) {
                     size_t first = p.find(':');
                     size_t second = p.find(':', first + 1);
                     size_t third = p.find(':', second + 1);
                     
                     if (first != std::string::npos && second != std::string::npos && third != std::string::npos) {
                        std::string ip = p.substr(0, first);
                        std::string port = p.substr(first + 1, second - first - 1);
                        std::string user = p.substr(second + 1, third - second - 1);
                        std::string pass = p.substr(third + 1);
                        p = "http://" + user + ":" + pass + "@" + ip + ":" + port;
                     }
                 }
                 
                 proxies.push_back(p);
             }
             std::cout << "[DEBUG] Loaded " << proxies.size() << " proxies from " << proxy_file << std::endl;
        } else {
             std::cerr << "[WARNING] Could not open proxy file: " << proxy_file << " (Using direct connection)" << std::endl;
        }

        // load headers
        std::ifstream hf(headers_file);
        if (hf.good()) {
            json j;
            try {
                hf >> j;
                if (j.contains("user_agents")) {
                    for (const auto& ua : j["user_agents"]) user_agents.push_back(ua);
                }
                std::cout << "[DEBUG] Loaded " << user_agents.size() << " User-Agents from " << headers_file << std::endl;
            } catch(...) {
                std::cerr << "[ERROR] JSON parse error in " << headers_file << std::endl;
            }
        } else {
            std::cerr << "[WARNING] Could not open headers file: " << headers_file << std::endl;
        }
    }

    // executes a get request with auto rotation
    cpr::Response get(const std::string& url) {
        int retries = 3;
        while (retries > 0) {
            std::string current_proxy;
            std::string user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"; // default fallback

            {
                std::lock_guard<std::mutex> lock(proxy_mutex);
                if (!proxies.empty()) {
                    current_proxy = proxies.front();
                    // rotate every 10 requests
                    request_count++;
                    if (request_count % 10 == 0) {
                        proxies.push_back(proxies.front());
                        proxies.pop_front();
                    }
                }
                if (!user_agents.empty()) {
                    user_agent = user_agents[utils::random_int(0, user_agents.size() - 1)];
                }
            }
            
            // debug print
            // std::cout << "[DEBUG] Fetching " << url << " with proxy: '" << (current_proxy.empty() ? "DIRECT" : current_proxy) << "'" << std::endl;

            cpr::Session session;
            session.SetUrl(cpr::Url{url});
            if (!current_proxy.empty()) {
                session.SetProxies(cpr::Proxies{{"http", current_proxy}, {"https", current_proxy}});
            }
            session.SetHeader(cpr::Header{{"User-Agent", user_agent}, {"Accept-Language", "de-DE,de;q=0.9"}});
            session.SetTimeout(cpr::Timeout{10000}); // 10s timeout
            session.SetVerifySsl(cpr::VerifySsl{false}); // disable ssl verify
            session.SetVerbose(cpr::Verbose{false}); // disable verbose logging for clean output

            // random delay before request
            utils::random_sleep(4000, 15000); 

            auto response = session.Get();

            if (response.status_code == 200) {
                return response;
            } else if (response.status_code == 429 || response.status_code == 403) {
                // ban detected, rotate proxy immediately
                std::lock_guard<std::mutex> lock(proxy_mutex);
                if (!proxies.empty()) {
                    proxies.push_back(proxies.front());
                    proxies.pop_front();
                }
                utils::random_sleep(10000, 30000); // long backoff
            }
            
            retries--;
        }
        return cpr::Response{}; // return empty/failed
    }

    // post request for webhooks (direct, no proxy rotation needed usually)
    cpr::Response post(const std::string& url, const std::string& json_payload) {
        std::cout << "[INFO] Posting data to " << url << "..." << std::endl;
        cpr::Session session;
        session.SetUrl(cpr::Url{url});
        session.SetBody(cpr::Body{json_payload});
        session.SetHeader(cpr::Header{{"Content-Type", "application/json"}});
        session.SetTimeout(cpr::Timeout{30000}); // 30s timeout
        session.SetVerifySsl(cpr::VerifySsl{false});
        return session.Post();
    }
};
