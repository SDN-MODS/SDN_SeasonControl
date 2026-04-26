// ============================================================================
// PASTA: 4_World/SDN_Core
// ARQUIVO: SDN_EnvironmentHook.c
// DESCRIÇÃO: Hook de Temperatura Solar.
// ATUALIZAÇÃO:
// 1. Logs de Debug DESATIVADOS para Produção (Performance Máxima).
// 2. Matemática Solar (2*PI) Mantida.
// 3. Zero One-Liners.
// ============================================================================

modded class Environment
{
    override float GetTemperature()
    {
        // 1. Tentar pegar a temperatura original primeiro
        float vanillaTemp = super.GetTemperature();

        // --- PROTEÇÕES ANTI-CRASH ---
        if (!GetGame())
        {
            return vanillaTemp;
        }

        if (!GetGame().GetWorld())
        {
            return vanillaTemp;
        }

        // No cliente, esperamos o jogador carregar
        if (GetGame().IsClient())
        {
            if (!GetGame().GetPlayer())
            {
                return vanillaTemp;
            }
        }

        SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
        if (!manager)
        {
            return vanillaTemp;
        }

        // 2. Ler configurações do JSON via Manager
        float seasonBase = manager.GetCurrentBaseTemp();
        float variance = manager.GetTempVariance();

        // Se a config ainda não carregou ou for inválida (0.0), retorna vanilla
        if (Math.AbsFloat(seasonBase) < 0.01)
        {
            return vanillaTemp;
        }

        // 3. Calcular a Oscilação Solar via GetDayTime
        // Retorna float entre 0.00 e 24.00
        float timeInHours = GetGame().GetDayTime();

        // CORREÇÃO MATEMÁTICA:
        // Usamos (Math.PI * 2) para um ciclo completo de 360 graus.
        float timeRad = (timeInHours / 24.0) * (Math.PI * 2);

        // Invertemos o Cosseno para ter:
        // 12:00 (PI) -> -(-1) = +1 (Máximo Calor)
        // 00:00 (0)  -> -(1)  = -1 (Máximo Frio)
        float solarFactor = -Math.Cos(timeRad);

        // 4. Calcular Temperatura Final
        // Ex: Base 42 + (1.0 * 10) = 52 (Meio dia)
        // Ex: Base 42 + (-1.0 * 10) = 32 (Meia noite)
        float finalTemp = seasonBase + (solarFactor * variance);

        // (Opcional) Adiciona uma pequena influência da altitude vanilla para realismo em montanhas
        // Se vanillaTemp for baixo (altitude alta), reduz a temperatura final levemente
        float altitudeInfluence = (vanillaTemp - 10.0) * 0.3;
        finalTemp = finalTemp + altitudeInfluence;

        // --- DEBUG LOG (DESATIVADO PARA PRODUÇÃO) ---
        // ATENÇÃO: Manter comentado em servidores com muitos jogadores para evitar LAG de disco (I/O).
        // Descomente apenas se precisar diagnosticar problemas de temperatura.
        
        // string logMsg = "[CLIMA] Hora: " + timeInHours + " | Base: " + seasonBase + " | Var: " + variance + " | Solar: " + solarFactor + " | FINAL: " + finalTemp;
        // manager.Log(logMsg);

        return finalTemp;
    }
}