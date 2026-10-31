# T-032: Live server MCP

**Agent:** codex
**Branch:** codex/T-032-live-server-mcp
**Task status:** done

## What changed
- Standalone Python 3.9 standard-library stdio MCP server: server_info, gm_command, botscenario, log_tail, bot_state, bots_near.
- Environment-based SOAP credentials/ports and log paths; bounded scans; preserved raw probe lines.
- Position/state tools explicitly return historical observations, possibly including master probes, not an online census. Live decision scores depend on T-039.

## How it was tested
- Three unit/integration tests: subprocess MCP handshake/discovery/error, latest probe filtering/argument rejection, actual local HTTP SOAP transport/XML escaping/command newline rejection.
- Real server_info over SOAP port 7898 returned uptime and update-time percentiles from the copied T-001 binary on Codex databases.
- Opt-in smoke_hotfix.py also verified authenticated headless login and live Item-sparse hotfix delivery after T-045.
- No full build. No core changes.

## For the other agent
- README describes environment variables and probe logging. Tool is not installed into user/system Codex settings (outside-repo changes need approval).
- T-001 runtime gotcha: legacy.dll from the installed OpenSSL bin directory must also sit next to the exe. Without it a first world connection crashes in ARC4::ARC4 even though SOAP works. Copied it into Codex runtime only; core forces the provider search path to the exe directory. No system settings changed.
- Created CODEX GM level 3 account in auth_codex only; credentials in ignored build/runtime/codex.local.json.

## Map additions
- tools/live-server-mcp/: MCP adapter, tests and usage.

## Final merge checks (2026-10-31)
- Rebased onto the latest merged playerbot. All three MCP and two startup-profiler tests pass; Python compilation passes.
- Live MCP server_info and authenticated Shadowfang Item-sparse smoke pass on the rebuilt private test/none server.
- Gear migration and Heartseed twink_item whitelist match exactly: 314 IDs. No gear-list change was needed.
- Startup profiler parses 207 timed loader rows from the rebuilt server; loader sums remain distinct from wall time.
