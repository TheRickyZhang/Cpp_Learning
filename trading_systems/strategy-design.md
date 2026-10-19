There are two choices for how we want to modify an order.
- Atomic cancel/replace: guarantees we only have one order out at a time.
- Separate cancel and new: can place new order faster, at risk of being double-exposed. Generally preferred because faster and simpler.

On CME, MBP and MBO are both in same message, with MBP coming before MBO as a sort of summary.
- This means that we can interpret just MBO itself to make decisions faster (fast path for decisions)
- We can also have a slower path that reads MBO and maintains full book / position

