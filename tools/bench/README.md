# Startup profiling

Python 3.9+, standard library only. Rank the server's own loader messages:

```
python tools/bench/startup_profile.py /path/to/Server.log --top 20
python tools/bench/startup_profile.py /path/to/Server.log --json
```

Supports milliseconds and seconds, including decimal durations. A `Using
configuration file` line starts a run; `World initialized` ends its loading
window. `--run 0` selects the first startup; the default is the last startup
containing timings. Partial logs work, but cannot establish a complete startup.
Source line numbers permit checking each result. The tool reads logs only.

Totals are diagnostic, not measured startup time: loader messages may nest,
overlap, omit work, or include asynchronous work. Do not turn the sum into a
wall-clock percentage. Logs without timing rows and invalid run indices fail
with an explanation. No hardcoded server paths or credentials.

Tests: `python -m unittest discover -s tools/bench -v`.
