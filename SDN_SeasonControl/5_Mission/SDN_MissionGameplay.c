// ============================================================================
// PASTA: 5_Mission
// ARQUIVO: SDN_MissionGameplay.c
// EXECUÇÃO: Client
// DESCRIÇÃO: Inicialização do Season Manager no lado do Cliente (Gameplay).
// ============================================================================

modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        // Inicializa o Manager no Cliente para permitir cálculos locais de clima e temperatura
        if (SDN_SeasonManager.GetInstance())
        {
            SDN_SeasonManager.GetInstance().Init();
        }
    }
}