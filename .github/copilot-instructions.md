# GitHub Copilot Instructions for ResInsight

> **Note**: For detailed build instructions, architecture overview, and development guidelines, see [CLAUDE.md](../CLAUDE.md) in the repository root.

ResInsight is a cross-platform 3D visualization and post-processing tool for reservoir simulation data, built with C++23, Qt6, and Python integration.

## Quick Reference

### Build & Test
```bash
# See CLAUDE.md for full build instructions
cmake . --preset=linux-base && ninja -C build  # Linux
cmake . --preset=x64-relwithdebinfo            # Windows
ctest -R "UnitTests"                           # Run tests
```

### Code Style
- **C++**: Use `clang-format-19` (config in `.clang-format`)
- **Python**: Use `ruff format` and `ruff check --fix` before committing
- Follow existing patterns in the file you're editing

### Git Conventions
- Commit format: `"#<issue_number> <Component>: <Description>"` (e.g., "#12773 Python: Add API for creating valve templates")
- **Always** format code before committing
- Main development on `dev` branch, `master` for releases

## Key Concepts for Code Suggestions

### PDM (Project Data Model) Framework
This is ResInsight's core UI/serialization framework. When suggesting code:
- Objects inherit from `caf::PdmObject`
- For Python API: Use `CAF_PDM_InitScriptableObject` and `CAF_PDM_InitScriptableField`
- Field naming: camelCase (e.g., "StartMd") → auto-converts to snake_case in Python (start_md)
- See CLAUDE.md section "Making PDM Objects Scriptable" for complete examples

### Directory Structure Context
- `ApplicationLibCode/`: Domain logic (reservoir simulation, completions, wells)
- `Fwk/AppFwk/`: PDM framework, commands, UI components
- `Fwk/VizFwk/`: 3D rendering (Qt/OpenGL)
- `GrpcInterface/Python/`: Python API (auto-generated from PDM objects)

### When Suggesting C++ Code
- C++23 standard: can use `std::expected`, `std::stacktrace`, ranges
- Qt6 APIs preferred (not Qt5 patterns)
- Use Qt resource system (`.qrc`) for UI resources
- Check existing patterns in similar files for consistency

### When Suggesting Python Code
- Python 3.8+ compatible
- Python classes auto-generated from PDM objects after build
- Test location: `GrpcInterface/Python/rips/tests/`
- Always suggest running `ruff` before commit

## Common Pitfall Reminders

- Don't mix Qt5 and Qt6 patterns
- PDM scriptable field names affect Python API - use camelCase
- Python tests require ResInsight executable built with `RESINSIGHT_ENABLE_GRPC=ON`
- vcpkg manages dependencies - check `vcpkg.json` before suggesting manual installs
