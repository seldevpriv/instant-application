#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <future>
#include <nlohmann/json.hpp>
#include <algorithm>
#include "http_client.hpp"
#include "parser.hpp"
#include "utils.hpp"

using json = nlohmann::json;

struct job_listing {
    std::string title;
    std::string company;
    std::string location;
    std::string url;
    std::string source;
    std::string date_posted;
    std::string email; // new field

    json to_json() const {
        return json{
            {"title", title},
            {"company", company},
            {"location", location},
            {"url", url},
            {"source", source},
            {"date_posted", date_posted},
            {"email", email} // export email
        };
    }
    
    // generate hash for deduplication
    std::size_t get_hash() const {
        return std::hash<std::string>{}(url + title + company);
    }
};

class scraper {
private:
    http_client& client;
    std::vector<std::string> keywords;
    std::string location;
    std::vector<std::string> allowed_locations;
    
public:
    scraper(http_client& c, const std::vector<std::string>& k, const std::string& l) 
        : client(c), keywords(k), location(l) {
            allowed_locations = utils::load_lines("locations.txt");
            if (!allowed_locations.empty()) {
                std::cout << "[INFO] Location filtering enabled: " << allowed_locations.size() << " allowed locations." << std::endl;
            }
        }

    // generic scrape execution
    std::vector<job_listing> scrape_site(const std::string& site_name, int pages) {
        std::vector<job_listing> jobs;
        
        for (const auto& k : keywords) {
            std::cout << "scraping " << site_name << " for keyword '" << k << "' (" << pages << " pages)..." << std::endl;
            for (int i = 0; i < pages; ++i) {
                std::string search_url = build_url(site_name, i, k);
                auto response = client.get(search_url);
                
                if (response.status_code != 200) {
                    std::cerr << "failed to fetch " << search_url << " status: " << response.status_code 
                              << " error: " << response.error.message << std::endl;
                    continue;
                }

                auto page_jobs = parse_jobs(site_name, response.text);
                
                // count valid jobs for this page
                int valid_count = 0;
                for(const auto& j : page_jobs) {
                    if (is_relevant(j.title, j.location)) valid_count++;
                }
                
                // actually filter them now (previously parse_jobs did filtering? no, parse_jobs calls is_relevant inside loop)
                // wait, parse_jobs ALREADY filters. So page_jobs contains ONLY valid jobs.
                // To get raw count, I need to modify parse_jobs or just trust the previous logs "status: 0".
                
                // Let's rely on the fact that if status is 0, we continue above.
                // If status 200 but 0 jobs, it's a parsing mistmatch.
                 
                jobs.insert(jobs.end(), page_jobs.begin(), page_jobs.end());
                
                // log progress
                std::cout << site_name << " (" << k << ") page " << (i+1) << ": returned " << page_jobs.size() << " valid jobs." << std::endl;
            }
        }
        return jobs;
    }
    
    // deep scrape: fetch details and extract email
    void enrich_job(job_listing& job) {
        if (job.url.empty()) return;
        
        // rudimentary check to avoid non-http links
        if (job.url.find("http") != 0) return;

        std::cout << "[DEEP] Checking " << job.url << "..." << std::endl;
        auto response = client.get(job.url);
        if (response.status_code == 200) {
            auto emails = parser::get_emails(response.text);
            if (!emails.empty()) {
                // remove duplicates
                std::sort(emails.begin(), emails.end());
                emails.erase(std::unique(emails.begin(), emails.end()), emails.end());
                
                // join
                for (size_t i = 0; i < emails.size(); ++i) {
                    job.email += emails[i] + (i < emails.size() - 1 ? ", " : "");
                }
                std::cout << "[DEEP] Found email(s): " << job.email << std::endl;
            }
        } else {
             // specific handling for indeed redirects could go here, but keeping it simple
        }
    }

private:
    std::string build_url(const std::string& site, int page, const std::string& keyword) {
        std::string query_str = keyword;
        // simple url encoding
        std::replace(query_str.begin(), query_str.end(), ' ', '+');

        if (site == "indeed") {
            return "https://de.indeed.com/jobs?q=" + query_str + "&l=" + location + "&start=" + std::to_string(page * 10);
        } else if (site == "stepstone") {
             // simplified logic for stepstone url structure
            return "https://www.stepstone.de/jobs/" + query_str + "/in-" + location + "?page=" + std::to_string(page + 1);
        } else if (site == "linkedin") {
            // linkedin guest job search url
            return "https://www.linkedin.com/jobs/search?keywords=" + query_str + "&location=" + location + "&start=" + std::to_string(page * 25);
        }
        return "";
    }

    std::vector<job_listing> parse_jobs(const std::string& site, const std::string& html) {
        std::vector<job_listing> jobs;
        
        if (site == "indeed") {
            // indeed regex extraction (heuristic)
            auto raw_titles = parser::get_all_by_regex(html, "class=\"jcs-JobTitle[^\"]*\">([^<]*)<");
            auto raw_companies = parser::get_all_by_regex(html, "data-testid=\"company-name\">([^<]*)<");
            auto raw_locs = parser::get_all_by_regex(html, "data-testid=\"text-location\">([^<]*)<");
            
            size_t limit = (std::min)({raw_titles.size(), raw_companies.size()});
            for(size_t i=0; i<limit; ++i) {
                job_listing job;
                job.title = raw_titles[i];
                job.company = raw_companies[i];
                job.location = (i < raw_locs.size()) ? raw_locs[i] : location;
                job.source = "indeed";
                if (is_relevant(job.title, job.location)) {
                     jobs.push_back(job);
                } else {
                     // std::cout << "[DEBUG] Filtered Indeed job: " << job.title << std::endl;
                }
            }
            if (raw_titles.size() > 0 && jobs.size() == 0) {
                 std::cout << "[WARNING] Indeed parsed " << raw_titles.size() << " items but ALL were filtered out!" << std::endl;
            } else if (raw_titles.size() == 0) {
                 std::cout << "[WARNING] Indeed parsed 0 items. HTML structure might have changed or blocking active." << std::endl;
            }
        } else if (site == "stepstone") {
            // stepstone extraction
            auto raw_titles = parser::get_all_by_regex(html, "<h2[^>]*>([^<]*)</h2>");
             for(const auto& t : raw_titles) {
                 job_listing job;
                 job.title = t;
                 job.source = "stepstone";
                 // stepstone location is vague in list view, using global location for check if generic
                 job.location = location; 
                 if (is_relevant(job.title, job.location)) jobs.push_back(job);
             }
            if (raw_titles.size() > 0 && jobs.size() == 0) {
                 std::cout << "[WARNING] StepStone parsed " << raw_titles.size() << " items but ALL were filtered out!" << std::endl;
            } else if (raw_titles.size() == 0) {
                 std::cout << "[WARNING] StepStone parsed 0 items. HTML structure might have changed or blocking active." << std::endl;
            }
        } else if (site == "linkedin") {
            // linkedin extraction (regex with multiline support)
            // Title: h3 class="base-search-card__title"
            auto raw_titles = parser::get_all_by_regex(html, "base-search-card__title[^>]*>((?:.|[\\r\\n])*?)<\\/h3>");
            
            // Company: class="hidden-nested-link"
            auto raw_companies = parser::get_all_by_regex(html, "hidden-nested-link[^>]*>((?:.|[\\r\\n])*?)<\\/a>");
            
            // URL: class="base-card__full-link" href="..."
            auto raw_urls = parser::get_all_by_regex(html, "base-card__full-link[^>]*href=\"([^\"]*)\"");
            
            // Location: class="job-search-card__location"
            auto raw_locs = parser::get_all_by_regex(html, "job-search-card__location[^>]*>((?:.|[\\r\\n])*?)<\\/span>");

            // use titles as base count
            size_t limit = raw_titles.size();
             for(size_t i=0; i<limit; ++i) {
                 job_listing job;
                 job.title = utils::trim(raw_titles[i]);
                 if (i < raw_companies.size()) job.company = utils::trim(raw_companies[i]);
                 if (i < raw_urls.size()) job.url = raw_urls[i];
                 if (i < raw_locs.size()) job.location = utils::trim(raw_locs[i]);
                 else job.location = location;
                 
                 job.source = "linkedin";
                 if (is_relevant(job.title, job.location)) jobs.push_back(job);
             }
        }
        
        return jobs;
    }

    bool is_relevant(const std::string& title, const std::string& job_location = "") {
        // 1. Keyword Check (Title)
        bool keyword_match = false;
        std::string lower_title = utils::to_lower(title);
        
        for(const auto& k : keywords) {
             std::string lower_k = utils::to_lower(k);
             if (lower_title.find(lower_k) != std::string::npos) {
                 keyword_match = true;
                 break;
             }
             // fuzzy check
             double score = utils::fuzzy_match(lower_title, lower_k);
             if (score > 0.8) {
                 keyword_match = true;
                 break;
             }
        }
        if (!keyword_match) return false;

        // 2. Location Check (if enabled)
        if (!allowed_locations.empty()) {
            if (job_location.empty()) {
                 std::cout << "[DEBUG] Rejecting '" << title << "' (Empty Location)" << std::endl;
                 return false; 
            }
            
            std::string lower_loc = utils::to_lower(job_location);
            bool loc_match = false;
            for(const auto& loc : allowed_locations) {
                if (lower_loc.find(utils::to_lower(loc)) != std::string::npos) {
                    loc_match = true;
                    break;
                }
                if (utils::to_lower(loc).find(lower_loc) != std::string::npos) {
                    loc_match = true;
                    break;
                }
            }
            if (!loc_match) {
                 std::cout << "[DEBUG] Rejecting '" << title << "' at '" << job_location << "' (Location Filter)" << std::endl;
                 return false;
            }
        }
        std::cout << "[DEBUG] Accepted '" << title << "' at '" << job_location << "'" << std::endl;
        return true;
    }
};
