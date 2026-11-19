# Employee Performance KPI Dashboard (CLI)

A lightweight, terminal-based Key Performance Indicator (KPI) dashboard written in C to track, analyze, and manage employee performance metrics directly from the command line.

## Features

* **Employee Record Management:** Add, update, view, and remove employee profiles and performance data.
* **KPI Tracking & Scoring:** Calculate overall performance scores based on core metrics (e.g., task completion rate, attendance, quality score, and efficiency).
* **CLI Data Visualization:** Render clean tabular reports and ASCII progress bars directly in the terminal.
* **Performance Filtering & Ranking:** Sort employees by KPI scores to identify top performers and team averages.
* **Persistent Storage:** Save and load employee records locally using file handling (`CSV` / `TXT` / binary files) *(remove if in-memory only)*.

## Tech Stack

* **Language:** C (C99 / C11 standard)
* **Compiler:** GCC / Clang / MSVC
* **Interface:** Command-Line Interface (CLI)

## Getting Started

### Prerequisites

Ensure you have a C compiler installed on your system (such as `gcc`):
```bash
gcc --version
