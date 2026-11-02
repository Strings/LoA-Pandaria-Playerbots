#include "TelemetryHooks.h"

namespace
{
    TelemetryHooks::Handlers handlers;
}

void TelemetryHooks::Set(Handlers const& value)
{
    handlers = value;
}

TelemetryHooks::Handlers const& TelemetryHooks::Get()
{
    return handlers;
}
