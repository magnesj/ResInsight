# ResInsight Processing Server - rips

A Python interface for the ResInsight visualization and post-processing suite for Reservoir Simulations

[Learn More](https://www.resinsight.org)

## MCP server

Install the optional MCP dependency and start the stdio server:

```bash
pip install "rips[mcp]"
rips-mcp
```

By default, the server attaches to ResInsight on `localhost:50051`. Set
`RESINSIGHT_PORT` to use another port, or set `RESINSIGHT_EXECUTABLE` and call
the server's `connect` tool with `launch=true` to launch ResInsight. The MCP
server is single-instance. Use `--read-only` to disable case imports and
exports. Property statistics are streamed and raw grid arrays are never
returned; summary vectors and well lists are paged.

Property and snapshot exports are restricted to the configured export
directory. Snapshot exports use a fresh subdirectory. Set
`RESINSIGHT_EXPORT_DIR` or pass `--export-directory` to choose it.

Example client configuration for Claude Desktop and VS Code clients that support
the MCP stdio configuration format:

```json
{
  "mcpServers": {
    "resinsight": {
      "command": "rips-mcp",
      "args": ["--read-only"],
      "env": {
        "RESINSIGHT_PORT": "50051"
      }
    }
  }
}
```

Copilot CLI can launch the server with:

```bash
copilot mcp add resinsight -- rips-mcp --read-only
```

After connecting, tools include `list_cases`, `case_info`,
`list_grid_properties`, and `get_grid_property_statistics`. Export tools write
to the allow-listed export directory and return paths. Generic PDM tools use
session-local object IDs returned by `list_pdm_objects`; field updates and
method calls are omitted in read-only mode. Use `resinsight://project` and
`resinsight://case/{case_id}` to inspect project resources.
