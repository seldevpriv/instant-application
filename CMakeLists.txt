cmake_minimum_required(VERSION 3.14)
project(UltimateJobScraper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# cpr for http requests
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5 # use a stable release tag
)
# nlohmann_json for json handling
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
# pugixml for html parsing
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
)

FetchContent_MakeAvailable(cpr json pugixml)

add_executable(scraper main.cpp)

target_link_libraries(scraper PRIVATE cpr::cpr nlohmann_json::nlohmann_json pugixml::pugixml)
