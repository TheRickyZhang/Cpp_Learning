Separate out a session layer (TCP) and application layer (Orders/Quotes)
Session Layer
- Login / ACK
- Heartbeat
- Logout
- SessionReject + disconnect (if violating protocol)

Appliation Layer
- New Order -> ACK
- Cancel Order -> ACK
- Mass Quote -> ACK per-symbol
- Mass Cancel -> ACK
- Get Report: Filled, Expired (IOC / EOD), Cancel (from mass or disconnect)

System Design:
Client Sessions (identity implicit on which socket bytes arrived on, ie ip/port combination)
->
Kernel TCP Stack (sockets + epoll)
->
Ingress `Thread`:
- Setup socket
- Trigger each socket in round-robin style with fixed budget: fair, nonblocking
- Buffer to reassemble messages (do we scramble in TCP?)
- Handle login state
- Decode + timestamp
- Don't read data via recv if matcher is behind?
->
Ingress SPSC ring

->

Matcher/Sequencer `Thread`
(This would be sharded by like underlying symbol first letter)

->
Push to journal ring `thread` (should not fail; shutdown exchange)
Validate data
Update book
Push to TCP ring `thread` publisher (fail -> disconnect client)
Push to MD  ring `thread` publisher (fail -> drop)

- Note that most latency is caused by kernel wakeup + two ring hops
- Later fairness: reorder based on timestamps instead of relying on queue order
- Prefer journaling on separate thread, ACKing immediately
- Startup: load universe → assign instrument IDs → preallocate books/pools → start journal → start MD (publish an instrument directory, see §4) → open listen socket → accept logins → set state OPEN

```cpp
// Often times might pad for alignment - implementation detail, add later
struct Login {
  char     client_id[8];
  char     token[16]; // Something like client_id + password
  uint32_t last_seen_seq;
  uint32_t _pad;
};
struct NewOrder {
  uint64_t cl_ord_id;
  uint32_t option_id;
  uint8_t  side;
  uint8_t  is_IOC;
  uint16_t _pad;
  int64_t  price;
  uint32_t qty;
  uint32_t _pad2;
};
struct OrderAck {
  uint64_t cl_ord_id;
  uint64_t order_id;
  uint64_t ts;
};
struct Fill {
  uint64_t cl_ord_id;
  uint64_t order_id;
  uint64_t exec_id;
  uint64_t trade_id;
  int64_t  price;
  uint32_t fill_qty;
  uint32_t leaves_qty;
  uint8_t  is_maker;
  uint64_t ts;
};
struct MassQuote {
  uint64_t quote_msg_id;
  uint16_t count;
  struct Entry {
    uint32_t instrument_id;
    uint32_t _pad;
    int64_t  bid_px; uint32_t bid_qty;
    uint32_t ask_qty; int64_t ask_px;
  } entries[count];
};
```

Market data tends to be published before order-entry confirmation or at same time.
