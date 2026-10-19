# Market Data
Exchanges use UDP for MD for efficiency and fairness (multiplex to everyone)
Publish feeds at various detail (L1/2/3)

They send two streams of data for redundancy
- Maintain feed handler that listens to both, keeps the first sequence number, detects gaps and handles out of order.

When a gap is detected, multiple possibilities:
- Ask for retransmit of specific sequence range
- Subscribe to periodic snapshot channel, and reapply buffered incremental changes after the latest snapshot
- Do nothing / try book healing on own


# Book Building
- Takes in market data and builds up queryable state
- Also needs reference data to interpret raw storage (ex integer prices) -> real values (real prices)

# Strategy
Split into alpha and execution engine.
- Alpha consumes market data and produces positions.
- Execution engine consumes positions and executes trades

Why split?
- By decoupling desired positions from executions, both units can be tested independently
- Alphas wanting opposite positions can cancel each other out

# Risk Checks
- Position/exposure/rate limits, self-trade prevention, kill switch
- Checks and logs required by SEC

# Order Manager
- Between send order and ACK, can be in many different state
- Pending order -> Rejected or Acked -> Partial Fill or Pending Cancel -> Filled or Canceled

- Owns client order id: an id we assign to each order, so that we can track state before the exchange gives back its id. Maintain map of <clordid, id>

# Order Gateway
Owns the session with the venue
- Session state has different lifecycle than order state

# Position Keeper
# Hedger
Separate from alpha, because it is a reaction to fills

## Journal
Log every inbound/outbound message, write to memory-mapped ring
Required by SEC

## Telemetry
Timestamp at ingress and egress

## Controller
- Has quick switches for strategy params, pulling orders, etc.
- If single-thread, the engine can do non-blocking recv from controller socket
- For multi-threaded, maintain a dedicated control thread that determines which thread to push events to, which are busy-polled in queue.
- Kill event is special; maintain shared memory atomic bool so that every thread can be notified

## Backtester
- Make matching engine model
- Simulation results can't be fully accurate, especially with market impact

## Post-Trade
Clearing and Settlement

