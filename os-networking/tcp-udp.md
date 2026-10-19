# Why prefer TCP / UDP in specific usages in trading systems?
UDP
- Replication is done by switches in hardware. Still needs to do O(n) writes
- Doesn't store any client state
- Think of it like writing to shared memory. Only cost is a pipelineable pointer enqueue, supported by ASIC hardware
- Number of switch transfers scales with log n, as it forms a tree

TCP: Replication is done by the sender, so needs N sockets, and sends through N copies of bytes through NIC and kernel
Stores substantial state: send and receive buffer, retransmit timers
Needs to manage slow clients, high variable latency


TCP/IP model layers (ATN DP): application, transport, network, data link, physical.

IP: Internet Protocol (Networking layer L3)
- Routes packets from source to destination using IP addresses
- No guarantee for packet delivery

TCP: Transmission Control Protocol (Transport, L4)
- 3-way handshake for reliable delivery

UDP: User Datagram Protocol (Transport, L4)
- Lightweight, no guarantee
