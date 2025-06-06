/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AddonPackets.h"
#include "ByteBuffer.h"
#include "PacketOperators.h"

namespace WorldPackets::Addon
{
ByteBuffer& operator>>(ByteBuffer& data, AddOnInfo& addonInfo)
{
    data.ResetBitPos();

    data >> SizedCString::BitsSize<10>(addonInfo.Name);
    data >> SizedCString::BitsSize<10>(addonInfo.Version);
    data >> Bits<1>(addonInfo.Loaded);
    data >> Bits<1>(addonInfo.Disabled);

    data >> SizedCString::Data(addonInfo.Name);
    data >> SizedCString::Data(addonInfo.Version);

    return data;
}
}
