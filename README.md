
# Beehive Simulation Project

## Project Overview

The Beehive Simulation project is a multi-process application that simulates the lifecycle and interactions of a hive. It incorporates different actors like bees, the queen, and a beekeeper to model the hive's dynamic behavior, including synchronization and resource management.

The project employs shared memory and semaphores for inter-process communication, making it an excellent example of parallel programming techniques in a C environment.

---

## Project Structure

```
.
├── src                # Directory containing source (.c) files
│   ├── main.c         # Entry point of the simulation
│   ├── common.c       # Contains common functions (shared memory, logging, etc.)
│   ├── bee.c          # Implementation of the bee process
│   ├── queen.c        # Implementation of the queen process
│   ├── beekeeper.c    # Implementation of the beekeeper process
├── include            # Directory containing header (.h) files
│   ├── common.h       # Header for common utilities and definitions
│   ├── bee.h          # Header for the bee process
│   ├── queen.h        # Header for the queen process
│   ├── beekeeper.h    # Header for the beekeeper process
├── .vscode            # Directory containing VS Code configuration files
├── Makefile           # Build script to compile the project
```

---

## How It Works

1. **Main Simulation (`src/main.c`)**:
   - Initializes shared memory for hive data and semaphores.
   - Spawns the queen, beekeeper, and initial bee processes.
   - Monitors and cleans up child processes.

2. **Queen Process (`src/queen.c`)**:
   - Periodically lays eggs and spawns new bees.
   - Ensures the hive doesn’t exceed its capacity.

3. **Bee Process (`src/bee.c`)**:
   - Simulates the lifecycle of worker bees, including entering and exiting the hive.
   - Uses semaphores for controlled access to hive resources.

4. **Beekeeper Process (`src/beekeeper.c`)**:
   - Responds to signals (e.g., `SIGUSR1` to add hive frames, `SIGUSR2` to remove frames).
   - Monitors and manages hive resources dynamically.

5. **Common Utilities (`src/common.c`)**:
   - Provides shared memory management, logging, and error handling utilities.

---

## How to Build and Run

### Prerequisites

- GCC (GNU Compiler Collection)
- Make utility

### Steps to Run the Simulation

1. **Clone the Repository**
   ```bash
   git clone <repository_url>
   cd <repository_name>
   ```

2. **Build the Project**
   Use the provided Makefile to compile the project:
   ```bash
   make
   ```

3. **Run the Simulation**
   Execute the simulation program with the following command:
   ```bash
   ./beehive_simulation <N> <T_k> <eggsCount>
   ```
   - **N**: Initial hive size (number of frames).
   - **T_k**: Time interval (seconds) between the queen's egg-laying cycles.
   - **eggsCount**: Number of eggs laid per cycle.

   Example:
   ```bash
   ./beehive_simulation 10 5 2
   ```

4. **Signals for Dynamic Management**
   - Add hive frames: `kill -SIGUSR1 <beekeeper_pid>`
   - Remove hive frames: `kill -SIGUSR2 <beekeeper_pid>`
   - Terminate the simulation: `kill -SIGINT <beekeeper_pid>`

---

## Key Features

- **Multi-Process Simulation**: Models real-time hive behavior using processes for queen, bees, and beekeeper.
- **Shared Memory & Semaphores**: Implements efficient inter-process communication and synchronization.
- **Dynamic Hive Management**: Adjusts hive capacity dynamically through signal handling.
- **Robust Error Handling**: Includes detailed logging and cleanup mechanisms to manage resources.

---

## Logging

Logs are stored in `beehive.log` and printed to the console based on the log level settings in `src/common.c`. The log levels include:
- `DEBUG`: Developer-level details.
- `INFO`: General operational information.
- `WARNING`: Alerts for potential issues.
- `ERROR`: Critical failures that affect execution.

---

## Cleanup

When the simulation ends:
- Shared memory and semaphores are released.
- All child processes (bees, queen, beekeeper) are terminated gracefully.

---

## Notes

- Ensure the initial hive size (`N`) is less than or equal to the `MAX_BEES` constant (default: 1000).
- The Makefile simplifies the build process and ensures proper compilation of all source files.

---

Enjoy exploring the world of bee simulation!
