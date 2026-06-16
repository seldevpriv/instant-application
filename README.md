# 🚀 C++ & n8n Automation Pipeline: Workflow Orchestration via n8n and SMTP in a Docker Container  

This project is a deep-dive educational playground built to understand the mechanics of high-performance web scrapers, data pipelines, and modern automation ecosystems. It connects a low-level, multi-threaded C++ extraction engine with self-hosted n8n workflows inside a Dockerized environment.

Instead of relying on heavy framework abstractions, this project implements core networking concepts from scratch to explore rate-limiting, raw HTML parsing, and asynchronous event-driven automation.

## 🛠️ System Architecture & Workflow

1. **C++ Engine**: Scrapes and normalizes raw data asynchronously across multiple targets.
2. **Data Pipeline**: Sanitizes, tokenizes, and deduplicates payloads in-memory.
3. **Webhook Bridge**: Serializes results into standard JSON and dispatches them via HTTP POST to an n8n endpoint.
4. **n8n Automation**: Orchestrates the incoming payload, filters content, and triggers automated SMTP mail workflows inside an isolated Docker network.

## ✨ Core Learning Objectives & Features

### ⚡ Low-Level & Advanced Networking (C++)
- **Conical Multithreading**: Scalable thread-pool implementation utilizing `std::thread` and `std::future` to master concurrency control and race-condition prevention.
- **Resilient HTTP Client**: Built on top of `cpr`/`libcurl` featuring custom proxy rotation middleware and ethical jitter/backoff strategies to bypass basic anti-bot blocks.
- **Deep DOM Parsing**: Utilizes Regex/XPath mapping to traverse nested detail views for granular metadata extraction (e.g., embedding contact info).

### 🛡️ Data Engineering & Persistence
- **Deduplication Matrix**: In-memory hash-sets preventing redundant entries before pipeline serialization.
- **Transactional Streaming**: Incremental I/O flushing to maintain structured JSON integrity even during unexpected runtime interruptions.

### 🤖 Automation & Containerization
- **n8n Webhook Integration**: Native JSON payloads dispatched straight into webhooks for automated event-triggering.
- **Dockerized Environment**: Fully containerized stack ensuring reproducible network routes between the scraping application, n8n instance, and SMTP relays.

## 📋 Prerequisites
- CMake 3.14+
- C++17 compliant compiler (GCC, Clang, MSVC)
- Docker & Docker Compose (for the full automation stack)

## 🔨 Quick Start & Build Instructions

### Building the C++ Binary
1. **Configure CMake**:
```bash
   cmake -B build -S .
   ```
2. **Compile Project**:
```bash
   cmake --build build --config Release
   ```

### Running the Pipeline Stack
To spin up the n8n environment and automation hooks, run:
```bash
docker-compose up -d
```

## 💻 Usage & Examples

Execute the binary directly from your shell to fetch data and feed the automation pipeline:

```bash
./build/Release/scraper.exe --sites all --location "Stuttgart" --keywords "C++,Linux" --output jobs.json --pages 5
```

### ⚙️ Arguments CLI Reference
- `--sites`: Target endpoints (e.g., `indeed`, `stepstone`, `linkedin`) or `all`.
- `--location`: Structural geo-filtering target.
- `--keywords`: Core search terms.
- `--output`: Filepath for localized fallback storage (default: `jobs.json`).
- `--threads`: Limit or scale worker threads.
- `--pages`: Extraction depth limit per execution block.
- `--deep`: Toggles second-level scanning to follow internal nodes and extract contact links.
- `--webhook`: The target n8n automation link (`POST` request receiver).
- `--proxies`: Path to file containing network proxies.

### 💡 Example Scenarios

**Local Analysis Configuration:**
```bash
./build/Release/scraper.exe --sites all --location "Stuttgart" --pages 1
```

**Full Automation Run (Triggering n8n & SMTP):**
```bash
./build/Release/scraper.exe --sites all --location "Stuttgart" --pages 1 --deep --webhook "http://localhost:5678/webhook/your-automation-id"
```

## 🔧 Internal Configuration Layout
- **proxies.txt**: Supply network proxies using `scheme://ip:port` or `ip:port:user:pass` patterns.
- **headers.json**: Custom HTTP configurations to study browser emulation behavior and header manipulation.

## ⚖️ Disclaimer & Educational Boundaries

**⚠️ STUDY PROJECT ONLY**

This repository was created exclusively for **educational and scientific research purposes** regarding network protocols, multi-threaded architecture patterns, and automation infrastructures. It is **NOT** intended for production environments, real-world data harvesting, or commercial deployment.

1. **Self-Contained Nature**: Use this tool responsibly. It is designed to evaluate how automation pipelines handle high-throughput payloads.
2. **Terms of Service**: Web scraping can conflict with the Terms of Service (ToS) of major platforms. This tool is built to experiment with standard browser emulator techniques.
3. **Data Handling Compliance**: If processing any contact data through the `--deep` flag, ensure compliance with localized data protection policies (e.g., GDPR/DSGVO). Never share or expose scraped logs publicly.
