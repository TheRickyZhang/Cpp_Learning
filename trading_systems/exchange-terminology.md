# Protocols
| Exchange | Order entry | Market data | Encoding / transport |
|---|---|---|---|
| Nasdaq | **OUCH** | **ITCH** | SoupBinTCP (entry), MoldUDP64 (data) |
| CME | iLink | MDP 3.0 | **SBE** |
| Cboe | BOE | PITCH | custom binary |
| NYSE | Pillar | Pillar Integrated Feed | custom binary |

FIX - universal tag=value option (slow)
SIP - consolidated US top-of-book

# Market Data
L1 = top of book and last trade
L2 = deep book with aggregated size
L3 = order-level granularity

# Types of orders:
Most support limit and market with TIF field.

Additional types
- Post-only (doesn't match)
- Iceberg orders: Only some quantity is displayed / available, but gets automatically topped up when used
  Block orders: under-the-table
  Different execution strategies - distributing orders to keep more hidden
- Pegged orders: keep updating around midpoint. Will update faster than manual management, but doesn't give control.

Stop order = not actually an order; waits for price trigger to then submit an order.

Wash trade = self-trade to create false impression of market activity

# Various
ES = futures ticker for Electronic S&P 500 Futures, traded on CME

MBP = market by price (L2)
MBO = market by orders (L3)

# Things that are traded
Asset Class: broad category, ie futures, options, equities, FX
Product: specific underlying and contract spec for how its traded
Instrument: anything tradable with its own security ID / book.
Can either be:
- Outright: A single contract / expiry, aka single-leg
- Spread: Package of multiple contracts, such as calendar spreads, butterflies

Relative roles:
Leg = part of a spread. In this sense, it is an outright of a spread


Underlying = the thing the derivative is based on
Outright = a contract that is just traded by itself; not included anything else

Futures Price

Exchanges trade trees
VIX - SPX -> implied

Auctions always held with new product

NIC -> wire
Multiple packets per message

Serializing - Send 4 packets out of the 5
Sending packet?
CME will penalize if not full-formed

Message Format
How orders processed through networks itself
MDP - iLink

Good queue pos is important because it also gives more / earlier information when filled
Called "canaries", potentially warning of iceberg orders


Timestamp of when public: Reconstruct accurate state of orderbook
- Tied to anonymous id? Or not

Improve fidelity of test env
- Convexity lot more variable in prod, fix it

Onload + ef_vi -> don't need kernal trap via syscall
Onload still uses socked implementation, caching behavior hurtful for cache reasons
ef_vi -> packets constructed on ASIC - CTPIO
- Quoters not on fully bare-metal (didn't use linux's idea of sockets)
