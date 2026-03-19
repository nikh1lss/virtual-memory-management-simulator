# Virtual Memory Address Translation Simulator

A C program that simulates virtual memory address translation, including page table lookup, demand paging with FIFO replacement, and TLB caching with LRU eviction.

## Overview

The simulator reads 32-bit logical addresses from an input file and translates them to physical addresses using a 22-bit addressing scheme (10-bit page number + 12-bit offset). It supports four incremental task levels, each adding a layer of memory management complexity.

## System Parameters

| Parameter | Value |
|---|---|
| Logical address space | 22 bits |
| Page number bits | 10 |
| Offset bits | 12 |
| Number of pages | 1024 |
| Number of frames | 256 |
| Frame size | 4096 bytes |
| TLB entries | 32 |

## Building

```sh
gcc -o translate translate.c -Wall -Wextra
```

## Usage

```sh
./translate -f <address_file> -t <task>
```

| Flag | Description |
|---|---|
| `-f` | Path to the input file containing one logical address per line |
| `-t` | Task level to run: `task1`, `task2`, `task3`, or `task4` |

### Example

```sh
./translate -f addresses.txt -t task3
```

## Output Format

Each logical address produces one or more lines of output depending on the task level.

**All tasks:**
```
logical-address=<addr>,page-number=<pn>,offset=<off>
```

```
page-number=<pn>,page-fault=<0|1>,frame-number=<fn>,physical-address=<pa>
```

```
evicted-page=<pn>,freed-frame=<fn>
```

```
tlb-hit=<0|1>,page-number=<pn>,frame=<fn|none>,physical-address=<pa|none>
tlb-remove=<pn|none>,tlb-add=<pn>
tlb-flush=<pn>,tlb-size=<n>
```

```

- **Page table** is a flat array of 1024 entries indexed directly by page number.
- **Frame table** tracks which page occupies each of the 256 physical frames.
- **FIFO replacement** uses a circular `oldestFrame` pointer that advances on each eviction.
- **TLB** uses an LRU policy driven by an `accessCount` counter stamped onto each entry on use; on eviction from the page table, the matching TLB entry is flushed immediately.
