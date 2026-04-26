// ============================================================================
// PASTA: 4_World/SDN_System
// ARQUIVO: SDN_AnimalSystem.c
// DESCRIÇÃO: Sistema de Controle de Fauna.
// CORREÇÃO: Deleção segura (Delayed) para evitar crash de 'Access Violation' no EEInit.
// ============================================================================

modded class AnimalBase
{
    override void EEInit()
    {
        super.EEInit();

        // Apenas no servidor
        if (GetGame().IsServer())
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            
            // Verifica se o manager existe
            if (manager)
            {
                string myType = this.GetType();
                
                // Pergunta ao manager se este animal é permitido hoje
                if (!manager.IsAnimalAllowed(myType))
                {
                    // CRASH FIX: Não podemos usar ObjectDelete(this) diretamente aqui,
                    // pois o animal ainda está inicializando (EEInit).
                    // Agendamos a deleção para 1ms depois (próximo frame).
                    GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().ObjectDelete, 1, false, this);
                }
            }
        }
    }
}