#include "stdafx.h"

#include "AutoLoot.h"
#include "../../Common/ETypes.h"
#include "../../Common/Helpers/RapidHelper.hpp"

#include <ATF/global.hpp>
#include <ATF/_itembox_create_zocl.hpp>
#include <rapidjson/document.h>
#include <sstream>
#include <algorithm>

namespace GameServer
{
    namespace Addon
    {
        bool CAutoLoot::m_bActivated = false;
        bool CAutoLoot::m_bPremiumOnly = false;
        CAutoLoot::FilterConfig CAutoLoot::m_MonsterFilter;
        CAutoLoot::FilterConfig CAutoLoot::m_GradeFilter;
        CAutoLoot::FilterConfig CAutoLoot::m_ItemFilter;
        
        // Thread-safe storage for monster-player pairs during loot processing
        struct AutoLootContext
        {
            ATF::CMonster* pMonster;
            ATF::CPlayer* pPlayer;
            bool shouldAutoLoot;
        };
        thread_local AutoLootContext* g_CurrentLootContext = nullptr;
        
        CAutoLoot::CAutoLoot()
        {
            m_MonsterFilter.mode = 0;
            m_GradeFilter.mode = 0;
            m_ItemFilter.mode = 0;
        }
        
        void CAutoLoot::load()
        {
            enable_hook(&ATF::CMonster::Destroy, &CAutoLoot::Destroy);
            enable_hook(&ATF::CItemBox::SendMsg_Create, &CAutoLoot::SendMsg_Create);
            enable_hook(&ATF::CPlayer::pc_TakeGroundingItem, &CAutoLoot::pc_TakeGroundingItem);
            enable_hook(&ATF::CItemBox::IsTakeRight, &CAutoLoot::IsTakeRight);
        }

        void CAutoLoot::unload()
        {
            cleanup_all_hook();
        }

        Yorozuya::Module::ModuleName_t CAutoLoot::get_name()
        {
            static const Yorozuya::Module::ModuleName_t name = "addon.auto_loot";
            return name;
        }

        void CAutoLoot::configure(const rapidjson::Value & nodeConfig)
        {
            m_bActivated = RapidHelper::GetValueOrDefault(nodeConfig, "activated", false);
            m_bPremiumOnly = RapidHelper::GetValueOrDefault(nodeConfig, "premium_only", false);
            
            // Load monster filter
            if (nodeConfig.HasMember("monster_filter") && nodeConfig["monster_filter"].IsObject())
            {
                const auto& filter = nodeConfig["monster_filter"];
                m_MonsterFilter.mode = RapidHelper::GetValueOrDefault(filter, "mode", 0);
                m_MonsterFilter.values.clear();
                if (filter.HasMember("values") && filter["values"].IsArray())
                {
                    for (const auto& val : filter["values"].GetArray())
                    {
                        if (val.IsString())
                        {
                            m_MonsterFilter.values.push_back(val.GetString());
                        }
                    }
                }
            }
            else
            {
                m_MonsterFilter.mode = 0;
                m_MonsterFilter.values.clear();
            }
            
            // Load grade filter
            if (nodeConfig.HasMember("grade_filter") && nodeConfig["grade_filter"].IsObject())
            {
                const auto& filter = nodeConfig["grade_filter"];
                m_GradeFilter.mode = RapidHelper::GetValueOrDefault(filter, "mode", 0);
                m_GradeFilter.values.clear();
                if (filter.HasMember("values") && filter["values"].IsArray())
                {
                    for (const auto& val : filter["values"].GetArray())
                    {
                        if (val.IsInt())
                        {
                            m_GradeFilter.values.push_back(std::to_string(val.GetInt()));
                        }
                        else if (val.IsString())
                        {
                            m_GradeFilter.values.push_back(val.GetString());
                        }
                    }
                }
            }
            else
            {
                m_GradeFilter.mode = 0;
                m_GradeFilter.values.clear();
            }
            
            // Load item filter
            if (nodeConfig.HasMember("item_filter") && nodeConfig["item_filter"].IsObject())
            {
                const auto& filter = nodeConfig["item_filter"];
                m_ItemFilter.mode = RapidHelper::GetValueOrDefault(filter, "mode", 0);
                m_ItemFilter.values.clear();
                if (filter.HasMember("values") && filter["values"].IsArray())
                {
                    for (const auto& val : filter["values"].GetArray())
                    {
                        if (val.IsString())
                        {
                            m_ItemFilter.values.push_back(val.GetString());
                        }
                    }
                }
            }
            else
            {
                m_ItemFilter.mode = 0;
                m_ItemFilter.values.clear();
            }
        }
        
        bool CAutoLoot::ShouldAutoLoot(ATF::CMonster* pMonster, ATF::CPlayer* pPlayer)
        {
            if (!m_bActivated)
                return false;
                
            if (!pMonster || !pPlayer)
                return false;
                
            // Check if player is alive
            if (!pPlayer->m_bLive)
                return false;
                
            // Check premium requirement
            if (m_bPremiumOnly && !pPlayer->IsApplyPcbangPrimium())
                return false;
                
            // Check if monster is boss
            if (pMonster->IsBossMonster())
                return false;
                
            // Check monster filter
            if (!CheckMonsterFilter(pMonster))
                return false;
                
            // Check grade filter
            if (!CheckGradeFilter(pMonster))
                return false;
                
            return true;
        }
        
        bool CAutoLoot::CheckMonsterFilter(ATF::CMonster* pMonster)
        {
            if (!pMonster || !pMonster->m_pMonRec)
                return false;
                
            if (m_MonsterFilter.values.empty())
                return true; // No filter = all monsters
                
            std::string monsterCode = std::string(pMonster->m_pMonRec->m_strCode);
            bool found = std::find(m_MonsterFilter.values.begin(), m_MonsterFilter.values.end(), monsterCode) != m_MonsterFilter.values.end();
            
            if (m_MonsterFilter.mode == 0) // except
                return !found; // Return true if NOT in list
            else // only
                return found; // Return true if IN list
        }
        
        bool CAutoLoot::CheckGradeFilter(ATF::CMonster* pMonster)
        {
            if (!pMonster)
                return false;
                
            if (m_GradeFilter.values.empty())
                return true; // No filter = all grades
                
            int monsterGrade = pMonster->GetMonsterGrade();
            std::string gradeStr = std::to_string(monsterGrade);
            bool found = std::find(m_GradeFilter.values.begin(), m_GradeFilter.values.end(), gradeStr) != m_GradeFilter.values.end();
            
            if (m_GradeFilter.mode == 0) // except
                return !found; // Return true if NOT in list
            else // only
                return found; // Return true if IN list
        }
        
        bool CAutoLoot::CheckItemFilter(ATF::_STORAGE_LIST::_db_con* pItem)
        {
            if (!pItem)
                return false;
                
            if (m_ItemFilter.values.empty())
                return true; // No filter = all items
                
            std::string itemCode = GetItemCodeString(pItem);
            bool found = std::find(m_ItemFilter.values.begin(), m_ItemFilter.values.end(), itemCode) != m_ItemFilter.values.end();
            
            if (m_ItemFilter.mode == 0) // except
                return !found; // Return true if NOT in list
            else // only
                return found; // Return true if IN list
        }
        
        std::string CAutoLoot::GetItemCodeString(ATF::_STORAGE_LIST::_db_con* pItem)
        {
            if (!pItem)
                return "";
                
            if (pItem->m_byTableCode >= _countof(ATF::Global::g_MainThread->m_tblItemData))
                return "";
                
            auto& ItemRecords = ATF::Global::g_MainThread->m_tblItemData[pItem->m_byTableCode];
            
            // Iterate through records to find matching index
            int nRecordNum = ItemRecords.GetRecordNum();
            for (int i = 0; i < nRecordNum; ++i)
            {
                auto pRecord = ItemRecords.GetRecord(i);
                if (pRecord && pRecord->m_dwIndex == pItem->m_wItemIndex)
                {
                    if (pRecord->m_strCode[0] != '\0')
                        return std::string(pRecord->m_strCode);
                    break;
                }
            }
                
            return "";
        }
    
        bool WINAPIV CAutoLoot::Destroy(
            ATF::CMonster* pMonster,
            char byDestroyCode,
            ATF::CGameObject* pAttObj,
            ATF::Info::CMonsterDestroy46_ptr next)
        {
            // Store context for item box creation
            AutoLootContext context;
            context.pMonster = pMonster;
            context.pPlayer = nullptr;
            context.shouldAutoLoot = false;
            
            if (pAttObj && pAttObj->m_ObjID.m_byID == (BYTE)e_obj_id::obj_id_player)
            {
                ATF::CPlayer* pPlayer = (ATF::CPlayer*)pAttObj;
                context.pPlayer = pPlayer;
                context.shouldAutoLoot = ShouldAutoLoot(pMonster, pPlayer);
            }
            
            // Set thread-local context
            g_CurrentLootContext = &context;
            
            // Call original destroy method
            bool bResult = next(pMonster, byDestroyCode, pAttObj);
            
            // Clear context
            g_CurrentLootContext = nullptr;
            
            return bResult;
        }
        
        void WINAPIV CAutoLoot::SendMsg_Create(
            ATF::CItemBox* pBox,
            ATF::Info::CItemBoxSendMsg_Create14_ptr next)
        {
            // Check if we should auto loot this item
            if (g_CurrentLootContext && g_CurrentLootContext->shouldAutoLoot && g_CurrentLootContext->pPlayer)
            {
                // Check item filter
                if (CheckItemFilter(&pBox->m_Item))
                {
                    // Create the item box first (required for pc_TakeGroundingItem to work)
                    next(pBox);
                    
                    // Validate all conditions before attempting to pick up (similar to LootExchange)
                    do
                    {
                        // Check if box is still alive after creation
                        if (!pBox->m_bLive)
                            break;
                        
                        // Validate item table code
                        if (pBox->m_Item.m_byTableCode >= _countof(ATF::Global::g_MainThread->m_tblItemData))
                            break;
                        
                        // Find existing item to stack with (if item can overlap)
                        uint16_t wAddSerial = 0xFFFF; // Default: new item
                        
                        if (ATF::Global::IsOverLapItem(pBox->m_Item.m_byTableCode))
                        {
                            // Look for existing item with same type to stack with
                            for (auto& item : g_CurrentLootContext->pPlayer->m_Param.m_dbInven.m_List)
                            {
                                if (!item.m_bLoad || item.m_bLock)
                                    continue;
                                    
                                if (item.m_byTableCode != pBox->m_Item.m_byTableCode)
                                    continue;
                                    
                                if (item.m_wItemIndex != pBox->m_Item.m_wItemIndex)
                                    continue;
                                    
                                if (item.m_dwDur + pBox->m_Item.m_dwDur <= 99)
                                {
                                    wAddSerial = item.m_wSerial;
                                    break;
                                }
                            }
                        }
                        
                        // Save original position of item box
                        float fOriginalPos[3];
                        fOriginalPos[0] = pBox->m_fCurPos[0];
                        fOriginalPos[1] = pBox->m_fCurPos[1];
                        fOriginalPos[2] = pBox->m_fCurPos[2];
                        
                        // Temporarily move item box to player position to bypass distance check
                        // This ensures any internal distance validation in pc_TakeGroundingItem will pass
                        pBox->m_fCurPos[0] = g_CurrentLootContext->pPlayer->m_fCurPos[0];
                        pBox->m_fCurPos[1] = g_CurrentLootContext->pPlayer->m_fCurPos[1];
                        pBox->m_fCurPos[2] = g_CurrentLootContext->pPlayer->m_fCurPos[2];
                        
                        // Immediately pick up the item by simulating player action
                        // This will add the item to inventory server-side and notify client
                        // Note: pc_TakeGroundingItem internally validates inventory space and distance
                        g_CurrentLootContext->pPlayer->pc_TakeGroundingItem(pBox, wAddSerial);
                        
                        // Restore original position of item box
                        // (If item was successfully picked up, box may be destroyed, but we restore anyway for safety)
                        pBox->m_fCurPos[0] = fOriginalPos[0];
                        pBox->m_fCurPos[1] = fOriginalPos[1];
                        pBox->m_fCurPos[2] = fOriginalPos[2];
                    } while (false);
                    
                    return;
                }
            }
            
            // Normal behavior: create the item box
            next(pBox);
        }
        
        void WINAPIV CAutoLoot::pc_TakeGroundingItem(
            ATF::CPlayer* pObj,
            ATF::CItemBox* pBox,
            uint16_t wAddSerial,
            ATF::Info::CPlayerpc_TakeGroundingItem1947_ptr next)
        {
            // Check if we're in auto-loot context and should bypass delay
            bool bBypassDelay = false;
            if (g_CurrentLootContext && 
                g_CurrentLootContext->shouldAutoLoot && 
                g_CurrentLootContext->pPlayer == pObj)
            {
                // Check if this item should be auto-looted
                if (CheckItemFilter(&pBox->m_Item))
                {
                    bBypassDelay = true;
                }
            }
            
            // Save original delay time if we need to bypass
            unsigned int dwOriginalTakeItemTime = 0;
            if (bBypassDelay)
            {
                dwOriginalTakeItemTime = pObj->m_dwLastTakeItemTime;
                // Set to 0 (very old time) to bypass delay check
                // The delay check typically compares current time with m_dwLastTakeItemTime
                // Setting it to 0 ensures enough time has "passed"
                pObj->m_dwLastTakeItemTime = 0;
            }
            
            // Call original method (will process normally, but delay check will pass)
            next(pObj, pBox, wAddSerial);
        }
        
        bool WINAPIV CAutoLoot::IsTakeRight(
            ATF::CItemBox* pBox,
            ATF::CPlayer* pPlayer,
            ATF::Info::CItemBoxIsTakeRight10_ptr next)
        {
            // If we're in auto-loot context and should auto-loot this item, bypass distance check
            if (g_CurrentLootContext && 
                g_CurrentLootContext->shouldAutoLoot && 
                g_CurrentLootContext->pPlayer == pPlayer)
            {
                // Check if this item should be auto-looted
                if (CheckItemFilter(&pBox->m_Item))
                {
                    // Return true to bypass distance and rights check
                    // This allows items to be picked up regardless of distance
                    return true;
                }
            }
            
            // Normal behavior: call original method
            return next(pBox, pPlayer);
        }
    }
}
