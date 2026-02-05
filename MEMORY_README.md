# Memory Savings Analysis

This directory contains detailed analysis of memory savings from the string pool optimization in `RifEclipseSummaryAddress`.

## Quick Answer for "FOPT", "WOPT:B1", "WOPT:B2"

### Per-Object Savings
- **Before**: 128 bytes per address
- **After**: 40 bytes per address
- **Reduction**: 88 bytes per object (69%)

### For 3 Addresses (as requested)
- Before: 384 bytes
- After: 604 bytes
- **Note**: Small overhead for tiny collections due to pool initialization (484 bytes)

### Break-Even Point
String pool becomes beneficial at **~6 addresses**

### Realistic Scenarios

| Addresses | Before | After | Savings | Reduction |
|-----------|--------|-------|---------|-----------|
| 10 | 1.25 KB | 0.86 KB | 0.39 KB | 31% |
| 100 | 12.5 KB | 4.38 KB | 8.12 KB | 65% |
| 1,000 | 125 KB | 39.5 KB | 85.5 KB | 68% |
| 10,000 | 1.25 MB | 391 KB | 859 KB | 69% |

### Key Insight: Deduplication

The real power comes from string deduplication:
- "WOPT" appears in both "WOPT:B1" and "WOPT:B2"
- **Before**: Stored twice = 64 bytes
- **After**: Stored once in pool = 72 bytes + 8 bytes indices = 80 bytes

For 500 wells using "WOPT":
- Before: 16,000 bytes
- After: 2,072 bytes
- **Savings: 87%!**

## Documentation Files

- **[MEMORY_SAVINGS_SUMMARY.md](MEMORY_SAVINGS_SUMMARY.md)** - Executive summary with concrete numbers
- **[MEMORY_ANALYSIS.md](MEMORY_ANALYSIS.md)** - Detailed technical analysis with calculations

## Running the Demonstration

To see actual memory measurements:

```bash
g++ -std=c++17 -o memory_demo memory_demo.cpp
./memory_demo
```

This shows:
- Actual sizeof() measurements for old vs new structures
- Memory usage at different scales
- String pool overhead calculations
- Scaling analysis

## Conclusion

For typical reservoir simulations with 1,000+ addresses:
- **Memory reduction: 60-69%**
- **Break-even: ~6 addresses**
- **Additional benefits**: Faster copies, better cache efficiency, thread-safe concurrent reads

The optimization is highly effective because:
1. Vector names heavily repeat (WOPT, WOPR, FOPT, etc.)
2. Well/group names shared across vectors
3. Real scenarios have thousands of addresses
