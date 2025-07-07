# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Dash Core is a cryptocurrency project extending Bitcoin Core with advanced features including masternodes, instant payments, privacy mixing, and decentralized governance. The codebase is primarily C++ using C++20 standard.

## Build Commands

### Setup and Build
```bash
# Generate build system
./autogen.sh

# Configure (common options)
./configure                           # Basic configuration
./configure --disable-wallet         # Build without wallet
./configure --without-gui            # Build dashd only
./configure --with-incompatible-bdb  # Use system Berkeley DB

# Build with parallel jobs
make -j$(nproc)

# Memory-constrained systems
./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"
```

### Development Environment Setup
```bash
# Fast setup (Ubuntu 24.04)
./setup-dev-env-fast.sh

# Full setup with all tools
./setup-dev-env.sh
```

## Testing Commands

### Unit Tests
```bash
# Run all unit tests
make check

# Run specific test
src/test/test_dash --run_test=getarg_tests

# Debug unit tests
gdb src/test/test_dash
```

### Functional Tests
```bash
# Run all functional tests
test/functional/test_runner.py

# Run specific test
test/functional/wallet_hd.py

# Extended test suite
test/functional/test_runner.py --extended

# Parallel execution
test/functional/test_runner.py -j$(nproc)

# Debug options
test/functional/test_runner.py --nocleanup --tracerpc -l DEBUG
```

### Code Quality
```bash
# Run all linting
test/lint/all-lint.py

# Common individual checks
test/lint/lint-python.py
test/lint/lint-shell.py
test/lint/lint-whitespace.py
test/lint/lint-circular-dependencies.py
```

## High-Level Architecture

### Core Extension Pattern
Dash extends Bitcoin Core through composition rather than core modification, using a layered architecture:

1. **Bitcoin Core Foundation**: Blockchain, consensus, networking
2. **Special Transaction Framework**: Generic extension system for new transaction types
3. **Masternode System**: Deterministic masternode lists and management
4. **LLMQ System**: Distributed consensus for advanced features
5. **Dash Services**: CoinJoin, governance, InstantSend, ChainLocks

### Key Architectural Components

#### Masternode System (`src/masternode/`, `src/evo/`)
- **Deterministic Masternode Lists**: Consensus-critical registry using immutable data structures
- **Active Masternode Manager**: Local masternode operations and BLS key handling
- **Special Transactions**: ProRegTx, ProUpServTx, ProUpRegTx, ProUpRevTx for masternode lifecycle

#### Long-Living Masternode Quorums (`src/llmq/`)
- **Quorum Types**: Multiple configurations (50/60, 400/60, 400/85) for different services
- **Distributed Key Generation**: Cryptographically secure quorum formation
- **Services**: ChainLocks (51% attack prevention), InstantSend, governance voting

#### CoinJoin Privacy (`src/coinjoin/`)
- **Mixing Architecture**: Masternode-coordinated mixing sessions
- **Denomination System**: Uniform outputs for privacy
- **Session Management**: Multi-party transaction construction

#### Decentralized Governance (`src/governance/`)
- **Governance Objects**: Proposals, triggers, superblock management
- **Treasury System**: Automated payouts based on governance votes
- **Voting Validation**: On-chain proposal voting and tallying

#### Evolution Database (`src/evo/evodb.h`)
- **Specialized Storage**: Masternode snapshots, quorum state, governance objects
- **Efficient Updates**: Differential updates for masternode lists
- **Credit Pool Management**: Platform integration support

### Integration Patterns

#### Initialization Flow
1. **Basic Setup**: Core Bitcoin initialization
2. **Parameter Interaction**: Dash-specific configuration validation
3. **Interface Setup**: Dash manager instantiation in NodeContext
4. **Main Initialization**: EvoDb, masternode system, LLMQ, governance startup

#### Consensus Integration
- **Block Validation Extensions**: Special transaction validation
- **Mempool Extensions**: Enhanced transaction relay
- **Chain State Extensions**: Masternode list and quorum state tracking
- **Fork Prevention**: ChainLocks prevent reorganizations

#### Key Design Patterns
- **Manager Pattern**: Centralized managers for each subsystem
- **Event-Driven Architecture**: ValidationInterface callbacks coordinate subsystems
- **Immutable Data Structures**: Efficient masternode list management using Immer library
- **Extension Over Modification**: Minimal changes to Bitcoin Core foundation

### Critical Interfaces
- **NodeContext**: Central dependency injection container
- **ValidationInterface**: Event distribution for block/transaction processing
- **ChainstateManager**: Enhanced with Dash-specific validation
- **MessagesSerializer**: Special transaction serialization
- **BLS Integration**: Cryptographic foundation for advanced features

## Development Workflow

### Common Tasks
```bash
# Clean build
make clean && make distclean

# Run dashd with debug logging
./src/dashd -debug=all -printtoconsole

# Run functional test with custom dashd
test/functional/test_runner.py --dashd=/path/to/dashd

# Generate compile_commands.json for IDEs
bear -- make -j$(nproc)
```

### Debugging
```bash
# Debug dashd
gdb ./src/dashd

# Profile performance
test/functional/test_runner.py --perf
perf report -i /path/to/datadir/test.perf.data --stdio | c++filt

# Memory debugging
valgrind --leak-check=full ./src/dashd
```

## Branch Structure

- `master`: Stable releases
- `develop`: Active development (built and tested regularly)

## Important Notes

- Use `make -j$(nproc)` for parallel builds to speed up compilation
- Always run linting before commits: `test/lint/all-lint.py`
- For memory-constrained systems, use special CXXFLAGS during configure
- Special transactions use payload extensions - see `src/evo/specialtx.h`
- Masternode lists use immutable data structures for thread safety
- LLMQ quorums have different configurations for different purposes