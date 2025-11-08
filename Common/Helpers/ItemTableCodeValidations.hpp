#pragma once

#include <ATF/global.hpp>
#include <vector>
#include <cstdint>
#include <string>
#include <type_traits>

namespace GameServer
{
    namespace Helpers
    {
        class CItemTableCodeValidations
        {
            public:
            CItemTableCodeValidations() { };

            static bool ValidateItemTableCode(
                char byTableCode,
                uint16_t wItemIndex
            )
            {
                // First check if the item exists in the specified table
                if (byTableCode >= _countof(ATF::Global::g_MainThread->m_tblItemData))
                {
                    return false;
                }

                auto& SpecifiedTable = ATF::Global::g_MainThread->m_tblItemData[byTableCode];
                auto pRecordInSpecified = SpecifiedTable.GetRecord(wItemIndex);
                
                // If item doesn't exist in specified table, it's invalid
                if (pRecordInSpecified == nullptr)
                {
                    return false;
                }

                // Now verify that the item doesn't exist in any other table
                // This prevents exploits where byTableCode is modified
                for (size_t i = 0; i < _countof(ATF::Global::g_MainThread->m_tblItemData); ++i)
                {
                    if (i == byTableCode)
                    {
                        continue; // Skip the specified table, already checked
                    }

                    auto& OtherTable = ATF::Global::g_MainThread->m_tblItemData[i];
                    auto pRecordInOther = OtherTable.GetRecord(wItemIndex);
                    
                    // If item exists in another table, the byTableCode is invalid
                    if (pRecordInOther != nullptr)
                    {
                        return false;
                    }
                }

                // Item exists only in the specified table, validation passed
                return true;
            }
        };
    }
}