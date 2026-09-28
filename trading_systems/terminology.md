# General timeline:
Exchange timestamp
NIC receives
Book updated
Strategy / Risk
Serialize Order
NIC sends

# Market Data
Generally, three levels of market data:
L1: just top of book, last trade
L2: Aggregated by price
L3: Full order fidelity

Types of Orders:
Bolt: order that is obviously good, race to fill.
Chip: order that has been partially filled
XXX: Order that was not good, but is now due to a change in underlying

Types of order patterns:
Blast: when someone makes a large order on one side very quickly


FIX Protocol = Business semantics, uniform across different exchanges (typically FIX TagValue = ASCII format)
-> Which is represented as SBE (simple binary encoding)

# Reference Data
May want to load dynamically each day, describe instrument ID -> ticker, tick size, price scale multiplier values

# Ambiguous state problem
Exchange's state is definite, but our local knowledge of that state generally is:
- No ACK
- Disconnect before getting ACK

Diagram of state transitions:
Pending New -> Live/Partially Filled -> Pending Cancel or Replace
-> No ack: unknown
-> filled, cancelled, or rejected

# Ways to reconcile:
- Replay Events                    = preserves history on same session. Needs gap tracking.
- Drop Copy (CCing own executions) = Works with replay as independent information source
- Mass status snapshot             = quick mismatch check, state may still be ambiguous

Generally, stop trading when reconciling.
Also need to explicitly exclude duplicates.
Use sequence numbers internally

# NYSE Arca
Most surprising behavior:
You are allowed to decrease size of partially filled order to below what has already matched: instead of rejecting the message, it will effectively cancel the order with leavesQty = 0

Note a lot of very low-latency performance work is trying to reverse engineer market microstructure to leverage
- Ordering guarantee when sending messages very close to each other
- Route to specific gateway / matching engine shard for faster processing
- Infer batching cadence, time distribution for stages

