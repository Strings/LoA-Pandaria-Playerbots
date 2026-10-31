# Live server MCP

Standalone Python 3.9+ stdio MCP server over Trinity's `urn:TC` SOAP endpoint and
existing `[PROBE]` logs. No package installation or C++ build required.

Start with `python tools/live-server-mcp/server.py`. The parent MCP client writes
newline-delimited JSON-RPC to stdin; stdout contains only protocol replies.
Supports protocol 2024-11-05 (negotiates that version), initialize, ping,
tools/list and tools/call. Reference: [MCP stdio](https://modelcontextprotocol.io/specification/2024-11-05/basic/transports).

Environment:

| Variable | Meaning/default |
|---|---|
| MOP_SOAP_HOST | 127.0.0.1 |
| MOP_SOAP_PORT | 7898 (Codex); set explicitly for another server |
| MOP_SOAP_USER / MOP_SOAP_PASSWORD | Required game GM credentials; never committed |
| MOP_SOAP_TIMEOUT | Request timeout, seconds; 30 |
| MOP_SERVER_LOG | Required absolute path for log tools, usually playerbots debug log |
| MOP_LOG_SCAN_BYTES | Bounded log tail, 1048576; maximum 16777216 |

Tools: `server_info`, `gm_command(command)`, `botscenario(bot, scenario)`,
`log_tail(lines=100)`, `bot_state(bot)`, `bots_near(map,x,y,z,radius)`.
Example scenario argument: `wander secs=10`; check the core's botscenario help
for supported verbs. Commands execute immediately with the account's privileges.
Use the agent's own server/DBs. Basic authentication is intended for localhost;
use an encrypted tunnel if accessing a remote SOAP endpoint.

`bot_state` and `bots_near` return **historical observations**, never an online
census. Enable `.botprobe Name on` and playerbots DEBUG logging in the server
profile, point MOP_SERVER_LOG at that logger's output, and `.botprobe Name off`
when finished. Probe rows can include masters, so results may include players.
Latest matching row in the bounded window wins, with raw line and fields retained
for timestamps and decision context. Missing observations do not mean offline.
Live decision-score explanations wait for T-039's core command.

Run tests: `python -m unittest discover -s tools/live-server-mcp -v`.

Opt-in live regression after applying T-045: set MOP_ACCOUNT/MOP_PASSWORD,
MOP_HOST, MOP_AUTH_PORT and MOP_WORLD_PORT for a dedicated test account, then run
`python tools/live-server-mcp/smoke_hotfix.py`. It creates Codextwink if the account
has no characters, logs in and asserts the hotfix notification and Shadowfang's
item level/crit fields from the actual Item-sparse reply. MOP_HEADLESS_CLIENT can
override the path to the repository's headless.py. No GUI is used.

Windows runtime gotcha: with OpenSSL 3, copy the installed matching `legacy.dll`
next to worldserver.exe as well as libcrypto/libssl. OpenSSLCrypto::threadsSetup
sets the provider search directory to the executable folder. Without legacy.dll,
the server starts and SOAP works but the first world connection can abort in
ARC4::ARC4. This is a runtime DLL requirement, not an MCP setting. See
[OpenSSL legacy provider](https://docs.openssl.org/3.2/man7/OSSL_PROVIDER-legacy/).
