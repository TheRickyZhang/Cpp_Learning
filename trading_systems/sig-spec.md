# PROMPT
I'm trying to effectively design an options exchange. Here are the requirements:

A price-time options exchange in C++ on Linux. Main goal is to work on the exchange protocol and wire spec.
Inputs loaded at startup: the options universe to trade, and a client whitelist.

Connections: TCP, only whitelisted clients, one session each.
Order types: DAY and IOC, limit only. New and cancel (no cancel-replace)
Quotes: two-sided, atomic both-side replace, post-only, exactly one resting quote per (client, symbol)
Matching: price-time, per options
Market data: multicast feed - trades and top 3 levels per side per symbol.

Messages we must support (in order of client -> exchange, exchange -> client):
Log on / logon response
New Order / ACK
Cancel Order / order status (fill, ack, expired)
Mass quote (two-sided) / ACK

The exact wire format - fields, sizes, framing, sequence numbers, error codes - is your design.

I think we should also have the following messages:

Heartbeat, logout, mass cancel, resend / snapshot requests, trade correction.
Design points to consider:
- What messages do we need?
- What types do we use?
- How to handle protocol errors?
- How do we know clients are up?
- What's the wire format & framing?


Additionally, ensure you keep these ideas in mind:
Recording
- Logging
- Auditing
- Replay & determinism
- Observability
- Threading & handoff
- Hot-path discipline
- Startup & shutdown
- Failure handling
- Testing & tooling

My process for designing this is: let's trace the full path of all the messages we support, and try to separate the steps into logically independent categories. Then, once we have a good picture of that, we can start thinking about throughput and fairness, which are the main architectural challenges of this system. Obviously, for a first-pass spec, we do want to keep things relatively simple, but also keep latency / throughput / fairness in mind, such as having dedicated threads and load balancing, etc.
# Matching Engine Project Notes

## 1. Big Picture

This project sounds like a **simulation / replay environment around a low-latency matching engine**.

The important public/common idea is:

```text
replay file
  -> market-data / event replay
  -> order generation or echo agent
  -> matching engine
  -> multicast UDP market-data output
```

Architecture:

```text
outside network
  -> service/gateway box
  -> controller / orchestration
  -> physical engine box
  -> matching engine container
  -> multicast output
```

---

## 2. Common vs Proprietary

### Common / Preparable

These are standard concepts in exchange-like or trading-system infrastructure:

- **Matching engine**
  - Deterministic order-book state machine.
  - Handles add, cancel, replace, market order, partial fill, trade generation.
  - Usually emphasizes strict sequencing and reproducibility.

- **Limit order book**
  - Price-time priority is the default matching rule to understand first.
  - Maintains bids and asks by price level.
  - Needs fast lookup by order ID for cancel/replace.

- **Replay system**
  - Reads a historical or canned event file.
  - Reconstructs a stream of inputs.
  - Used for testing, simulation, debugging, and performance validation.

- **Market-data publishing**
  - UDP multicast is common for low-latency market-data distribution.
  - Messages usually include sequence numbers.
  - Receivers need gap detection and book reconstruction.

- **Physical engine hosts**
  - Common for latency-sensitive systems.
  - Gives better control over CPU isolation, NUMA locality, NIC placement, and jitter.

- **Kernel bypass networking**
  - Used to avoid normal kernel networking overhead.
  - Common concepts: RDMA, raw Ethernet queue pairs, DPDK, AF_XDP, busy polling.

- **Mellanox / NVIDIA NICs**
  - Common in high-performance networking.
  - Support RDMA, RoCE, raw packet queues, hardware timestamping, and offloads.

### Likely Proprietary / Internal

These probably refer to internal infrastructure, naming, file formats, or operational details:

- **TSIM**
  - Likely an internal trading simulation environment or network segment.

- **Andromeda matching engine**
  - Likely the internal engine or engine wrapper name.

- **`.ncr` files**
  - Probably an internal or vendor-specific replay format.

- **MDF tools**
  - Likely internal or vendor-specific market-data tooling.

- **market echo agent**
  - Internal component name for replaying, echoing, or generating market/order flow.

- **tree / dev boxes / service box / controller / engine / web box**
  - Common role pattern, but exact topology is internal.

---

## 3. Interpreting the Host Layout

A likely interpretation:

```text
tree = isolated TSIM environment / segment

inside the tree:
  - dev boxes: shared development machines
  - service box: gateway / entry point
  - controller: orchestration and control plane
  - engine: physical host running hot-path engine
  - web box: shared UI / monitoring / control interface
```

Key point: the **engine box is separate and physical**, which suggests the hot path is latency-sensitive and needs more controlled hardware behavior.

---

## 4. Replay Flow

Your notes suggest this flow:

```text
.ncr file
  -> market-data replay
  -> market echo agent
  -> orders / events
  -> TSIM Andromeda matching engine
  -> multicast UDP output
```

This is the useful abstraction:

```text
input event log
  -> decoder
  -> deterministic engine
  -> output event log / market-data feed
```

Important properties:

- same input should produce same output;
- events should be sequenced;
- timestamps may be replay-time, simulated-time, or original exchange-time;
- output should be checkable against expected fills/book updates;
- failures should be reproducible from logs.

---

## 5. Matching Engine Core Concepts

A basic matching engine should support:

- add limit order;
- cancel order;
- replace / modify order;
- market order;
- partial fills;
- full fills;
- trade reports;
- book updates;
- order ID lookup;
- per-symbol book partitioning.

Most important invariant:

```text
same ordered input stream -> same exact output stream
```

This is why many engines use a **single-writer event loop** per symbol, product, or partition.

---

## 6. Limit Order Book Design

Common structure:

```text
Order ID map:
  order_id -> order pointer / metadata

Bid book:
  price -> FIFO queue of buy orders

Ask book:
  price -> FIFO queue of sell orders
```

For price-time priority:

- best bid = highest buy price;
- best ask = lowest sell price;
- earlier order at same price has priority;
- aggressive orders match resting orders on the opposite side.

Common implementation choices:

- `std::map` / tree by price: easier, slower but fine for learning;
- flat arrays by tick index: faster if price range is bounded;
- intrusive linked lists: common in high-performance designs;
- object pools: avoid hot-path allocation.

---

## 7. UDP Multicast Market Data

UDP multicast is common because one publisher can send to many subscribers with low overhead.

Typical message fields:

```text
sequence number
message type
symbol id
side
price
quantity
order id or level id
timestamp
```

Receiver responsibilities:

- track expected sequence number;
- detect packet gaps;
- request recovery or reload snapshot;
- reconstruct book state;
- handle duplicate or out-of-order packets if feed design allows them.

Common feed pattern:

```text
snapshot + incremental updates
```

The snapshot gives the full book state. Incrementals update it afterward.

---

## 8. RDMA / Raw QP / Kernel Bypass

Do not over-focus on full RDMA semantics at first. The likely relevant concept is **kernel bypass packet I/O**.

Normal socket path:

```text
application -> syscall -> kernel networking stack -> NIC
```

Kernel bypass path:

```text
application -> NIC queue directly / mostly directly
```

Useful vocabulary:

- **NIC**: network interface card;
- **QP**: queue pair;
- **CQ**: completion queue;
- **MR**: memory region;
- **RoCE**: RDMA over Converged Ethernet;
- **raw Ethernet QP**: send/receive raw packets through verbs;
- **busy polling**: spin instead of blocking to reduce latency;
- **huge pages**: large memory pages, often used in high-performance packet I/O.

Important distinction:

```text
Mellanox + QP does not always mean one-sided RDMA reads/writes.
```

It may just mean raw packet TX/RX with lower overhead than normal sockets.

---

## 9. CPU / NUMA / Physical Host Notes

Physical engine boxes matter because latency-sensitive systems care about jitter.

Concepts to know:

- **CPU pinning**: keep hot threads on specific cores;
- **NUMA locality**: keep thread, memory, and NIC close to each other;
- **IRQ affinity**: control which cores handle NIC interrupts;
- **isolated cores**: reserve cores for hot-path processes;
- **cache locality**: avoid bouncing data across cores;
- **false sharing**: avoid unrelated hot variables sharing a cache line.

More cores do not automatically make a matching engine faster.

Often the bottleneck is ordered state mutation, so a single book/partition may be intentionally single-threaded for determinism.

---

## 10. What to Prepare

### Highest Value

1. Build a simple deterministic matching engine.
2. Add replay from a file.
3. Emit market-data events.
4. Add sequence numbers and gap detection.
5. Add UDP multicast publisher/subscriber.
6. Profile and reduce allocation in the hot path.
7. Learn CPU pinning and NUMA basics.

### Good Mini-Project

Build this locally:

```text
orders.txt
  -> replay.cpp
  -> engine.cpp
  -> market_data_publisher.cpp
  -> multicast subscriber
  -> reconstructed book
```

Start with text input. Later convert to binary.

Example input:

```text
1 ADD 1001 AAPL B 100 10
2 ADD 1002 AAPL S 101 5
3 ADD 1003 AAPL B 101 3
4 CANCEL 1001
```

Expected engine behavior:

- order `1003` crosses with sell order `1002`;
- trade is generated at resting order price;
- remaining quantities are updated;
- book update is emitted.

---

## 11. What Not to Over-Study

Lower priority unless internal docs require it:

- exact meaning of `TSIM`;
- exact `.ncr` file format;
- exact Andromeda APIs;
- exact MDF command syntax;
- exact host naming conventions.

Those are probably learned fastest from internal documentation or onboarding.

Public prep should focus on transferable concepts:

```text
matching rules
order-book data structures
deterministic replay
market-data feeds
UDP multicast
kernel bypass networking
NUMA / CPU affinity
low-latency C++ design
```

---

## 12. Concise Mental Model

A matching engine project is not just an order book.

It is usually a full pipeline:

```text
input data
  -> deterministic event sequencing
  -> order-book state transition
  -> execution generation
  -> market-data publication
  -> replay/debug/recovery tooling
```

The most important engineering themes are:

- determinism;
- sequencing;
- latency;
- reproducibility;
- observability;
- controlled networking;
- controlled hardware placement.
  rsorsa tns rt  eiorst
