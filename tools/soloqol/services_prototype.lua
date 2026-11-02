-- GM-only Lua prototype. Load explicitly in an owned Eluna runtime; not autoloaded in production.
local entry = 904000
local services = {
    {"Rename my character", 1},
    {"Customize my appearance", 8},
    {"Change my race", 128},
    {"Change my faction", 64},
}
local function Hello(event, player, creature)
    if player:GetGMRank() < 2 then return false end
    player:GossipClearMenu()
    for index, service in ipairs(services) do
        player:GossipMenuAddItem(0, service[1], 0, index)
    end
    player:GossipSendMenu(94000, creature)
    return true
end
local function Select(event, player, creature, sender, action)
    if player:GetGMRank() < 2 or sender ~= 0 or not services[action] then return false end
    player:SetAtLoginFlag(services[action][2])
    player:SaveToDB()
    player:GossipComplete()
    player:SendBroadcastMessage("Your free service is ready. Log out to use it on character selection.")
    return true
end
RegisterCreatureGossipEvent(entry, 1, Hello)
RegisterCreatureGossipEvent(entry, 2, Select)
