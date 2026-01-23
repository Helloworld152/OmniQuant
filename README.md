# OmniQuant (全视量化)

**Codename**: `Project Aegis`
**Slogan**: Unified Protocol, Universal Access.

OmniQuant is a high-performance, low-latency, distributed quantitative monitoring middle platform.

## Architecture

- **Layer 1: Gateways (C++)** - Connects to CTP/XTP.
- **Layer 2: Core Router (Rust)** - Data enrichment and routing.
- **Layer 3: Message Bus (RabbitMQ)** - Reliable messaging.
- **Layer 4: Data Service (Python)** - Business logic, DB, WebSocket.
- **Layer 5: Dashboard (Vue 3)** - Frontend UI.

## Quick Start

1. Start Infrastructure:
   ```bash
   docker-compose up -d
   ```

2. Build Protocol:
   (Instructions to run scripts/gen_*.sh)

3. Run Components:
   - See `gateway_ctp/README.md`
   - See `core_router/README.md`
   - See `backend_api/README.md`
   - See `web_dashboard/README.md`
