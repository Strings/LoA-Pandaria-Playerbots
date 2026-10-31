-- An opt-in GM-only smoke probe; ordinary player chat is untouched.
local function OnChat(event, player, message, chatType, language)
    if message == "prototype-check" and player:GetGMRank() > 0 then
        player:SendBroadcastMessage("UltimateMoP prototype hooks ready.")
        return false
    end
end

RegisterPlayerEvent(18, OnChat)
