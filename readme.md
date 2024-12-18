# LAZY Corp Distributed Systems Project

## Overview
Welcome to the LAZY Corp Distributed Systems Project! This repository contains two innovative solutions that embody our core philosophy of "lazy efficiency" - a file manager simulation and a distributed sorting system.

## Project Components

### 1. LAZY Read-Write Simulation (`lazyreadwrite`)
#### Challenge
Simulate a file manager with complex concurrency and resource management constraints.

#### Key Features
- Concurrent file access management
- User request handling with patience limits
- Simulated processing of READ, WRITE, and DELETE operations
- Concurrency control with user access limitations

#### Simulation Rules
- Limited concurrent file access
- 1-second processing delay for requests
- User request cancellation if not processed within time limit
- Unique concurrency rules for different file operations

### 2. LAZY Sort (`lazysort`)
#### Challenge
Implement a dynamic distributed sorting mechanism that adapts to file count.

#### Key Features
- Dual-mode sorting strategy
- Distributed Count Sort for small file sets (< 42 files)
- Distributed Merge Sort for larger file sets (≥ 42 files)
- Flexible sorting based on Name, ID, or Timestamp

#### Sorting Criteria
- Threshold: 42 files
- Sorting columns: Name, ID, Timestamp
- Efficient resource allocation
- Distributed task management

## Input Formats

### LAZY Read-Write
```
r w d           # Operation times
n c T           # Files, concurrent access limit, user patience
u_1 f_1 o_1 t_1 # User requests
...
STOP
```

### LAZY Sort
```
<file_count>
<filename> <id> <timestamp>
...
<sort_column>
```

## Requirements
- Implements advanced concurrency concepts
- Demonstrates distributed computing principles
- Focuses on efficiency and lazy resource management


## Contributing
Embrace the LAZY Corp philosophy: Work smarter, not harder! 🛋️💻