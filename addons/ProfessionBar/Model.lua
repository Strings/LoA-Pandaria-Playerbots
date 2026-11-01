ProfessionBarModel = {}
-- Eight rank chains from the server's MoP spell_ranks data; skill IDs match ProfessionRules.
ProfessionBarModel.professions = {
    { skill=171, ranks={2259,3101,3464,11611,28596,51304,80731,105206} },
    { skill=164, ranks={2018,3100,3538,9785,29844,51300,76666,110396} },
    { skill=333, ranks={7411,7412,7413,13920,28029,51313,74258,110400} },
    { skill=202, ranks={4036,4037,4038,12656,30350,51306,82774,110403} },
    { skill=182, ranks={2366,2368,3570,11993,28695,50300,74519,110413}, action=2383 },
    { skill=773, ranks={45357,45358,45359,45360,45361,45363,86008,110417} },
    { skill=755, ranks={25229,25230,28894,28895,28897,51311,73318,110420} },
    { skill=165, ranks={2108,3104,3811,10662,32549,51302,81199,110423} },
    { skill=186, ranks={2575,2576,3564,10248,29354,50310,74517,102161}, action=2580 },
    { skill=393, ranks={8613,8617,8618,10768,32678,50305,74522,102216}, action=8613 },
    { skill=197, ranks={3908,3909,3910,12180,26790,51309,75156,110426} },
}

function ProfessionBarModel.Visible(isKnown, primary)
    local result = {}
    for _, profession in ipairs(ProfessionBarModel.professions) do
        if not primary[profession.skill] then
            for _, spell in ipairs(profession.ranks) do
                if isKnown(spell) then
                    result[#result + 1] = profession
                    break
                end
            end
        end
    end
    return result
end
