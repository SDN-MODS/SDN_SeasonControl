// ============================================================================
// PASTA: 4_World/SDN_Core
// ARQUIVO: SDN_EnvironmentHook.c
// DESCRIÇÃO: Hook de Temperatura Solar Otimizado.
// ============================================================================

modded class Environment
{
    override float GetTemperature()
    {
        // 1. Tentar pegar a temperatura original primeiro
        float vanillaTemp = super.GetTemperature();

        // --- PROTEÇÕES ANTI-CRASH ---
        if (!GetGame() || !GetGame().GetWorld())
        {
            return vanillaTemp;
        }

        // No cliente, esperamos o jogador carregar
        if (GetGame().IsClient() && !GetGame().GetPlayer())
        {
            return vanillaTemp;
        }

        SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
        if (!manager)
        {
            return vanillaTemp;
        }

        // 2. Obter temperatura cacheada do Manager (Otimização de Performance)
        // Isso evita recalcular Math.Cos e Math.PI milhares de vezes por segundo.
        float finalTemp = manager.GetCachedTemperature();

        // 3. Adicionar influência da altitude (Vanilla) para realismo em montanhas
        // Se vanillaTemp for baixo (altitude alta), reduz a temperatura final levemente
        float altitudeInfluence = (vanillaTemp - 10.0) * 0.3;
        finalTemp += altitudeInfluence;

        return finalTemp;
    }
}