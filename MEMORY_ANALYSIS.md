# Memory Reduction Analysis for Specific Address Examples

This document provides a detailed memory analysis for the string pool optimization in `RifEclipseSummaryAddress`, using concrete examples from reservoir simulation.

## Example Addresses

We'll analyze three typical summary addresses:
1. **"FOPT"** - Field Oil Production Total (field category, 1 string: vectorName)
2. **"WOPT:B1"** - Well Oil Production Total for well B1 (well category, 2 strings: vectorName + wellName)
3. **"WOPT:B2"** - Well Oil Production Total for well B2 (well category, 2 strings: vectorName + wellName)

## Memory Layout Analysis

### Before Optimization (Using std::string)

Each `RifEclipseSummaryAddress` object contained:

```cpp
class RifEclipseSummaryAddress {
    SummaryCategory m_category;      // 4 bytes (enum)
    StatisticsType  m_statisticsType; // 4 bytes (enum)
    std::string     m_vectorName;     // 32 bytes (typical libstdc++ SSO)
    std::string     m_name;           // 32 bytes (well/group/network name)
    std::string     m_lgrName;        // 32 bytes (LGR name)
    int             m_number0;        // 4 bytes
    int             m_number1;        // 4 bytes
    int             m_number2;        // 4 bytes
    bool            m_isErrorResult;  // 1 byte
    int             m_id;             // 4 bytes
    // + padding: ~7 bytes
};
```

**Total per object: ~128 bytes** (on 64-bit system with typical std::string implementation)

Note: Modern std::string implementations use Small String Optimization (SSO), typically:
- 24-32 bytes per string object (depending on implementation)
- Strings up to ~15-23 characters stored inline
- Longer strings require heap allocation (additional cost)

### After Optimization (Using String Pool + Indices)

Each `RifEclipseSummaryAddress` object now contains:

```cpp
class RifEclipseSummaryAddress {
    SummaryCategory m_category;        // 4 bytes (enum)
    StatisticsType  m_statisticsType;  // 4 bytes (enum)
    uint32_t        m_vectorNameIdx;   // 4 bytes (index)
    uint32_t        m_nameIdx;         // 4 bytes (index)
    uint32_t        m_lgrNameIdx;      // 4 bytes (index)
    int             m_number0;         // 4 bytes
    int             m_number1;         // 4 bytes
    int             m_number2;         // 4 bytes
    bool            m_isErrorResult;   // 1 byte
    int             m_id;              // 4 bytes
    // + padding: ~3 bytes
};
```

**Total per object: ~44 bytes**

Plus shared string pool (one-time cost):
```cpp
class RiaStringPool {
    std::shared_mutex               m_mutex;         // ~40 bytes
    std::vector<std::string>        m_strings;       // 24 bytes + content
    std::unordered_map<string, idx> m_stringToIndex; // 56 bytes + content
    uint32_t                        m_emptyIndex;    // 4 bytes
};
```

## Detailed Memory Calculation for Example Scenario

### Scenario: 3 Address Objects
- 1x "FOPT" (field address)
- 1x "WOPT:B1" (well address)
- 1x "WOPT:B2" (well address)

### Memory Usage BEFORE Optimization

**Address 1: "FOPT"**
- m_category: 4 bytes
- m_statisticsType: 4 bytes
- m_vectorName = "FOPT": 32 bytes (string object)
- m_name = "": 32 bytes (empty string object)
- m_lgrName = "": 32 bytes (empty string object)
- Other fields: 21 bytes
- Total: **125 bytes**

**Address 2: "WOPT:B1"**
- m_category: 4 bytes
- m_statisticsType: 4 bytes
- m_vectorName = "WOPT": 32 bytes (string object)
- m_name = "B1": 32 bytes (string object)
- m_lgrName = "": 32 bytes (empty string object)
- Other fields: 21 bytes
- Total: **125 bytes**

**Address 3: "WOPT:B2"**
- m_category: 4 bytes
- m_statisticsType: 4 bytes
- m_vectorName = "WOPT": 32 bytes (string object, DUPLICATE of Address 2!)
- m_name = "B2": 32 bytes (string object)
- m_lgrName = "": 32 bytes (empty string object)
- Other fields: 21 bytes
- Total: **125 bytes**

**Total Memory Before: 375 bytes**

### Memory Usage AFTER Optimization

**String Pool (One-time cost):**
- Pool overhead: ~124 bytes
- Strings stored:
  - "" (empty): 32 bytes (index 0, pre-allocated)
  - "FOPT": 36 bytes (32 byte string object + 4 byte hash entry)
  - "WOPT": 36 bytes (32 byte string object + 4 byte hash entry)
  - "B1": 34 bytes (32 byte string object + 2 byte string + hash entry)
  - "B2": 34 bytes (32 byte string object + 2 byte string + hash entry)
- Pool subtotal: **296 bytes** (one-time cost, shared by all addresses)

**Address 1: "FOPT"**
- m_category: 4 bytes
- m_statisticsType: 4 bytes
- m_vectorNameIdx: 4 bytes (index to "FOPT")
- m_nameIdx: 4 bytes (index to "")
- m_lgrNameIdx: 4 bytes (index to "")
- Other fields: 21 bytes
- Total: **45 bytes**

**Address 2: "WOPT:B1"**
- m_category: 4 bytes
- m_statisticsType: 4 bytes
- m_vectorNameIdx: 4 bytes (index to "WOPT")
- m_nameIdx: 4 bytes (index to "B1")
- m_lgrNameIdx: 4 bytes (index to "")
- Other fields: 21 bytes
- Total: **45 bytes**

**Address 3: "WOPT:B2"**
- m_category: 4 bytes
- m_statisticsType: 4 bytes
- m_vectorNameIdx: 4 bytes (index to "WOPT" - SHARED with Address 2!)
- m_nameIdx: 4 bytes (index to "B2")
- m_lgrNameIdx: 4 bytes (index to "")
- Other fields: 21 bytes
- Total: **45 bytes**

**Total Memory After: 296 (pool) + 135 (addresses) = 431 bytes**

Wait, this seems higher! But this is only for 3 addresses. Let's see the break-even point...

### Analysis: When Does String Pool Win?

For N addresses using this pattern:
- **Before**: 125 × N bytes
- **After**: 296 + 45 × N bytes

Break-even point:
```
125N = 296 + 45N
80N = 296
N ≈ 3.7
```

**Break-even at ~4 addresses!**

For realistic scenarios:

| # Addresses | Memory Before | Memory After | Savings | Reduction % |
|-------------|---------------|--------------|---------|-------------|
| 3           | 375 B         | 431 B        | -56 B   | -15% (overhead) |
| 4           | 500 B         | 476 B        | 24 B    | 5% |
| 10          | 1,250 B       | 746 B        | 504 B   | 40% |
| 100         | 12,500 B      | 4,796 B      | 7,704 B | 62% |
| 1,000       | 125 KB        | 45.3 KB      | 79.7 KB | 64% |
| 10,000      | 1.22 MB       | 439 KB       | 781 KB  | 64% |

## Real-World Scenario: Typical Reservoir Simulation

A typical reservoir simulation might have:
- 50 unique vector names (FOPT, FOPR, WOPT, WOPR, WBHP, etc.)
- 500 wells (W1, W2, ... W500)
- 20 groups (G1, G2, ... G20)
- Each well has ~5 different vectors (WOPT, WOPR, WBHP, WWCT, WGOR)

**Total addresses**: 500 wells × 5 vectors = 2,500 well addresses
Plus field addresses, group addresses, etc. ≈ **3,000+ total addresses**

### Memory Usage:

**Before Optimization:**
- 3,000 addresses × 125 bytes = **375 KB**

**After Optimization:**
- String pool: 50 vectors + 500 wells + 20 groups = 570 unique strings
- Pool size: ~124 + (570 × 35) = ~20 KB
- Addresses: 3,000 × 45 bytes = 135 KB
- **Total: ~155 KB**

**Savings: 375 - 155 = 220 KB (59% reduction)**

## Key Benefits of String Pool

1. **Memory Deduplication**: Strings like "WOPT" appear once, not 500 times
2. **Scales Linearly**: More addresses = better savings ratio
3. **Cache Friendly**: Smaller objects = better CPU cache utilization
4. **Faster Copies**: Copying 12 bytes (3 indices) vs 96 bytes (3 strings)
5. **Faster Comparisons**: Integer comparisons vs string comparisons

## Conclusion

For the specific example of "FOPT", "WOPT:B1", "WOPT:B2":
- **Small collections (< 4 addresses)**: Slight overhead due to pool initialization
- **Typical collections (100+ addresses)**: **60-65% memory reduction**
- **Large collections (10,000+ addresses)**: **~64% memory reduction**

The string pool optimization is highly effective for real-world reservoir simulation scenarios where thousands of addresses share common vector names and entity names.
