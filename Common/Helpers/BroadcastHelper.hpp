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
        // Enum para tipos de broadcast
        enum class e_broadcast_type
        {
            notice,      // Broadcast via packet (mensagem na tela)
            systemChat, // Broadcast via system chat (histórico do chat)
            both         // Envia ambos: notice e systemChat
        };

        // Helper para broadcast otimizado para todos os players online
        class CBroadcastHelper
        {
        public:
            // Função para broadcast simples - apenas players online
            // Otimizada: verifica flags na ordem mais eficiente e usa early exit
            template<typename TPacket>
            static void BroadcastToAllOnline(
                const TPacket& packet,
                char byType[2],
                size_t nPacketSize,
                e_broadcast_type eType = e_broadcast_type::notice,
                const ::std::string& sChatMessage = "")
            {
                // Loop otimizado: verifica m_bOper primeiro (mais comum ser false)
                // e usa índice direto para melhor cache locality
                for (uint16_t i = 0; i < ATF::Global::max_player; ++i)
                {
                    auto& player = ATF::Global::g_Player[i];
                    
                    // Verificação otimizada: m_bOper é mais rápido de verificar
                    // e geralmente é false para slots vazios
                    if (!player.m_bOper)
                        continue;
                    
                    if (!player.m_bLive)
                        continue;

                    // Envia system chat se necessário
                    if ((eType == e_broadcast_type::systemChat || eType == e_broadcast_type::both) 
                        && !sChatMessage.empty())
                    {
                        // Envia system chat para histórico
                        player.SendData_ChatTrans(
                            0,
                            -1,
                            player.GetObjRace(),
                            false,
                            (char*)sChatMessage.c_str(),
                            player.m_Param.m_byPvPGrade,
                            nullptr);
                    }

                    // Envia notice se necessário
                    if (eType == e_broadcast_type::notice || eType == e_broadcast_type::both)
                    {
                        // Envia mensagem usando índice direto (mais eficiente que m_ObjID.m_wIndex)
                        ATF::Global::g_NetProcess[(uint8_t)e_type_line::client]
                            ->LoadSendMsg(i, byType, (char*)&packet, nPacketSize);
                    }
                }
            }

            // Função para broadcast com filtro customizado
            template<typename TPacket, typename TFilter>
            static void BroadcastToAllOnlineWithFilter(
                const TPacket& packet,
                char byType[2],
                size_t nPacketSize,
                TFilter&& filter)
            {
                for (uint16_t i = 0; i < ATF::Global::max_player; ++i)
                {
                    auto& player = ATF::Global::g_Player[i];
                    
                    if (!player.m_bOper || !player.m_bLive)
                        continue;

                    // Aplica filtro customizado
                    if (!filter(player))
                        continue;

                    ATF::Global::g_NetProcess[(uint8_t)e_type_line::client]
                        ->LoadSendMsg(i, byType, (char*)&packet, nPacketSize);
                }
            }

            // Versão otimizada que retorna contagem de players online
            // Útil para estatísticas
            static size_t GetOnlinePlayerCount()
            {
                size_t count = 0;
                for (uint16_t i = 0; i < ATF::Global::max_player; ++i)
                {
                    auto& player = ATF::Global::g_Player[i];
                    if (player.m_bOper && player.m_bLive)
                        ++count;
                }
                return count;
            }
        };
    }
}

