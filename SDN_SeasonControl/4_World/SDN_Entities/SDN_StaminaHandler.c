// ============================================================================
// PASTA: 4_World/SDN_Entities
// ARQUIVO: SDN_StaminaHandler.c
// DESCRIÇÃO: Modifica a recuperação de Stamina baseada na estação via SDN_SeasonManager.
// ============================================================================

modded class StaminaHandler
{
    override void Update(float deltaT, int pCurrentCommandID)
    {
        super.Update(deltaT, pCurrentCommandID);

        if (m_StaminaDelta > 0)
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            if (manager)
            {
                float staminaMult = manager.GetInterpolatedValue(ESDN_SeasonParam.STAMINA_RECOVERY);
                if (staminaMult < 1.0)
                {
                    float added = m_StaminaDelta * deltaT;
                    float shouldAdd = added * staminaMult;
                    m_Stamina -= (added - shouldAdd);
                }
            }
        }
    }
}