# Feature #2: Downloads Management

## Goal
Track and display file downloads with progress indicators

## Implementation Plan

1. **Create BrayaDownloads class**
   - Track active downloads
   - Store download history
   - Show progress (bytes downloaded, total size, speed)

2. **UI Components**
   - Downloads panel (slide-out or dialog)
   - Download progress bar for each item
   - Pause/Cancel buttons
   - Open file/folder buttons
   - Clear completed downloads

3. **Integration**
   - Connect to WebKit download signals
   - Show notification when download starts
   - Update progress in real-time
   - Keyboard shortcut (Ctrl+J)

4. **Storage**
   - Save downloads to ~/Downloads (configurable)
   - Track download history
   - Store metadata (URL, filename, size, time)

## Files to create:
- src/BrayaDownloads.h
- src/BrayaDownloads.cpp

## Files to modify:
- src/BrayaWindow.h/cpp
- CMakeLists.txt

Let's build it!
