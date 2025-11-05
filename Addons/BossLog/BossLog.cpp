#include "stdafx.h"

#include "BossLog.h"
#include "../../Common/ETypes.h"
#include "../../Common/Helpers/RapidHelper.hpp"
#include "../../Common/Helpers/BroadcastHelper.hpp"

#include <ATF/global.hpp>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <string>
#include <fstream>
#include <vector>

namespace GameServer
{
    namespace Addon
    {
        bool CBossLog::m_bActivated = false;
        bool CBossLog::m_bLogBirth = true;
        bool CBossLog::m_bLogDeath = true;
        bool CBossLog::m_bLogKillerName = true;
        ::std::vector<::std::string> CBossLog::m_arrMonsterNames;

        CBossLog::CBossLog()
        {
        }

        void CBossLog::load()
        {
            enable_hook(&ATF::CMonster::SendMsg_Create, &CBossLog::SendMsg_Create);
            enable_hook(&ATF::CMonster::Destroy, &CBossLog::Destroy);
        }

        void CBossLog::unload()
        {
            cleanup_all_hook();
        }

        Yorozuya::Module::ModuleName_t CBossLog::get_name()
        {
            static const Yorozuya::Module::ModuleName_t name = "addon.boss_log";
            return name;
        }

        void CBossLog::configure(const rapidjson::Value & nodeConfig)
        {
            m_bActivated = RapidHelper::GetValueOrDefault(nodeConfig, "activated", false);
            m_bLogBirth = RapidHelper::GetValueOrDefault(nodeConfig, "log_birth", true);
            m_bLogDeath = RapidHelper::GetValueOrDefault(nodeConfig, "log_death", true);
            m_bLogKillerName = RapidHelper::GetValueOrDefault(nodeConfig, "log_killer_name", true);
            ::std::string monsterJsNamesFilePath = RapidHelper::GetValueOrDefault<::std::string>(nodeConfig, "monsters_json_file_path", "");
            if (!monsterJsNamesFilePath.empty())
            {
                ::std::ifstream ifs(monsterJsNamesFilePath);
                rapidjson::IStreamWrapper isw(ifs);
                rapidjson::Document document;
                document.ParseStream(isw);
                if (document.IsArray())
                {
                    for (const auto& monster : document.GetArray())
                    {
                        if (monster.IsString())
                        {
                            m_arrMonsterNames.push_back(monster.GetString());
                        }
                    }
                }
            }
        }

        void WINAPIV CBossLog::SendMsg_Create(
            ATF::CMonster* pMonster,
            ATF::Info::CMonsterSendMsg_Create210_ptr next)
        {
            next(pMonster);

            if (!m_bActivated)
                return;

            if (!m_bLogBirth)
                return;

            if (!pMonster->IsBossMonster())
                return;

            if (!pMonster->m_pMonRec)
                return;

            if (!pMonster->m_pCurMap || !pMonster->m_pCurMap->m_pMapSet)
                return;

            ::std::string sMonsterName = "Desconhecido";
            if (pMonster->m_pMonRec->m_dwIndex < m_arrMonsterNames.size())
            {
                sMonsterName = m_arrMonsterNames[pMonster->m_pMonRec->m_dwIndex];
                if (sMonsterName.empty())
                {
                    sMonsterName = "Desconhecido";
                }
            }

            ::std::string sMessage = "Boss ";
            sMessage += sMonsterName;
            sMessage += " nasceu em ";
            sMessage += pMonster->m_pCurMap->m_pMapSet->m_strFileName;
            sMessage += "!";

            ATF::_trans_gm_msg_inform_zocl packet;
            packet.wMsgSize = (unsigned short)sMessage.length();
            memcpy(packet.wszChatData, sMessage.c_str(), packet.wMsgSize);
            packet.wszChatData[packet.wMsgSize] = '\0';

            char byType[2]{ 2, 14 };
            Helpers::CBroadcastHelper::BroadcastToAllOnline(
                packet, byType, packet.size());
        }

        bool WINAPIV CBossLog::Destroy(
            ATF::CMonster* pMonster,
            char byDestroyCode,
            ATF::CGameObject* pAttObj,
            ATF::Info::CMonsterDestroy46_ptr next)
        {
            bool bResult = next(pMonster, byDestroyCode, pAttObj);

            if (!m_bActivated)
                return bResult;

            if (!m_bLogDeath)
                return bResult;

            if (!pMonster->IsBossMonster())
                return bResult;

            if (!pAttObj)
                return bResult;

            if (pAttObj->m_ObjID.m_byID != (BYTE)e_obj_id::obj_id_player)
                return bResult;

            if (!pMonster->m_pMonRec)
                return bResult;

            if (!pMonster->m_pCurMap || !pMonster->m_pCurMap->m_pMapSet)
                return bResult;

            ATF::CPlayer* pPlayer = (ATF::CPlayer*)pAttObj;
            if (!pPlayer->m_bLive)
                return bResult;

            char* pPlayerName = pPlayer->m_Param.GetCharNameA();
            if (!pPlayerName)
                return bResult;

            ::std::string sMonsterName = "Desconhecido";
            if (pMonster->m_pMonRec->m_dwIndex < m_arrMonsterNames.size())
            {
                sMonsterName = m_arrMonsterNames[pMonster->m_pMonRec->m_dwIndex];
                if (sMonsterName.empty())
                {
                    sMonsterName = "Desconhecido";
                }
            }

            ::std::string sMessage = "Boss ";
            sMessage += sMonsterName;
            sMessage += " foi derrotado";

            if (m_bLogKillerName) {
                sMessage += " por ";
                sMessage += pPlayerName;
            }

            sMessage += " em ";
            sMessage += pMonster->m_pCurMap->m_pMapSet->m_strFileName;
            sMessage += "!";

            ATF::_trans_gm_msg_inform_zocl packet;
            packet.wMsgSize = (unsigned short)sMessage.length();
            memcpy(packet.wszChatData, sMessage.c_str(), packet.wMsgSize);
            packet.wszChatData[packet.wMsgSize] = '\0';

            char byType[2]{ 2, 14 };
            Helpers::CBroadcastHelper::BroadcastToAllOnline(
                packet, byType, packet.size(),
                Helpers::e_broadcast_type::both,
                sMessage);

            return bResult;
        }
    }
}
