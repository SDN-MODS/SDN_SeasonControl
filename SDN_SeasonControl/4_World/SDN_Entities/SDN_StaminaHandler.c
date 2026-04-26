// ============================================================================
// PASTA: 4_World/SDN_Entities
// ARQUIVO: SDN_StaminaHandler.c
// CAMADA: 4_World
// EXECUÇÃO: Server & Client
// DESCRIÇÃO:
// Modifica a recuperação de Stamina baseada na estação (Cansaço no verão).
// ============================================================================

modded class StaminaHandler
{
    override void Update(float deltaT, int pCurrentCommandID)
    {
        super.Update(deltaT, pCurrentCommandID);
        
        // Apenas ajustamos se estiver recuperando stamina
        if (m_StaminaDelta > 0)
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            if (manager)
            {
                float staminaMult = manager.GetStaminaRecoveryMultiplier();
                
                // Se for 0.7 (Verão), a recuperação é cortada em 30%
                // Se for 1.0 (Normal), mantém
                // Aplicamos diretamente na stamina atual para ajustar a taxa
                // Nota: m_StaminaDelta já foi aplicado no super.Update, então precisamos
                // reverter e aplicar o novo, ou ajustar a próxima.
                // Maneira mais segura: Ajustar o cap ou regeneração base. 
                // Mas como delta é local, vamos reduzir o ganho efetivo.
                
                if (staminaMult < 1.0)
                {
                    // Removemos a parte "extra" que o vanilla adicionou
                    float added = m_StaminaDelta * deltaT; // O quanto subiu neste frame
                    float shouldAdd = added * staminaMult;
                    float difference = added - shouldAdd;
                    
                    // Subtrai a diferença para simular recuperação lenta
                    m_Stamina -= difference;
                }
            }
        }
    }
}