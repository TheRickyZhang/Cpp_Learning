# Prerequisite knowledge:
Normally, CPU code: virtual addresses -> physical RAM address (page tables)
But hardware has:   DMA address       ~= physical RAM address (maybe IOMMU)

Start from the actual payload. Then it gets wrapped in:
- TCP header (note may not preserve delimination)
- IP header
- Ethernet header/ender = largest wrapper
And finally transformed to changing voltage / or modulating light

The RX queue is filled with completed DMA entries - NIC pushes items, and driver consumes (polls) items.
A driver is code that knows how to control a specific device. It is part of the kernel.

- Hard IRQ: Triggered by physical hardware, immediately changes CPU from executing userspace code to kernel code.
- Soft IRQ: Generally run immediately after hard IRQ, in same kernel context for optimization. But can be interrupted by other hard IRQs.
Normal use: hard = notify that NIC queue has work, soft = NAPI = actually processing the packets from the queue

PCIe = high-speed communication lane from CPU (NICs, GPUs)
Driver = translator between linux networking stack and specific hardware
struct sock = Linux's encapsulation of socket state, while struct socket = public higher-level wrapper

Socket: kernel communication endpoint between user processes and kernel networking stack
Note that in user code we have fds, which are only valid in our process's file descriptor table.

Regular model for handling sockets is: epoll_wait on sockets that have data, then recv() on them.
Overall, io_uring generally better, immediately doing some action on data when it's ready and notifying on completion, rather than listing out the ready / recv / do steps. A little more machinery.

# Concise answer
Normally the NIC DMAs into kernel buffers, and receiving data requires an interrupt, the kernel network stack, a socket wakeup, and a copy into userspace. With ef_vi, I give the NIC buffers that are mapped into my own process, so it DMAs frames directly into my memory and signals completion on a ring I can read from userspace. A pinned thread busy-polls that ring instead of waiting on interrupts, so there’s no kernel entry, no context switch, and no copy. The cost is getting raw Ethernet frames and needing to handle protocol parsing, but that was abstracted away in an internal library that SIG gave to us.

# Normal Path (Remember this!)
Bytes go from:
- NIC -> DMA to kernel -> maybe copy to userspace

NIC gets Ethernet frame, selects RX queue
        ↓
NIC DMAs packet into next RX descriptor buffer address and marks descriptor complete
        ↓
NIC raises an IRQ                 [often coalesced, not per packet]
        ↓
CPU runs hard IRQ handler
    masks/defer interrupts
    schedules NAPI
        ↓
NET_RX_SOFTIRQ
        ↓
driver's NAPI poll() the RX queue (until no more to process, or reach limit. Basically, keep polling if available, otherwise use interrupt-driven)
    process completed descriptors
    create/manage skb representation
    possibly XDP / GRO / checksum offload handling
    replenish RX descriptors with new buffers
        ↓
Linux network stack
    Ethernet → IP → UDP/TCP → socket lookup
        ↓
socket receive queue
        ↓
wake blocked recv()/epoll_wait()
        ↓
typically copy payload kernel → userspace with recv()

# Kernel Bypass Path
NIC gets Ethernet frame, selects userspace-accessible buffer
 ↓
NIC DMAs packet into next RX descriptor buffer address and marks descriptor complete (we now need to manage buffers)
 ↓
userspace CPU polls descriptor ring
 ↓
application reads packet directly
 ↓
application handles required protocol logic
Note that most things happen in kernel: when raising hard irq, the CPU stops and 

## Types of Kernel Bypass
Vertical (get work off the critical path): three decisions.

Stack: kernel sockets → Onload → ef_vi/TCPDirect. Pick based on how much TCP you’re willing to own.
Transmit: DMA → PIO → CTPIO. Fewer PCIe round-trips before the first byte hits the wire.
Where the logic runs: software → FPGA. Only for the simplest, most time-critical reactions.

Horizontal (make it predictable): two habits.

Isolate: dedicated cores, pinned threads, busy-polling, no interrupts, C-states off.
Pre-allocate: no malloc, no page faults, no syscalls, no locks after startup.

## Types of Kernel Bypass
- Solarflare ef_vi / Generic DPDK: Raw userspace NIC access for market data
- OpenOnload: bypass kernel, but keep TCP semantics
- Userspace TCP/IP stack for order entry, exposes as TCP sockets (TCPDirect)

Normally, from NIC to application, the path is:

NIC:
- receives Ethernet frame, does some initial processing
- sends to specific RX queue
After 
- DMA to memory
- Marks the RX descriptor complete

NIC generates an interrupt / hard IRQ, followed by a soft IRQ, processed by kernel to perform NAPI in loop {
  Polls until empty or budget reached for efficiency
  builds sk_buff (socket buffer = metadata) around packet
  Linux may run GRO (generic received offload), which batches packets to improve throughput at cost of latency. *Turn off* for trading systems
  Recycles RX buffer / descriptor when appropriate (only one of NIC, driver can modify buffer / descriptor)
}

Now it is finally handled by Linux:
- Process Ethernet info, IP info -> UDP or TCP
- UDP: just performs socket lookup
- TCP: handles sequences numbers / duplicates / out-of-order / ACK state
Goes to sock{}.

Now that we have content in the socket, recv() or epoll_wait() successfully wake the corresponding process.
Normally, kernel will copy the packet to userspace, but there are also options to DMA for zero-copy as well, while keeping TCP handling.

