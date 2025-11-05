#pragma once

#include "../../Common/Interfaces/ModuleInterface.h"
#include "../../Common/Helpers/ModuleHook.hpp"
#include <ATF/_trans_gm_msg_inform_zocl.hpp>
#include <ATF/CMonsterInfo.hpp>
#include <vector>
#include <string>

namespace GameServer
{
    namespace Addon
    {
        class CBossLog
            : public Yorozuya::Module::IModule
            , CModuleHook
        {
        public:
            CBossLog();

            virtual void load() override;

            virtual void unload() override;

            virtual Yorozuya::Module::ModuleName_t get_name() override;

            virtual void configure(const rapidjson::Value& nodeConfig) override;

        private:
            static bool m_bActivated;
            static bool m_bLogBirth;
            static bool m_bLogDeath;
            static bool m_bLogKillerName;
            static ::std::vector<::std::string> m_arrMonsterNames;

            static void WINAPIV SendMsg_Create(
                ATF::CMonster* pMonster,
                ATF::Info::CMonsterSendMsg_Create210_ptr next);

            static bool WINAPIV Destroy(
                ATF::CMonster* pMonster,
                char byDestroyCode,
                ATF::CGameObject* pAttObj,
                ATF::Info::CMonsterDestroy46_ptr next);
        };
    };
};
