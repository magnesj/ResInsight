# Progress Info Usage Examples

This document provides practical examples of using the cafProgressInfo system.

## Basic Progress Tracking

### Simple Linear Progress

```cpp
#include "cafProgressInfo.h"

void processItems(const std::vector<Item>& items)
{
    caf::ProgressInfo progress(items.size(), "Processing Items");
    
    for (size_t i = 0; i < items.size(); i++)
    {
        progress.setProgressDescription(QString("Processing item %1").arg(i));
        
        // Do work with item
        processItem(items[i]);
        
        progress.incrementProgress();
    }
}
```

### Using ProgressScope (Recommended)

```cpp
#include "cafProgressInfo.h"

void processItems(const std::vector<Item>& items)
{
    caf::ProgressScope progress(items.size(), "Processing Items");
    
    for (size_t i = 0; i < items.size(); i++)
    {
        if (progress.isCancelled())
        {
            // Handle cancellation
            return;
        }
        
        progress.setDescription(QString("Processing item %1").arg(i));
        processItem(items[i]);
        progress.increment();
    }
}
```

### Percentage-Based Progress

```cpp
#include "cafProgressInfo.h"

void loadLargeFile(const QString& filename)
{
    caf::ProgressPercentage progress("Loading File");
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;
    
    qint64 totalSize = file.size();
    qint64 bytesRead = 0;
    
    while (!file.atEnd())
    {
        QByteArray chunk = file.read(CHUNK_SIZE);
        bytesRead += chunk.size();
        
        double percentage = (100.0 * bytesRead) / totalSize;
        progress.setPercentage(percentage);
        
        // Process chunk
        processChunk(chunk);
    }
}
```

## Nested Progress

### Multi-Stage Operations

```cpp
void importData(const QString& path)
{
    caf::ProgressInfo mainProgress(3, "Importing Data");
    
    // Stage 1: Scan files
    {
        caf::ProgressInfo scanProgress(100, "Scanning Files");
        // Scan work...
        for (int i = 0; i < 100; i++)
        {
            scanProgress.incrementProgress();
        }
    }
    mainProgress.incrementProgress();
    
    // Stage 2: Parse data
    {
        caf::ProgressInfo parseProgress(200, "Parsing Data");
        // Parse work...
        for (int i = 0; i < 200; i++)
        {
            parseProgress.incrementProgress();
        }
    }
    mainProgress.incrementProgress();
    
    // Stage 3: Build structures
    {
        caf::ProgressInfo buildProgress(50, "Building Structures");
        // Build work...
        for (int i = 0; i < 50; i++)
        {
            buildProgress.incrementProgress();
        }
    }
    mainProgress.incrementProgress();
}
```

### Using Task-Based Progress

```cpp
void processWorkflow()
{
    caf::ProgressInfo progress(5, "Processing Workflow");
    
    {
        auto task = progress.task("Initialize");
        initialize();
        // Task auto-increments on scope exit
    }
    
    {
        auto task = progress.task("Load Data", 2);
        loadData();
        // Task auto-increments by 2 on scope exit
    }
    
    {
        auto task = progress.task("Process");
        process();
    }
    
    {
        auto task = progress.task("Save Results");
        saveResults();
    }
}
```

## Advanced Usage

### Variable Step Sizes

```cpp
void processComplexWorkflow()
{
    caf::ProgressInfo progress(10, "Complex Workflow");
    
    // Quick task - 1 unit
    quickOperation();
    progress.incrementProgress();
    
    // Heavy task - 5 units
    progress.setNextProgressIncrement(5);
    heavyOperation();
    progress.incrementProgress();
    
    // Medium tasks - 2 units each
    progress.setNextProgressIncrement(2);
    mediumOperation1();
    progress.incrementProgress();
    
    progress.setNextProgressIncrement(2);
    mediumOperation2();
    progress.incrementProgress();
}
```

### With Cancellation Support

```cpp
bool processWithCancellation(const std::vector<Item>& items)
{
    caf::ProgressInfo progress(items.size(), "Processing", true, true);
    // delayShowingProgress=true, allowCancel=true
    
    for (size_t i = 0; i < items.size(); i++)
    {
        if (progress.isCancelled())
        {
            // Clean up partial work
            cleanupPartialWork();
            return false;
        }
        
        progress.setProgressDescription(QString("Item %1/%2").arg(i+1).arg(items.size()));
        
        if (!processItem(items[i]))
        {
            // Handle processing error
            return false;
        }
        
        progress.incrementProgress();
    }
    
    return true;
}
```

### Blocking Progress Display

```cpp
void quickOperation()
{
    // Don't show progress for very quick operations
    caf::ProgressInfoBlocker blocker;
    
    // This won't show a progress dialog
    performQuickTask();
}
```

### Without Event Processing

```cpp
void processInBackground()
{
    // Prevent event processing during progress updates
    caf::ProgressInfoEventProcessingBlocker blocker;
    
    caf::ProgressInfo progress(100, "Background Processing");
    
    for (int i = 0; i < 100; i++)
    {
        // Progress updates won't call processEvents()
        progress.incrementProgress();
        doWork();
    }
}
```

## Common Patterns

### File Reading

```cpp
void readEclipseFile(const QString& filename)
{
    QFile file(filename);
    qint64 fileSize = file.size();
    
    caf::ProgressPercentage progress("Reading Eclipse File");
    
    qint64 bytesProcessed = 0;
    const qint64 updateInterval = fileSize / 100; // Update every 1%
    qint64 nextUpdate = updateInterval;
    
    while (!file.atEnd())
    {
        QByteArray data = file.read(BUFFER_SIZE);
        bytesProcessed += data.size();
        
        if (bytesProcessed >= nextUpdate)
        {
            double percentage = (100.0 * bytesProcessed) / fileSize;
            progress.setPercentage(percentage);
            progress.setDescription(QString("%1 MB / %2 MB")
                .arg(bytesProcessed / 1024 / 1024)
                .arg(fileSize / 1024 / 1024));
            nextUpdate += updateInterval;
        }
        
        processData(data);
    }
}
```

### Grid Processing

```cpp
void processGrid(const Grid& grid)
{
    size_t totalCells = grid.cellCount();
    caf::ProgressScope progress(totalCells, "Processing Grid");
    
    for (size_t i = 0; i < grid.cellCountI(); i++)
    {
        for (size_t j = 0; j < grid.cellCountJ(); j++)
        {
            for (size_t k = 0; k < grid.cellCountK(); k++)
            {
                if (progress.isCancelled())
                    return;
                
                processCell(grid.cell(i, j, k));
                progress.increment();
                
                // Update description occasionally (not every cell)
                if ((i * j * k) % 1000 == 0)
                {
                    progress.setDescription(QString("Cell [%1, %2, %3]").arg(i).arg(j).arg(k));
                }
            }
        }
    }
}
```

### Batch Processing

```cpp
void processBatch(const QStringList& files)
{
    caf::ProgressScope mainProgress(files.size(), "Processing Files", true, true);
    
    for (int fileIdx = 0; fileIdx < files.size(); fileIdx++)
    {
        if (mainProgress.isCancelled())
            break;
        
        const QString& filename = files[fileIdx];
        mainProgress.setDescription(QString("File %1/%2: %3")
            .arg(fileIdx + 1)
            .arg(files.size())
            .arg(QFileInfo(filename).fileName()));
        
        // Process individual file with nested progress
        {
            caf::ProgressInfo fileProgress(100, "");
            processFile(filename, fileProgress);
        }
        
        mainProgress.increment();
    }
}
```

## Best Practices

1. **Use ProgressScope for simple loops**: Simplest API for common cases
2. **Use ProgressPercentage for file I/O**: Natural percentage-based progress
3. **Enable cancellation for long operations**: Always allow user to cancel
4. **Delay showing for quick operations**: Use `delayShowingProgress=true`
5. **Update description regularly**: Helps user understand current operation
6. **Check isCancelled() in loops**: Handle cancellation gracefully
7. **Use appropriate granularity**: Don't update too frequently or too rarely
8. **Clean up on cancellation**: Always clean up partial work
9. **Use task() for RAII pattern**: Automatic progress management
10. **Nest progress appropriately**: Match nesting to logical operation hierarchy

## Migration from Old Code

### Before
```cpp
caf::ProgressInfo progress(100, "Processing");
for (int i = 0; i < 100; i++) {
    progress.setProgress(i);
    doWork(i);
}
```

### After (Recommended)
```cpp
caf::ProgressScope progress(100, "Processing");
for (int i = 0; i < 100; i++) {
    if (progress.isCancelled()) break;
    doWork(i);
    progress.increment();
}
```
