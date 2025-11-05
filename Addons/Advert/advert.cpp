#include "stdafx.h"

#include "advert.h"
#include "../../Common/ETypes.h"
#include "../../Common/Helpers/RapidHelper.hpp"
#include "../../Common/Helpers/BroadcastHelper.hpp"

#include <ATF/global.hpp>

namespace GameServer
{
    namespace Addon
    {
        void CAdvert::load()
        {
        }

        void CAdvert::unload()
        {
            cleanup_all_hook();
        }

        Yorozuya::Module::ModuleName_t CAdvert::get_name()
        {
            static const Yorozuya::Module::ModuleName_t name = "addon.advert";
            return name;
        }

        void CAdvert::loop()
        {
            if (!m_bActivated)
                return;

            for (auto& i : m_vecRecords)
            {
                if (!i.timer.is_end())
                    continue;

                i.timer.begin(i.sDelay);

                char byType[2]{ 2, 14 };
                
                // Loop único otimizado: faz broadcast e chat trans no mesmo loop
                // Evita iterar duas vezes sobre os mesmos players
                for (uint16_t idx = 0; idx < ATF::Global::max_player; ++idx)
                {
                    auto& player = ATF::Global::g_Player[idx];
                    
                    // Early exit otimizado
                    if (!player.m_bOper || !player.m_bLive)
                        continue;

                    // Filtro premium
                    if (i.bHideForPremium && player.IsApplyPcbangPrimium())
                        continue;

                    // Broadcast do packet
                    ATF::Global::g_NetProcess[(uint8_t)e_type_line::client]
                        ->LoadSendMsg(idx, byType, (char *)&i.packet, i.packet.size());

                    // Chat trans
                    player.SendData_ChatTrans(
                        0,
                        -1,
                        player.GetObjRace(),
                        false,
                        (char *)i.sMsg.c_str(),
                        player.m_Param.m_byPvPGrade,
                        nullptr);
                }
            }
        }

        void CAdvert::configure(const rapidjson::Value & nodeConfig)
        {
            m_bActivated = RapidHelper::GetValueOrDefault(nodeConfig, "activated", false);
            if (!m_bActivated)
                return;

            for (auto& rec : nodeConfig["records"].GetArray())
            {
                advert_fld item(
                    RapidHelper::GetValueOrDefault(rec, "hide_for_premium", true),
                    RapidHelper::GetValue<::std::string>(rec, "message"),
                    ::std::chrono::seconds(RapidHelper::GetValue<uint64_t>(rec, "delay"))
                );
                m_vecRecords.emplace_back(item);
            }
        }
    }
}
