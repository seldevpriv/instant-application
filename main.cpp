#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <fstream>
#include <chrono>
#include <algorithm>
#include "scraper.hpp"
#include "utils.hpp"
#include "http_client.hpp"

// simple argument parser
std::map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0, 2) == "--") {
            std::string key = arg.substr(2);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args[key] = argv[++i];
            } else {
                args[key] = "true";
            }
        }
    }
    return args;
}

void print_help() {
    std::cout << "usage: ./scraper --sites all --location \"stuttgart\" --keywords \"cpp,dev\" --output jobs.json --proxies proxies.txt --threads 4 --pages 5\n";
    std::cout << "options:\n";
    std::cout << "  --sites       comma separated list of sites (indeed,stepstone,linkedin) or 'all'\n";
    std::cout << "  --location    location to search for\n";
    std::cout << "  --keywords    comma separated keywords\n";
    std::cout << "  --output      output file path (json)\n";
    std::cout << "  --proxies     proxy file path\n";
    std::cout << "  --headers     header config file path\n";
    std::cout << "  --threads     number of threads (default: 1)\n";
    std::cout << "  --pages       pages per site to scrape (default: 1)\n";
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argc, argv);
    
    if (args.empty() || args.count("help")) {
        print_help();
        return 0;
    }

    bool deep_scrape = args.count("deep");

    std::string location = args.count("location") ? args["location"] : "Deutschland";
    std::string proxies_file = args.count("proxies") ? args["proxies"] : "proxies.txt";
    std::string headers_file = args.count("headers") ? args["headers"] : "headers.json";
    std::string output_file = args.count("output") ? args["output"] : "jobs.json";
    std::string webhook_url = args.count("webhook") ? args["webhook"] : "";
    int threads_num = args.count("threads") ? std::stoi(args["threads"]) : 1;
    int pages = args.count("pages") ? std::stoi(args["pages"]) : 1;
    
    std::string keywords_str = args.count("keywords") ? args["keywords"] : "anwendungsentwicklung,software entwicklung";
    std::vector<std::string> keywords;
    std::stringstream ss(keywords_str);
    std::string segment;
    while(std::getline(ss, segment, ',')) {
        keywords.push_back(segment);
    }
    
    std::cout << "starting scraper..." << std::endl;
    std::cout << "location: " << location << std::endl;
    std::cout << "keywords: " << keywords_str << std::endl;
    std::cout << "threads: " << threads_num << std::endl;

    // init client
    http_client client(proxies_file, headers_file);
    
    // sites to scrape
    std::vector<std::string> sites;
    if (args.count("sites")) {
        std::string s_arg = args["sites"];
        if (s_arg == "all") {
            sites = {"stepstone", "indeed", "linkedin"};
        } else {
            std::stringstream ss2(s_arg);
            while(std::getline(ss2, segment, ',')) sites.push_back(segment);
        }
    } else {
        sites = {"stepstone"};
    }

    std::vector<job_listing> all_jobs;
    std::mutex jobs_mutex;
    std::vector<std::thread> threads;
    std::mutex print_mutex;

    // launch threads
    for (const auto& site : sites) {
        threads.emplace_back([&, site]() {
            try {
                {
                    std::lock_guard<std::mutex> lock(print_mutex);
                    std::cout << "starting thread for " << site << "..." << std::endl;
                }
                scraper s(client, keywords, location);
                auto jobs = s.scrape_site(site, pages);
                
                std::lock_guard<std::mutex> lock(jobs_mutex);
                all_jobs.insert(all_jobs.end(), jobs.begin(), jobs.end());
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(print_mutex);
                std::cerr << "THREAD ERROR (" << site << "): " << e.what() << std::endl;
            } catch (...) {
                std::lock_guard<std::mutex> lock(print_mutex);
                std::cerr << "THREAD ERROR (" << site << "): unknown exception" << std::endl;
            }
        });
    }

    for(auto& t : threads) {
        if(t.joinable()) t.join();
    }

    // deduplicate
    std::cout << "deduplicating " << all_jobs.size() << " jobs..." << std::endl;
    std::sort(all_jobs.begin(), all_jobs.end(), [](const job_listing& a, const job_listing& b) {
        return a.get_hash() < b.get_hash();
    });
    auto last = std::unique(all_jobs.begin(), all_jobs.end(), [](const job_listing& a, const job_listing& b) {
        return a.get_hash() == b.get_hash();
    });
    all_jobs.erase(last, all_jobs.end());

    // deep scrape / enrichment
    if (deep_scrape) {
        // save raw results first (safety save)
        std::cout << "saving " << all_jobs.size() << " initial jobs to " << output_file << " (before deep scan)..." << std::endl;
        {
            json j_out = json::array();
            for(const auto& job : all_jobs) j_out.push_back(job.to_json());
            std::ofstream out(output_file);
            out << j_out.dump(4);
        }

        std::cout << "starting deep scrape (email extraction) for " << all_jobs.size() << " jobs..." << std::endl;
        
        // single threaded for now to avoid ban hammer on detail pages
        // create a temporary scraper instance for utilities (sharing client)
        scraper util_scraper(client, {}, "");
        
        int count = 0;
        for (auto& job : all_jobs) {
            std::cout << "[" << ++count << "/" << all_jobs.size() << "] ";
            util_scraper.enrich_job(job);
            
            // save incrementally every 5 jobs
            if (count % 5 == 0) {
                 json j_out = json::array();
                 for(const auto& j : all_jobs) j_out.push_back(j.to_json());
                 std::ofstream out(output_file);
                 out << j_out.dump(4);
            }
            
            // tiny delay
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    std::cout << "saving " << all_jobs.size() << " unique jobs to " << output_file << std::endl;

    json j_out = json::array();
    for(const auto& job : all_jobs) {
        j_out.push_back(job.to_json());
    }

    std::ofstream out(output_file);
    std::string json_payload = j_out.dump(4);
    out << json_payload;
    
    std::cout << "done." << std::endl;

    if (!webhook_url.empty()) {
        std::cout << "sending results to n8n webhook: " << webhook_url << "..." << std::endl;
        auto response = client.post(webhook_url, json_payload);
        if (response.status_code >= 200 && response.status_code < 300) {
            std::cout << "Gemi-chan says: Daten an n8n gesendet! Status: uwu (" << response.status_code << ")" << std::endl;
        } else {
            std::cerr << "failed to send webhook. status: " << response.status_code << " error: " << response.error.message << std::endl;
        }
    }

    return 0;
}