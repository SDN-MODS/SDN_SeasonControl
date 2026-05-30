// ============================================================================
// PASTA: 4_World/SDN_Core
// ARQUIVO: SDN_SeasonManager.c
// DESCRIÇÃO: Manager Central do Mod SDN_SeasonControl.
// ============================================================================

class SDN_SeasonManager
{
    private static ref SDN_SeasonManager m_Instance;

    // --- CONSTANTES VISUAIS ---
    protected const string SDN_ICON = "SDN_SeasonControl\\data\\seasonal_White.edds";
    protected const string SDN_SOUND = "SDN_SeasonControl\\Sounds\\Sound01.ogg";
    protected const int SDN_COLOR = -23296;

    // --- DADOS E CONFIGURAÇÃO ---
    protected ref SDN_SeasonConfig m_Config;
    protected ref SDN_SeasonSaveData m_Data;

    // --- RPCs ---
    static const int RPC_SEND_MESSAGE = 894712;
    static const int RPC_SYNC_SEASON_DATA = 894714;
    static const int RPC_PLAY_SOUND = 894715;

    // --- TIMERS E CACHE ---
    protected const float UPDATE_INTERVAL = 60.0;
    protected float m_TimeAccumulator;
    protected float m_CachedTemperature;

    protected float m_NotificationAccumulator;
    protected int m_CurrentNotificationIndex;

    // --- VARIÁVEIS CLIENTE ---
    protected int m_ClientCurrentSeasonIndex;
    protected ref SDN_SeasonSettings m_ClientCurrentSettings;
    protected ref SDN_SeasonSettings m_ClientNextSettings;
    protected int m_ClientStartTimestamp;
    protected int m_ClientDurationMinutes;
    protected float m_ClientTransitionPercent;

    // --- LOGS ---
    protected string m_LogFilePath;
    protected bool m_LoggingInitialized;

    void SDN_SeasonManager()
    {
        m_Config = new SDN_SeasonConfig();
        m_Data = new SDN_SeasonSaveData();
        m_ClientCurrentSettings = null;
        m_ClientNextSettings = null;
        m_LoggingInitialized = false;
        m_LogFilePath = "";
        m_NotificationAccumulator = 0.0;
        m_CurrentNotificationIndex = 0;
        m_CachedTemperature = 20.0;
    }

    static SDN_SeasonManager GetInstance()
    {
        if (!m_Instance)
        {
            m_Instance = new SDN_SeasonManager();
        }
        return m_Instance;
    }

    void Init()
    {
        // Inicialização Comum (Servidor e Cliente)
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.OnUpdateTimer, 1000, true);

        if (GetGame().IsServer())
        {
            InitLogging();
            GetGame().GetWeather().MissionWeather(false);

            LoadConfig();
            LoadPersistence();

            Log("=== SERVIDOR INICIADO ===");

            UpdateCachedTemperature();
            ApplyWeather(true);
            ApplyDateAndMoon();
        }
    }

    void InitLogging()
    {
        if (m_LoggingInitialized)
        {
            return;
        }

        if (!FileExist("$profile:SDN_Logs"))
        {
            MakeDirectory("$profile:SDN_Logs");
        }

        CF_Date now = CF_Date.Now();
        string dateStr = "" + now.GetYear() + "-" + now.GetMonth() + "-" + now.GetDay() + "_" + now.GetHours() + "-" + now.GetMinutes() + "-" + now.GetSeconds();
        m_LogFilePath = "$profile:SDN_Logs/SDN_Log_" + dateStr + ".log";

        FileHandle f = OpenFile(m_LogFilePath, FileMode.WRITE);
        if (f)
        {
            FPrintln(f, "==========================================");
            FPrintln(f, " SDN SEASON CONTROL - LOG DE SESSAO");
            FPrintln(f, " Data: " + dateStr);
            FPrintln(f, "==========================================");
            CloseFile(f);
            m_LoggingInitialized = true;
        }
    }

    void Log(string msg)
    {
        if (m_LoggingInitialized && m_LogFilePath != "")
        {
            FileHandle f = OpenFile(m_LogFilePath, FileMode.APPEND);
            if (f)
            {
                CF_Date now = CF_Date.Now();
                string timeStr = "[" + now.GetHours() + ":" + now.GetMinutes() + ":" + now.GetSeconds() + "] ";
                FPrintln(f, timeStr + msg);
                CloseFile(f);
            }
        }
    }

    void OnUpdateTimer()
    {
        m_TimeAccumulator += 1.0;

        if (m_TimeAccumulator >= UPDATE_INTERVAL)
        {
            if (GetGame().IsServer())
            {
                CheckSeasonProgression();
                ApplyWeather(false);
                ApplyDateAndMoon();
            }

            // Ambos (Srv/Cli) precisam de temperatura atualizada para o EnvironmentHook
            UpdateCachedTemperature();
            m_TimeAccumulator = 0;
        }

        if (GetGame().IsServer())
        {
            m_NotificationAccumulator += 1.0;
            if (m_NotificationAccumulator >= m_Config.NotificationInterval)
            {
                TriggerSeasonNotification();
                m_NotificationAccumulator = 0;
            }
        }
    }

    // ========================================================================
    // CACHE DE TEMPERATURA (Otimização Enfusion)
    // ========================================================================

    void UpdateCachedTemperature()
    {
        float seasonBase = GetInterpolatedValue(ESDN_SeasonParam.BASE_AIR_TEMP);
        float variance = GetInterpolatedValue(ESDN_SeasonParam.TEMP_VARIANCE);

        float timeInHours = GetGame().GetDayTime();
        float timeRad = (timeInHours / 24.0) * (Math.PI * 2);
        float solarFactor = -Math.Cos(timeRad);

        m_CachedTemperature = seasonBase + (solarFactor * variance);
    }

    float GetCachedTemperature()
    {
        return m_CachedTemperature;
    }

    // ========================================================================
    // SISTEMA DE NOTIFICAÇÕES
    // ========================================================================

    void TriggerSeasonNotification()
    {
        SDN_SeasonSettings curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);

        if (curr.SeasonNotifications && curr.SeasonNotifications.Count() > 0)
        {
            if (m_CurrentNotificationIndex >= curr.SeasonNotifications.Count())
            {
                m_CurrentNotificationIndex = 0;
            }

            SDN_NotificationEntry notif = curr.SeasonNotifications.Get(m_CurrentNotificationIndex);

            array<Man> players = new array<Man>;
            GetGame().GetPlayers(players);

            foreach (Man p : players)
            {
                SendNotificationToPlayer(p.GetIdentity(), notif);
            }

            Log("Notificacao Ciclica Enviada: " + notif.Title);
            m_CurrentNotificationIndex++;
        }
    }

    void SendNotificationToPlayer(PlayerIdentity identity, SDN_NotificationEntry notif)
    {
        if (!identity)
        {
            return;
        }

        if (SDN_SOUND != "")
        {
            ScriptRPC rpcSound = new ScriptRPC();
            rpcSound.Write(SDN_SOUND);
            rpcSound.Send(null, RPC_PLAY_SOUND, true, identity);
        }

        StringLocaliser titleLoc = new StringLocaliser(notif.Title);
        StringLocaliser textLoc = new StringLocaliser(notif.Text);

        NotificationSystem.Create(titleLoc, textLoc, SDN_ICON, SDN_COLOR, notif.Duration, identity);
    }

    void SendWelcomeNotification(PlayerIdentity identity)
    {
        if (m_Config.WelcomeNotification)
        {
            SendNotificationToPlayer(identity, m_Config.WelcomeNotification);
            Log("Notificacao de Boas Vindas enviada para: " + identity.GetName());
        }
    }

    void ScheduleWelcomeNotification(PlayerIdentity identity)
    {
        if (m_Config)
        {
            float delaySeconds = m_Config.JoinNotificationDelay;
            int delayMs = (int)(delaySeconds * 1000);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SendWelcomeNotification, delayMs, false, identity);
        }
    }

    // ========================================================================
    // LÓGICA DE ESTAÇÃO E TEMPO
    // ========================================================================

    int GetTimestamp()
    {
        return CF_Date.Now().GetTimestamp();
    }

    int GetSecondsRemaining()
    {
        int start = 0;
        int duration = 0;

        if (GetGame().IsServer())
        {
            start = m_Data.SeasonStartTimestamp;
            duration = m_Config.SeasonDurationMinutes;
        }
        else
        {
            start = m_ClientStartTimestamp;
            duration = m_ClientDurationMinutes;
        }

        int current = GetTimestamp();
        int end = start + (duration * 60);
        return (end - current);
    }

    void CheckSeasonProgression()
    {
        int current = GetTimestamp();
        int elapsedMinutes = (current - m_Data.SeasonStartTimestamp) / 60;

        if (elapsedMinutes >= m_Config.SeasonDurationMinutes)
        {
            AdvanceSeason();
        }
    }

    float GetTransitionFactor()
    {
        int start = 0;
        int duration = 0;
        float pct = 0.2;

        if (GetGame().IsServer())
        {
            start = m_Data.SeasonStartTimestamp;
            duration = m_Config.SeasonDurationMinutes;
            pct = m_Config.TransitionPercent;
        }
        else
        {
            start = m_ClientStartTimestamp;
            duration = m_ClientDurationMinutes;
            pct = m_ClientTransitionPercent;
        }

        if (duration == 0 || pct <= 0.001)
        {
            return 0.0;
        }

        float fElapsed = ((float)GetTimestamp() - (float)start) / 60.0;
        float fTransStart = (float)duration * (1.0 - pct);

        if (fElapsed < fTransStart)
        {
            return 0.0;
        }

        float fTransDur = (float)duration - fTransStart;
        if (fTransDur <= 0.01)
        {
            return 1.0;
        }

        return Math.Clamp((fElapsed - fTransStart) / fTransDur, 0.0, 1.0);
    }

    SDN_SeasonSettings GetNextSeasonSettings()
    {
        if (GetGame().IsServer())
        {
            int next = m_Data.CurrentSeasonIndex + 1;
            if (next > 3)
            {
                next = 0;
            }
            return m_Config.Seasons.Get(next);
        }
        return m_ClientNextSettings;
    }

    void AdvanceSeason()
    {
        m_Data.CurrentSeasonIndex++;
        if (m_Data.CurrentSeasonIndex > 3)
        {
            m_Data.CurrentSeasonIndex = 0;
        }

        m_Data.SeasonStartTimestamp = GetTimestamp();
        SavePersistence();
        Log("AVANCO DE ESTACAO: Nova Estacao -> " + GetCurrentSeasonName());

        m_CurrentNotificationIndex = 0;

        SyncToAllClients();
        UpdateCachedTemperature();
        ApplyWeather(true);
        ApplyDateAndMoon();
    }

    void ForceSeasonIndex(int index)
    {
        m_Data.CurrentSeasonIndex = index;
        m_Data.SeasonStartTimestamp = GetTimestamp();
        SavePersistence();
        SyncToAllClients();
        UpdateCachedTemperature();
        ApplyWeather(true);
        ApplyDateAndMoon();

        Log("ESTACAO FORCADA ADMIN: Index -> " + index);
        m_CurrentNotificationIndex = 0;
    }

    void SendStatusReport(PlayerIdentity identity)
    {
        if (!identity)
        {
            return;
        }

        float realTemp = GetCachedTemperature();
        string seasonName = GetCurrentSeasonName();
        int remainingMinutes = GetSecondsRemaining() / 60;

        SendPrivateMessage(identity, "--- STATUS SDN SEASON ---");
        SendPrivateMessage(identity, "Estacao Atual: " + seasonName);
        SendPrivateMessage(identity, "Tempo Restante: " + remainingMinutes + " minutos");
        SendPrivateMessage(identity, "-------------------------");
        SendPrivateMessage(identity, "Temp Real (Cache):  " + realTemp + " C");
        SendPrivateMessage(identity, "-------------------------");
    }

    // ========================================================================
    // APLICAÇÃO DE AMBIENTE
    // ========================================================================

    void ApplyDateAndMoon()
    {
        SDN_SeasonSettings curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
        int y, m, d, h, mn;
        GetGame().GetWorld().GetDate(y, m, d, h, mn);

        float currentMonth = (float)curr.SeasonMonth;
        float targetDay = (float)d;
        float lerp = GetTransitionFactor();

        if (lerp > 0.01)
        {
            SDN_SeasonSettings next = GetNextSeasonSettings();
            if (next)
            {
                float nextMonth = (float)next.SeasonMonth;
                if (nextMonth < currentMonth)
                {
                    nextMonth += 12.0;
                }

                float interpolatedVal = Math.Lerp(currentMonth, nextMonth, lerp);
                if (interpolatedVal > 12.9)
                {
                    interpolatedVal -= 12.0;
                }

                int finalMonth = (int)interpolatedVal;
                float fraction = interpolatedVal - finalMonth;
                targetDay = (int)(fraction * 28.0) + 1;
                currentMonth = (float)finalMonth;
            }
        }

        int applyMonth = (int)currentMonth;
        int applyDay = (int)targetDay;

        if (curr.ForceFullMoon != -1)
        {
            if (curr.ForceFullMoon == 1)
            {
                applyDay = 15;
            }
            else if (curr.ForceFullMoon == 0)
            {
                applyDay = 1;
            }
        }

        GetGame().GetWorld().SetDate(y, applyMonth, applyDay, h, mn);
    }

    void ApplyWeather(bool forceChange)
    {
        Weather weather = GetGame().GetWeather();

        // --- OTIMIZAÇÃO DE TRANSIÇÃO ---
        // Se já houver uma transição de nuvens em curso (mais de 10s restantes),
        // evitamos resetar o motor a cada 60s, a menos que seja uma mudança forçada (Admin ou Nova Estação).
        if (!forceChange && weather.GetOvercast().GetRemainingTime() > 10.0)
        {
            return;
        }

        float smoothTime = GetInterpolatedValue(ESDN_SeasonParam.SMOOTH_TIME);

        if (smoothTime < 10.0)
        {
            smoothTime = 180.0;
        }
        if (forceChange)
        {
            smoothTime = 0.0;
        }

        float tOvcMin = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.OVERCAST_MIN), 0.0, 1.0);
        float tOvcMax = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.OVERCAST_MAX), 0.0, 1.0);
        float tWind = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.WIND_LEVEL), 0.0, 1.0);
        float tRain = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.RAIN_CHANCE), 0.0, 1.0);
        float tFog = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.FOG_CHANCE), 0.0, 1.0);

        float tRainMin = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.RAIN_INTENSITY_MIN), 0.0, 1.0);
        float tRainMax = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.RAIN_INTENSITY_MAX), 0.0, 1.0);
        float tFogMin = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.FOG_INTENSITY_MIN), 0.0, 1.0);
        float tFogMax = Math.Clamp(GetInterpolatedValue(ESDN_SeasonParam.FOG_INTENSITY_MAX), 0.0, 1.0);

        if (tOvcMin > tOvcMax)
        {
            tOvcMin = tOvcMax;
        }

        float dice = Math.RandomFloat01();
        bool isRaining = (tRain > 0.01 && dice < tRain);

        // --- APLICAÇÃO ---
        weather.GetOvercast().SetLimits(0.0, 1.0);

        if (isRaining)
        {
            weather.GetOvercast().Set(Math.RandomFloat(0.85, 1.0), smoothTime);
        }
        else
        {
            weather.GetOvercast().Set(Math.RandomFloat(tOvcMin, tOvcMax), smoothTime);
        }

        // Vento
        if (tWind <= 0.05)
        {
            weather.GetWindMagnitude().SetLimits(0.0, 0.0);
            weather.GetWindMagnitude().Set(0.0, smoothTime);
        }
        else
        {
            float wMin = Math.Max(0.1, tWind - 0.1);
            float wMax = Math.Min(1.0, tWind + 0.1);
            weather.GetWindMagnitude().SetLimits(wMin, wMax);
            weather.GetWindMagnitude().Set(tWind, smoothTime);
        }

        // Chuva
        weather.GetRain().SetLimits(0.0, 1.0);
        if (isRaining)
        {
            // Se for Inverno (Index 3), força céu nublado total para neve
            if (GetGame().IsServer() && m_Data.CurrentSeasonIndex == 3)
            {
                weather.GetOvercast().Set(1.0, smoothTime);
            }

            weather.GetRain().Set(Math.RandomFloat(Math.Min(tRainMin, tRainMax), Math.Max(tRainMin, tRainMax)), smoothTime);
        }
        else
        {
            weather.GetRain().Set(0.0, smoothTime);
        }

        // Neblina
        weather.GetFog().SetLimits(0.0, 1.0);
        if (dice < tFog)
        {
            weather.GetFog().Set(Math.RandomFloat(Math.Min(tFogMin, tFogMax), Math.Max(tFogMin, tFogMax)), smoothTime);
        }
        else
        {
            weather.GetFog().Set(0.0, smoothTime);
        }
    }

    // --- GETTERS (Refatorados para ENUM) ---
    float GetWaterMultiplier() { return GetInterpolatedValue(ESDN_SeasonParam.WATER_DEPLETION); }
    float GetEnergyMultiplier() { return GetInterpolatedValue(ESDN_SeasonParam.ENERGY_DEPLETION); }
    float GetFoodDecayMultiplier() { return GetInterpolatedValue(ESDN_SeasonParam.FOOD_DECAY); }
    float GetItemDryingMultiplier() { return GetInterpolatedValue(ESDN_SeasonParam.ITEM_DRYING); }
    float GetStaminaRecoveryMultiplier() { return GetInterpolatedValue(ESDN_SeasonParam.STAMINA_RECOVERY); }
    float GetSicknessChance() { return GetInterpolatedValue(ESDN_SeasonParam.SICKNESS_CHANCE); }

    float GetInterpolatedValue(ESDN_SeasonParam param)
    {
        SDN_SeasonSettings curr;
        if (GetGame().IsServer())
        {
            curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
        }
        else
        {
            curr = m_ClientCurrentSettings;
        }

        if (!curr)
        {
            return 0.0;
        }

        float val = GetParamValue(curr, param);
        float lerp = GetTransitionFactor();

        if (lerp > 0.01)
        {
            SDN_SeasonSettings next = GetNextSeasonSettings();
            if (next)
            {
                val = Math.Lerp(val, GetParamValue(next, param), lerp);
            }
        }

        return val;
    }

    protected float GetParamValue(SDN_SeasonSettings s, ESDN_SeasonParam param)
    {
        switch (param)
        {
            case ESDN_SeasonParam.BASE_AIR_TEMP: return s.BaseAirTemp;
            case ESDN_SeasonParam.TEMP_VARIANCE: return s.TempVariance;
            case ESDN_SeasonParam.WATER_DEPLETION: return s.WaterDepletionMult;
            case ESDN_SeasonParam.ENERGY_DEPLETION: return s.EnergyDepletionMult;
            case ESDN_SeasonParam.FOOD_DECAY: return s.FoodDecayMult;
            case ESDN_SeasonParam.ITEM_DRYING: return s.ItemDryingMult;
            case ESDN_SeasonParam.STAMINA_RECOVERY: return s.StaminaRecoveryMult;
            case ESDN_SeasonParam.SICKNESS_CHANCE: return s.SicknessChance;
            case ESDN_SeasonParam.OVERCAST_MIN: return s.OvercastMin;
            case ESDN_SeasonParam.OVERCAST_MAX: return s.OvercastMax;
            case ESDN_SeasonParam.WIND_LEVEL: return s.WindLevel;
            case ESDN_SeasonParam.RAIN_CHANCE: return s.RainChance;
            case ESDN_SeasonParam.FOG_CHANCE: return s.FogChance;
            case ESDN_SeasonParam.RAIN_INTENSITY_MIN: return s.RainIntensityMin;
            case ESDN_SeasonParam.RAIN_INTENSITY_MAX: return s.RainIntensityMax;
            case ESDN_SeasonParam.FOG_INTENSITY_MIN: return s.FogIntensityMin;
            case ESDN_SeasonParam.FOG_INTENSITY_MAX: return s.FogIntensityMax;
            case ESDN_SeasonParam.SMOOTH_TIME: return s.SmoothTime;
        }
        return 0.0;
    }

    string GetCurrentSeasonName()
    {
        if (GetGame().IsServer())
        {
            if (m_Config && m_Config.Seasons && m_Data)
            {
                if (m_Data.CurrentSeasonIndex >= 0 && m_Data.CurrentSeasonIndex < m_Config.Seasons.Count())
                {
                    SDN_SeasonSettings s = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
                    if (s)
                    {
                        return s.SeasonName;
                    }
                }
            }
        }
        else if (m_ClientCurrentSettings)
        {
            return m_ClientCurrentSettings.SeasonName;
        }
        return "Unknown";
    }

    // ========================================================================
    // IO E NETWORKING
    // ========================================================================

    void LoadConfig()
    {
        if (!FileExist(SDN_Consts.CONFIG_DIR))
        {
            MakeDirectory(SDN_Consts.CONFIG_DIR);
        }
        if (FileExist(SDN_Consts.CONFIG_FILE))
        {
            JsonFileLoader<SDN_SeasonConfig>.JsonLoadFile(SDN_Consts.CONFIG_FILE, m_Config);
        }
        else
        {
            SaveConfig();
        }
    }

    void SaveConfig()
    {
        JsonFileLoader<SDN_SeasonConfig>.JsonSaveFile(SDN_Consts.CONFIG_FILE, m_Config);
    }

    void LoadPersistence()
    {
        if (FileExist(SDN_Consts.SAVE_FILE))
        {
            JsonFileLoader<SDN_SeasonSaveData>.JsonLoadFile(SDN_Consts.SAVE_FILE, m_Data);
            if (m_Data.SeasonStartTimestamp == 0)
            {
                m_Data.SeasonStartTimestamp = GetTimestamp();
            }
        }
        else
        {
            m_Data.SeasonStartTimestamp = GetTimestamp();
            SavePersistence();
        }
    }

    void SavePersistence()
    {
        JsonFileLoader<SDN_SeasonSaveData>.JsonSaveFile(SDN_Consts.SAVE_FILE, m_Data);
    }

    PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        foreach (Man p : players)
        {
            if (p.GetIdentity() == identity)
            {
                return PlayerBase.Cast(p);
            }
        }
        return null;
    }

    void SendPrivateMessage(PlayerIdentity identity, string msg)
    {
        if (!GetGame().IsServer())
        {
            return;
        }
        PlayerBase targetPlayer = GetPlayerByIdentity(identity);
        if (targetPlayer)
        {
            ScriptRPC rpc = new ScriptRPC();
            rpc.Write(msg);
            rpc.Send(targetPlayer, RPC_SEND_MESSAGE, true, identity);
        }
    }

    void SyncToPlayer(PlayerIdentity identity)
    {
        if (!GetGame().IsServer())
        {
            return;
        }

        PlayerBase targetPlayer = GetPlayerByIdentity(identity);
        if (targetPlayer)
        {
            int currentIdx = m_Data.CurrentSeasonIndex;
            int nextIdx = currentIdx + 1;
            if (nextIdx > 3)
            {
                nextIdx = 0;
            }

            SDN_SeasonSettings current = m_Config.Seasons.Get(currentIdx);
            SDN_SeasonSettings next = m_Config.Seasons.Get(nextIdx);

            ScriptRPC rpc = new ScriptRPC();
            rpc.Write(currentIdx);
            rpc.Write(current);
            rpc.Write(next);
            rpc.Write(m_Data.SeasonStartTimestamp);
            rpc.Write(m_Config.SeasonDurationMinutes);
            rpc.Write(m_Config.TransitionPercent);
            rpc.Send(targetPlayer, RPC_SYNC_SEASON_DATA, true, identity);
        }
    }

    void SyncToAllClients()
    {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        foreach (Man player : players)
        {
            SyncToPlayer(player.GetIdentity());
        }
    }

    void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type == RPC_SYNC_SEASON_DATA)
        {
            int idx, startTime, duration;
            SDN_SeasonSettings settings, nextSettings;
            float transitionPct;

            if (!ctx.Read(idx)) return;
            if (!ctx.Read(settings)) return;
            if (!ctx.Read(nextSettings)) return;
            if (!ctx.Read(startTime)) return;
            if (!ctx.Read(duration)) return;
            if (!ctx.Read(transitionPct)) return;

            m_ClientCurrentSeasonIndex = idx;
            m_ClientCurrentSettings = settings;
            m_ClientNextSettings = nextSettings;
            m_ClientStartTimestamp = startTime;
            m_ClientDurationMinutes = duration;
            m_ClientTransitionPercent = transitionPct;

            // Atualiza temperatura local imediatamente após sincronizar
            UpdateCachedTemperature();
        }
        else if (rpc_type == RPC_SEND_MESSAGE)
        {
            string msg;
            if (!ctx.Read(msg)) return;
            ChatMessageEventParams chatParams = new ChatMessageEventParams(CCSystem, "SDN System", msg, "");
            GetGame().GetMission().OnEvent(ChatMessageEventTypeID, chatParams);
        }
        else if (rpc_type == RPC_PLAY_SOUND)
        {
            string soundFile;
            if (!ctx.Read(soundFile)) return;
            if (GetGame().GetPlayer())
            {
                EffectSound sound = SEffectManager.PlaySound(soundFile, GetGame().GetPlayer().GetPosition(), 0, 0, false);
                if (sound)
                {
                    sound.SetSoundAutodestroy(true);
                }
            }
        }
    }

    bool IsAnimalAllowed(string animalClass)
    {
        if (!GetGame().IsServer())
        {
            return true;
        }

        SDN_SeasonSettings curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
        if (!curr.AllowedAnimals || curr.AllowedAnimals.Count() == 0)
        {
            return true;
        }

        animalClass.ToLower();
        foreach (string allowed : curr.AllowedAnimals)
        {
            allowed.ToLower();
            if (animalClass.Contains(allowed))
            {
                return true;
            }
        }
        return false;
    }
}