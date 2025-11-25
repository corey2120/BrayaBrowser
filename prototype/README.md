# Gecko Embedding Prototypes

This directory contains experimental prototypes for Gecko (Firefox) embedding.

## Goal
Create minimal examples to prove libxul embedding works before migrating the full browser.

## Prototypes

### gecko-hello.cpp (Week 1-2)
Minimal GTK4 + libxul integration:
- Creates window
- Embeds Gecko widget
- Loads a URL
- Basic navigation

### gecko-tabs.cpp (Week 3)
Multi-tab test:
- Creates multiple browser instances
- Tab switching
- Memory management

### gecko-full.cpp (Week 4+)
Feature-complete test:
- All browser features
- Performance testing
- Integration verification

## Building

```bash
# Will be added once we figure out linker flags
```

## Resources

- libxul.so location: /usr/lib64/firefox/libxul.so
- Firefox version: 145.0.1
- Platform: Fedora 43

