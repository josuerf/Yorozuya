#pragma once

#include "../../Common/Interfaces/ModuleInterface.h"
#include "../../Common/Helpers/ModuleHook.hpp"
#include <ATF/CPlayerInfo.hpp>
#include <ATF/CItemBoxInfo.hpp>
#include <vector>
#include <string>
#include <unordered_set>

namespace GameServer
{
    namespace Addon
    {
        class CAutoLoot
            : public Yorozuya::Module::IModule
            , CModuleHook
        {
        public:
            CAutoLoot();

            virtual void load() override;

            virtual void unload() override;

            virtual Yorozuya::Module::ModuleName_t get_name() override;

            virtual void configure(const rapidjson::Value& nodeConfig) override;

        private:
            static bool m_bActivated;
            static bool m_bPremiumOnly;
            
            // Filter configurations
            struct FilterConfig
            {
                int mode; // 0 = except, 1 = only
                std::vector<std::string> values;
            };
            
            static FilterConfig m_MonsterFilter;
            static FilterConfig m_GradeFilter;
            static FilterConfig m_ItemFilter;
            
            // Helper methods
            static bool ShouldAutoLoot(ATF::CMonster* pMonster, ATF::CPlayer* pPlayer);
            static bool CheckMonsterFilter(ATF::CMonster* pMonster);
            static bool CheckGradeFilter(ATF::CMonster* pMonster);
            static bool CheckItemFilter(ATF::_STORAGE_LIST::_db_con* pItem);
            static std::string GetItemCodeString(ATF::_STORAGE_LIST::_db_con* pItem);
            
            // Hook methods
            static bool WINAPIV Destroy(
                ATF::CMonster* pMonster,
                char byDestroyCode,
                ATF::CGameObject* pAttObj,
                ATF::Info::CMonsterDestroy46_ptr next);
                
            static void WINAPIV SendMsg_Create(
                ATF::CItemBox* pBox,
                ATF::Info::CItemBoxSendMsg_Create14_ptr next);
                
            static void WINAPIV pc_TakeGroundingItem(
                ATF::CPlayer* pObj,
                ATF::CItemBox* pBox,
                uint16_t wAddSerial,
                ATF::Info::CPlayerpc_TakeGroundingItem1947_ptr next);
                
            static bool WINAPIV IsTakeRight(
                ATF::CItemBox* pBox,
                ATF::CPlayer* pPlayer,
                ATF::Info::CItemBoxIsTakeRight10_ptr next);
        };
    };
};
