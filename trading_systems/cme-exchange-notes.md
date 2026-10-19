Notes from CME Group Client Systems Wiki Home

[https://cmegroupclientsite.atlassian.net/wiki/spaces/EPICSANDBOX/overview?homepageId=457314687]

# Exchanges and Channels
CME Group owns 4 exchanges:
- CME (ES futures, SOFR)
- NYMEX (Crude oil, natural gas)
- CBOT (Corn, Wheat)
- COMEX (Gold, silver, copper)
Everything uses Globex matching engine platform

Has channels, which cover specific product subsections
- ES (S&P Mini) gets its own channel
- Futures and Options are always on different channels, and are premium-only

# Market Data Feeds
Unless specified, assume default is UDP
Multiple different axes

## Real-time vs conflated
Real-time: Also called "premium", emits events in real-time
Conflated: updates merged by certain time window (loses some info)

## Book Granularity
Market By Price (MBP): aggregated depth per level.
Market By Order Full Depth: individual orders

## Transport
UDP: default
TCP: generally uses FIX

## Feed Types
Incremental Feed: Normal
- Also publishes definitions that update in real time

Market Recovery Feeds
- Repeatedly publishes full snapshots. Contains last seq num seen to sync with recovery.

TCP Message Recovery
- Publishes message history, limited scale since TCP.

Instrument Definition Feeds: same idea, repeatedly publishes instrument definitions

Everything has an A/B feed for redundancy.

# MDP 3.0 Protocol
## Packet Structure
Packets can contain multiple MDP messages, which is slightly unusual
- Most other exchanges have sequencing per message instead of packet

Events are logical groupings, and can be spanned across multiple packets.

## Message Ordering Within an Event
Specific order:
Trades -> Cumulative volume update -> Order Book -> Session stats -> implied (ex. combining two outrights of calendar spread)
- Makes sense because an aggressor will generate matches first, which is also the most important info.

## Event Indicators
We need some way to indicate "last event" of a specific type. Possible ways to do so as an exchange:
- Announce up front: requires buffering whole event
- Add start/end of event messages: requires two extra messages
- Add a flag when it's the last message of a type: best overall

## Book Updates (MBP)
MBP updates are indexed by position (kth best) rather than price
Delete/change commands update the book (not trades)

## Trade Summary
Summary provided for an entire match action, which contains an entry per price. Also provides the aggregate and order-level view. 
(NASDAQ ITCH sends one summary per resting order, in contrast)

Only sent when at least one real order (not implied) gets traded

## Statistics
Session Statistics - Byproduct of matching. Mostly derivable, for regulatory and authoritative purposes

Daily Statistics - Processes outside the matching engine, like settlement prices (policy-specific vwap)

# Implied Matching
Basically, the matching engine continuously and automatically computes implied prices across linked books and will execute all legs at once if hit.
- Implied = general noun: a specific instantiation for futures is calendar spreads, composed of two outrights
- Exchanges commonly turn it on/off, because computationally heavy and can cause unwanted behavior at low liquidity
- Desired because better price discovery, shared liquidity

Because there are many futures per product, the exchange also manages implied instruments in spreads. (Only first-degree affects, and is always less priority than direct customers)
- Implied in = bid and offer imply spread
- Implied out = spread and side imply other side
Basically, what this means is that orders can generate seemingly unrelated trade report information as well

# Reference Data and SecDef
## Sources
- Schema Dissemination = protocol spec
- Market Data Config = connectivity details
- Security Definition File = reference data / secdef file
- Market Schedule File
- Reference Data API

## Correlating Order Entry with Market Data
Secdef is the rosetta stone that helps translate

## User-Defined Instruments
Users can also define their own instrument by sending a secdef request, which if successful gets and ACK and publishes on MD
- Necessary, because most trading is multi-leg to manage risk.
- Exchange provides common combinations
- Very prevalent for options market makers; very helpful to detect these new definitions and instantly price them

## Futures SecDef Quirks
- Treasury futures quoted as whole + x/32 + y/8th of a 1/32
- CME runs different allocation rules per product
- Some product are lump-sum (1000 barrels of oil), while others are spread across the month (and thus will decay per day)

# Market States
## Session Stages
Pre-Open: Accumulating orders. Can cross.
Pre-Open No Cancel: 30s where can't cancel or modify orders.
Opening: Brief period of uncrossing
Open:
(Pause): potential halt mid-session, needs mini opening to resume
Close Final: eliminates day orders
Post-Close

## Futures Contract States
Some situations in which contract is in different state
- Reserved: price jumps too much, freeze and do auction before resuming
- Forbidden: expired
- Pre-Cross/Cross: allowing other participants to a trade agreed upon in private
- Activation: indicating new contract

# Order Entry
## Timestamps and Latency
Three timestamps on order
Goes from our NIC -> CME Market Segment Gateway (MSGW) -> Match Engine -> MSGW -> our NIC
CME stamps at 3 timestamps:
- gateway time: when request reaches MSGW
- match time: When matching engine matches, per its clock
- send time: When ACK was sent by gateway

The time diffs we care about:
- internal -> gateway: measures cost of our colo / stack
- Gateway-to-Engine / CME jitter: gateway -> match
  - Important because it's only step where orders contend and can change priority
- Engine-to-Send: match -> send
- internalTime - SendingTimeEpoch: cost of our receive pipeline

Round-trip-time: our send -> our receive
Wire-to-wire: our recv NIC to send NIC

Because edge is so small in delta-one, queue position is very important. (similar to greeks in options)

## In-flight Mitigation
- Specific feature of CME that basically means: if you send a cancel-replace for N, even if your existing order partially filled, you still will only get at most N exposure total

CME are anonymous

How to create own internal state

See position earlier


## Quirks
Some firms tried to "game" order entry by pre-building an entire order and sending most but not all of it before actually submitting the final packets to fully trigger it.

This optimization works locally, where we want to stage the packet in the NIC and only commit to sending once decided, but exchanges don't like it because it is resource abuse.

So, they introduce a 3us of processing time per detection

(Note that other exchanges chose regulation instead of software penalty)
