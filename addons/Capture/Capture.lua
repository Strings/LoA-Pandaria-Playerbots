-- Capture: /idea and /bug slash commands for UltimateMoP (T-040).
-- Takes a client screenshot via Screenshot() and forwards the note plus the
-- expected best-effort screenshot filename (WoWScrnShot_MMDDYY_HHMMSS) to the
-- server's .idea / .bug GM command.

local ADDON_PREFIX = "|cff55ccff[Capture]|r "

local function trim(s)
    if not s then return "" end
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function expectedScreenshotName()
    -- WoW 5.4.8 names screenshots WoWScrnShot_MMDDYY_HHMMSS.<ext> in Screenshots/
    -- using local system time at the moment Screenshot() is invoked.
    return "WoWScrnShot_" .. date("%m%d%y_%H%M%S")
end

local function sendCapture(kind, rawMsg)
    local text = trim(rawMsg)
    if text == "" then
        if DEFAULT_CHAT_FRAME then
            DEFAULT_CHAT_FRAME:AddMessage(ADDON_PREFIX .. "Usage: /" .. kind .. " <description>")
        end
        return
    end

    local scrnName = expectedScreenshotName()
    if type(Screenshot) == "function" then
        Screenshot()
    end

    local cmd = string.format(".%s [screenshot=%s] %s", kind, scrnName, text)
    SendChatMessage(cmd, "SAY")
end

SLASH_CAPTUREIDEA1 = "/idea"
SlashCmdList["CAPTUREIDEA"] = function(msg)
    sendCapture("idea", msg)
end

SLASH_CAPTUREBUG1 = "/bug"
SlashCmdList["CAPTUREBUG"] = function(msg)
    sendCapture("bug", msg)
end

SLASH_CAPTUREINSPECT1 = "/capture"
SlashCmdList["CAPTUREINSPECT"] = function(msg)
    local args = trim(msg)
    if args == "" then
        SendChatMessage(".capture list", "SAY")
    else
        SendChatMessage(".capture " .. args, "SAY")
    end
end
