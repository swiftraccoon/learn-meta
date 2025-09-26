# learn-meta

A multi-language implementation of an SDRTrunk radio API system using Meta's open-source technologies.

## Project Description

This project implements the same radio call ingestion API in four programming languages (C++, Go, Python, Rust), all built with Meta's Buck2 build system. The implementations share a common architecture using Meta technologies including fbthrift for RPC, Proxygen for HTTP handling, and a React/Relay/GraphQL frontend.

## Technologies Used

### Core Technologies

| Technology | Purpose | Implementation |
|------------|---------|----------------|
| fbthrift | RPC framework | C++, Python, Rust |
| Buck2 | Build system | All languages |
| Proxygen | HTTP gateway | C++ |
| GraphQL | API Gateway | All implementations |
| React | Web dashboard | Frontend |
| Relay | GraphQL client | Frontend |
| PostgreSQL | Database | Shared by all |

## System Architecture

```
┌─────────────────┐
│    SDRTrunk     │
└────────┬────────┘
         │ HTTP/2
         ▼
┌──────────────────────────────────────┐
│      HTTP API Gateway (C++)           │
│         Proxygen Framework            │
│   - Multipart upload handling         │
│   - Routes to implementations         │
│   - Meta's production HTTP stack      │
└──────────────────────────────────────┘
         │
         ├─────────────┬─────────────┬─────────────┬─────────────┐
         ▼             ▼             ▼             ▼             ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│   Go Impl   │ │  C++ Impl   │ │Python Impl  │ │  Rust Impl  │ │  GraphQL    │
│   (Native)  │ │  (fbthrift) │ │ (fbthrift)  │ │ (fbthrift)  │ │   Gateway   │
│  Port 8001  │ │  Port 8002  │ │  Port 8003  │ │  Port 8004  │ │  Port 8005  │
└─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘
         │             │             │             │             │
         └─────────────┴─────────────┴─────────────┴─────────────┘
                                    │
                       ┌──────────────────────────┐
                       │      PostgreSQL          │
                       │   (Shared Database)      │
                       └──────────────────────────┘
                                    │
                       ┌──────────────────────────┐
                       │   React Dashboard        │
                       │   (Relay + GraphQL)      │
                       └──────────────────────────┘
```

## Build System

### Buck2 Integration

The project uses Buck2 as the primary build system with incremental adoption:

- Native build tools maintained in parallel initially
- Buck2 configuration for each language implementation
- Unified build targets for the entire project
- Code generation for Thrift and GraphQL schemas

## Implementation Details

TBD

## Testing

### Unit Tests

- Go: Standard library testing package
- C++: Google Test framework
- Python: pytest framework
- Rust: Built-in testing framework

### Integration Tests

- Unified test suite verifying identical behavior across implementations
- Performance benchmarking between implementations
- All tests orchestrated through Buck2

### Load Testing

- Tools: Locust or K6
- Consistent load testing across all implementations
- Performance metrics comparison

## Project Status

### Current Implementation Tasks

- [ ] Four language implementations (Go, C++, Python, Rust)
- [ ] Buck2 build configuration for all components
- [ ] Proxygen HTTP gateway
- [ ] Shared test suite
- [ ] GraphQL API
- [ ] React dashboard with Relay
- [ ] Buck2 documentation

### Additional Features

- [ ] Prometheus monitoring integration
- [ ] Docker containerization
- [ ] CI/CD pipeline with Buck2
- [ ] Remote execution configuration

## Technical Specifications

### Language-Specific Implementation Details

#### Go Implementation

- Native HTTP server
- PostgreSQL integration via sqlx
- GraphQL using Ent framework
- Buck2 configuration for Go modules

#### C++ Implementation

- fbthrift RPC server
- PostgreSQL integration via libpq++
- Connection pooling
- Audio file handling

#### Python Implementation

- fbthrift with FastAPI
- SQLAlchemy for database operations
- Async request handlers
- Optional audio analysis features

#### Rust Implementation

- fbthrift with Tokio async runtime
- Trait-based storage abstraction
- Zero-copy optimizations
- Memory-safe concurrency patterns

### Frontend Implementation

- React with TypeScript
- Relay for GraphQL data fetching
- Real-time updates via subscriptions
- Buck2 configuration for JavaScript/TypeScript