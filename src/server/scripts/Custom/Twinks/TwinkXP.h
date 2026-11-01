#ifndef ULTIMATEMOP_TWINK_XP_H
#define ULTIMATEMOP_TWINK_XP_H
class Player;
namespace Twinks
{
    bool FreeXPEnabled();
    // Reusable by the future settings panel; false means the feature is disabled.
    bool SetExperienceEnabled(Player* player, bool enabled);
}
#endif
