// ============================================================================
// PASTA: 4_World/SDN_Entities
// ARQUIVO: SDN_ItemBase.c
// DESCRIÇÃO: Controle de apodrecimento de comida via SDN_SeasonManager.
// ============================================================================

modded class Edible_Base
{
    override void ProcessDecay(float delta, bool hasRootAsPlayer)
    {
        if (GetGame().IsServer())
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            if (manager)
            {
                // Uso do novo sistema de ENUM para performance
                float decayMult = manager.GetInterpolatedValue(ESDN_SeasonParam.FOOD_DECAY);
                delta *= decayMult;
            }
        }

        super.ProcessDecay(delta, hasRootAsPlayer);
    }
}