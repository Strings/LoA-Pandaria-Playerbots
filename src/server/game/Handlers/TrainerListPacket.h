#ifndef TRINITY_TRAINER_LIST_PACKET_H
#define TRINITY_TRAINER_LIST_PACKET_H

#include <string>

class Creature;
class WorldSession;
struct TrainerSpellData;

// Kept out of WorldSession.h so scripts can send a filtered trainer list without touching a widely included header.
namespace TrainerListPacket
{
    // Sends SMSG_TRAINER_LIST for `unit` with `spells`. Buying still validates against the unit's own list.
    void Send(WorldSession* session, Creature* unit, std::string const& title, TrainerSpellData const& spells);
}

#endif
