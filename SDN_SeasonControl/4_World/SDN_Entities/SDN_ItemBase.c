// ============================================================================
// PASTA: 4_World/SDN_Entities
// ARQUIVO: SDN_ItemBase.c
// CAMADA: 4_World
// EXECUÇÃO: Server
// DESCRIÇÃO:
// Controle de apodrecimento de comida (Edible_Base).
// Lógica de roupas removida (está no PlayerBase).
// ============================================================================

modded class Edible_Base
{
    // Intercepta o processamento de decomposição (Rotten)
    override void ProcessDecay(float delta, bool hasRootAsPlayer)
    {
        if (GetGame().IsServer())
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            if (manager)
            {
                float decayMult = manager.GetFoodDecayMultiplier();
                
                // Aplica o multiplicador do JSON
                // Se decayMult > 1.0 (Verão), estraga mais rápido.
                // Se decayMult < 1.0 (Inverno), preserva.
                delta = delta * decayMult;
            }
        }

        super.ProcessDecay(delta, hasRootAsPlayer);
    }
}