#pragma once

#include <ATF/global.hpp>
#include <vector>
#include <cstdint>

namespace GameServer
{
    namespace Helpers
    {
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
                size_t nPacketSize)
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

                    // Envia mensagem usando índice direto (mais eficiente que m_ObjID.m_wIndex)
                    ATF::Global::g_NetProcess[(uint8_t)e_type_line::client]
                        ->LoadSendMsg(i, byType, (char*)&packet, nPacketSize);
                }
            }

            // Função para broadcast com filtro customizado
            template<typename TPacket, typename TFilter>
            static void BroadcastToAllOnline(
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

