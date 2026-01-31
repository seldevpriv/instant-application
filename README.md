# 📝 C++17 Job Aggregator (Research Project)

High-performance, multi-threaded C++ application demonstrating modern networking, HTML parsing, and asynchronous system architecture.

> **Note for Recruiters/Reviewers:** This project was developed as an educational exploration of C++17 standards, multithreading (`std::thread`, `std::future`), and HTTP protocol implementation (`cpr`/`libcurl`). It showcases ability to handle complex system challenges like rate limiting, concurrency control, and data sanitization.

## ✨ Features

### 🛡️ Technical Highlights
- **Consolidated Architecture**: Aggregates data from multiple sources (**LinkedIn**, **Indeed**, **StepStone**) into a unified data structure.
- **Intelligent Pipeline**: Implements tokenization and strict location filtering (`locations.txt`) to ensure high data relevance.
- **Data Integrity**: 
  - **Deduplication algorithm**: Uses hash-based sets to eliminate redundant entries.
  - **Transactional Saving**: Implements incremental IO flushing to ensure data persistence during long-running tasks.

### ⚡ Advanced Networking
- **Deep Extraction**: Asynchronous traversal of detail pages to parse metadata (e.g. contact info) via Regex/XPath.
- **Webhook Integration (n8n)**: Automatically serializes and pushes results via POST requests to external automation workflows.
- **Resilient Request System**: 
  - Configurable proxy rotation middleware.
  - Ethical rate-limiting implementation (Jitter/Backoff strategies).
- **Concurrency**: Scalable thread-pool implementation for parallel processing.

## 📋 Prerequisites
- CMake 3.14+
- C++17 Compiler (MSVC, GCC, Clang)
- Internet Connection

## 🛠️ Build Instructions

1. **Configure**:
   ```bash
   cmake -B build -S .
   ```

2. **Build**:
   ```bash
   cmake --build build --config Release
   ```

## 💻 Usage

Run the scraper from the command line:

```bash
./build/Release/scraper.exe --sites all --location "Stuttgart" --keywords "Fachinformatiker,C++" --output jobs.json --pages 5
```

### ⚙️ Arguments
- `--sites`: Comma-separated sites (indeed, stepstone, linkedin) or 'all'.
- `--location`: Target city or region.
- `--keywords`: Search terms.
- `--output`: Path to save JSON results (default: `jobs.json`).
- `--threads`: Number of threads.
- `--pages`: Number of pages to scrape per site.
- `--deep`: **(New)** Enable deep scraping to visit job pages and extract emails.
- `--webhook`: **(New)** URL to send the JSON result payload to (POST request).
- `--proxies`: Path to proxy list (default: `proxies.txt`).
- `--headers`: Path to header config (default: `headers.json`).

### 💡 Examples
**Standard Run:**
```bash
./build/Release/scraper.exe --sites all --location "Stuttgart" --pages 1
```

**Deep Scan with n8n Integration:**
```bash
./build/Release/scraper.exe --sites all --location "Stuttgart" --pages 1 --deep --webhook "http://localhost:5678/webhook"
```

## 🔧 Configuration
- **proxies.txt**: Add proxies in `scheme://ip:port` or `ip:port:user:pass` format (handled by cpr/curl).
- **headers.json**: Modify user agents and referers to match your target demographic.

## ⚖️ Legal Disclaimer & Terms of Use

**⚠️ EDUCATIONAL PROJECT ONLY - PLEASE READ CAREFULLY**

This software is a **student project** created strictly for **educational and learning purposes** (researching C++, HTTP networking, and HTML parsing). It is **NOT** intended for commercial use, data mining, or mass scraping.

1.  **No Liability**: The author assumes no responsibility for any consequences arising from the use of this software. You use it entirely at your own risk.
2.  **Respect `robots.txt`**: This tool technically ignores `robots.txt` to function as a browser emulator. By using this tool, you acknowledge that you are bypassing standard automated access controls.
3.  **Terms of Service**: Scraping data may violate the Terms of Service (ToS) of the target platforms (LinkedIn, Indeed, StepStone). Use of this tool may lead to your IP address or account being blocked.
4.  **GDPR/DSGVO Compliance**: If you extract personal data (like names or email addresses via `--deep`), you are responsible for handling this data in compliance with local data protection laws (e.g., GDPR in Europe). Do not publish or sell scraped data.
5.  **Do Not Misuse**: Do not use this tool to spam, harass, or overload the servers of the target platforms.

**By downloading or running this software, you agree to use it only for personal learning and testing.**
