MountBarModel = {}

-- Macro names are capped at 16 characters by the client.
MountBarModel.MAX_NAME = 16

function MountBarModel.MacroName(mountName)
    return string.sub(mountName or "Mount", 1, MountBarModel.MAX_NAME)
end

-- CreateMacro takes the icon's file name without the Interface\Icons\ folder.
function MountBarModel.IconName(texture)
    if not texture or texture == "" then
        return "INV_Misc_QuestionMark"
    end
    return string.match(texture, "([^\\/]+)$") or texture
end

-- Looks the mount up by spell at click time: journal indexes shift whenever a mount is learned.
function MountBarModel.MacroBody(spellID)
    return "/run if IsMounted() then Dismount() else for i=1,GetNumCompanions(\"MOUNT\") do "
        .. "if select(3,GetCompanionInfo(\"MOUNT\",i))==" .. tonumber(spellID)
        .. " then CallCompanion(\"MOUNT\",i) end end end"
end
