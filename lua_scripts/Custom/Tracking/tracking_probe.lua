-- T-047 is a client experiment. Native aura stacking and tracking fields stay untouched.
local Probe = {}
local spells = {1494, 19883, 2383, 2580}
local profiles = {creatures = {1494, 19883}, resources = {2383, 2580}, all = spells}
-- 5.4.8.18414 UpdateFields.h: UNIT_END=0xA0, private tracking fields +0x5A3/+0x5A4.
local creatureField, resourceField = 0x643, 0x644

function Probe.Enabled()
    local file = io.open(os.getenv("MOP_WORLD_CONFIG") or "worldserver.conf", "r")
    if not file then return false end
    local enabled = false
    for line in file:lines() do
        local value = line:match("^%s*Tracking%.Probe%.Enable%s*=%s*([^%s#]+)")
        if value then
            value = value:lower()
            enabled = value == "1" or value == "true" or value == "yes" or value == "on"
        end
    end
    file:close()
    return enabled
end

local function status(player)
    local active = {}
    for _, id in ipairs(spells) do
        if player:HasAura(id) then active[#active + 1] = tostring(id) end
    end
    player:SendBroadcastMessage(string.format("TrackingProbe enabled=%d creatures=%d resources=%d active=%s",
        Probe.Enabled() and 1 or 0, player:GetUInt32Value(creatureField),
        player:GetUInt32Value(resourceField), table.concat(active, ",")))
end

function Probe.Command(_, player, command)
    command = command:gsub("^%.", "")
    if command:match("^(%S+)") ~= "trackingprobe" then return end
    if not player or player:GetGMRank() < 2 then
        if player then player:SendBroadcastMessage("Tracking probe requires a GM.") end
        return false
    end
    local action = command == "trackingprobe" and "status" or
        command:match("^trackingprobe%s+(%a+)%s*$")
    if action ~= "status" and action ~= "clear" and not profiles[action] then
        player:SendBroadcastMessage("Use .trackingprobe status|resources|creatures|all|clear on a test fixture.")
        return false
    end
    if profiles[action] and not Probe.Enabled() then
        player:SendBroadcastMessage("Probe forcing is disabled. Set Tracking.Probe.Enable=1 in the active world config.")
        return false
    end
    if action ~= "status" then
        for _, id in ipairs(spells) do player:RemoveAura(id) end
        for _, id in ipairs(profiles[action] or {}) do
            if not player:AddAura(id, player) then
                player:SendBroadcastMessage("Tracking probe could not apply spell " .. id)
            end
        end
    end
    status(player)
    return false
end

RegisterPlayerEvent(42, Probe.Command)
return Probe
