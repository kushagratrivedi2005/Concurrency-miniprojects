# LAZY Corp - Concurrency Project

## Overview
This project implements concurrent file handling operations with assumptions set to maximize system efficiency while remaining adaptable for various system capabilities.

### Features:
- **LAZYSORT**: Efficient multi-threaded sorting mechanism for file handling.
- **LAZYREADWRITE**: Thread-safe, concurrent read and write operations, ensuring data consistency across multiple user requests.

---

## Assumptions

### LAZYSORT
- **File Name Length**: 
  - The maximum file name length is set to **8 characters**, adjustable via the macro `MAX_FILENAME_LENGTH`.
- **Maximum Threads**:
  - A maximum of **4 threads** is assumed, adjustable via the macro `MAX_THREADS` depending on system capabilities.
- **File Name Format**:
  - File names are restricted to use **26 lowercase English letters** (`a-z`) and the **full stop** (`.`).
- **Timestamp Year Requirement**:
  - For any file containing a timestamp, the **year must be greater than 1900** to be considered valid.

### LAZYREADWRITE
- **File Deletion**:
  - If a file is in the deletion phase, any request to access it will result in an **immediate "Invalid File Does Not Exist" error**.
- **Maximum Files**:
  - The system supports up to **100 files**, adjustable via the macro `MAX_FILES`.
- **Maximum Users**:
  - A maximum of **100 users** can access the system concurrently, adjustable via the macro `MAX_USERS`.
- **Request Limits**:
  - The system can handle up to **1000 requests** at once, with limits adjustable through `MAX_QUEUE_SIZE` and `MAX_REQUESTS` macros.

---

## Configuration
All adjustable limits are defined in the macro section of the code, allowing for fine-tuning based on specific system requirements or constraints.

