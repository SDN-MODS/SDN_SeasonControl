// ============================================================================
// MOD: SDN_SeasonControl (Client-Side)
// ARQUIVO: SDN_SeasonManager.c
// DESCRIO: Manager Central. Depende do SDN_ServerModule para validacao.
// ============================================================================

class SDN_SeasonManager
{
    private static ref SDN_SeasonManager m_Instance;

    // --- VARIVEL DE SEGURANCA (DRM) ---
    // Controlada pela resposta do SDN_LicenseValidator
    protected bool m_IsLicensed;

    // --- CONSTANTES VISUAIS ---
    protected const string SDN_ICON = "SDN_SeasonControl\\data\\seasonal_White.edds";
    protected const string SDN_SOUND = "SDN_SeasonControl\\Sounds\\Sound01.ogg";
    protected const int SDN_COLOR = -23296;

    // --- DADOS E CONFIGURAO ---
    protected ref SDN_SeasonConfig m_Config;
    protected ref SDN_SeasonSaveData m_Data;

    // --- RPCs ---
    static const int RPC_SEND_MESSAGE = 894712;
    static const int RPC_SYNC_SEASON_DATA = 894714;
    static const int RPC_PLAY_SOUND = 894715;

    // --- TIMERS ---
    protected const float UPDATE_INTERVAL = 60.0;
    protected float m_TimeAccumulator;

    protected float m_NotificationAccumulator;
    protected int m_CurrentNotificationIndex;

    // --- VARIVEIS CLIENTE ---
    protected int m_ClientCurrentSeasonIndex;
    protected ref SDN_SeasonSettings m_ClientCurrentSettings;
    protected int m_ClientStartTimestamp;
    protected int m_ClientDurationMinutes;

    // --- LOGS ---
    protected string m_LogFilePath;
    protected bool m_LoggingInitialized;

    void SDN_SeasonManager()
    {
        m_Config = new SDN_SeasonConfig();
        m_Data = new SDN_SeasonSaveData();
        m_ClientCurrentSettings = null;
        m_LoggingInitialized = false;
        m_LogFilePath = "";
        m_NotificationAccumulator = 0.0;
        m_CurrentNotificationIndex = 0;

        // Inicia bloqueado at a validacao no Init
        m_IsLicensed = false;
    }

    static SDN_SeasonManager GetInstance()
    {
        if (!m_Instance)
        {
            m_Instance = new SDN_SeasonManager();
        }
        return m_Instance;
    }

    // ========================================================================
    // INICIALIZACAO E PONTE DE SEGURANCA
    // ========================================================================

    void Init()
    {
        if (GetGame().IsServer())
        {
            // CHAMADA PARA A PONTE DE SEGURANCA (NOVO MOTOR)
            if (!SDN_LicenseValidator.IsLicenseValid("SDN_SeasonControl"))
            {
                Print("[SDN MANAGER] ===================================================");
                Print("[SDN MANAGER] BLOQUEIO DE SEGURANCA ATIVO");
                Print("[SDN MANAGER] O Modulo Server-Side nao esta instalado ou a licenca e invalida.");
                Print("[SDN MANAGER] Instale o @SDN_ServerModule na pasta do servidor.");
                Print("[SDN MANAGER] ===================================================");

                m_IsLicensed = false;

                // Forca o desligamento imediato (Kill Switch Local)
                GetGame().RequestExit(0);
                return;
            }

            // Se passou pela ponte, o mod esta autorizado
            m_IsLicensed = true;

            // INICIALIZACAO DOS SISTEMAS
            InitLogging();

            GetGame().GetWeather().MissionWeather(false);

            LoadConfig();
            LoadPersistence();

            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.OnUpdateTimer, 1000, true);

            Print("[SDN] MOD INICIADO E LICENCIADO VIA SERVER-MODULE.");
            Log("=== SERVIDOR INICIADO ===");
            Log("Status: LICENCIADO (Validacao Server-Side OK)");

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

        if (!FileExist(SDN_Consts.MODS_DIR))
        {
            MakeDirectory(SDN_Consts.MODS_DIR);
        }

        if (!FileExist(SDN_Consts.BASE_DIR))
        {
            MakeDirectory(SDN_Consts.BASE_DIR);
        }

        if (!FileExist(SDN_Consts.LOG_DIR))
        {
            MakeDirectory(SDN_Consts.LOG_DIR);
        }

        CF_Date now = CF_Date.Now();
        string dateStr = "" + now.GetYear() + "-" + now.GetMonth() + "-" + now.GetDay() + "_" + now.GetHours() + "-" + now.GetMinutes() + "-" + now.GetSeconds();
        m_LogFilePath = SDN_Consts.LOG_DIR + "/SDN_Log_" + dateStr + ".log";

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
        if (m_LoggingInitialized)
        {
            if (m_LogFilePath != "")
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
    }

    // ========================================================================
    // LOOP PRINCIPAL (TIMER)
    // ========================================================================

    void OnUpdateTimer()
    {
        if (!GetGame().IsServer())
        {
            return;
        }

        // Proteo: Se no estiver licenciado, o timer no faz nada
        if (!m_IsLicensed)
        {
            return;
        }

        m_TimeAccumulator = m_TimeAccumulator + 1.0;

        if (m_TimeAccumulator >= UPDATE_INTERVAL)
        {
            CheckSeasonProgression();
            ApplyWeather(false);
            ApplyDateAndMoon();
            m_TimeAccumulator = 0;
        }

        m_NotificationAccumulator = m_NotificationAccumulator + 1.0;

        if (m_NotificationAccumulator >= m_Config.NotificationInterval)
        {
            TriggerSeasonNotification();
            m_NotificationAccumulator = 0;
        }
    }

    // ========================================================================
    // SISTEMA DE NOTIFICAES
    // ========================================================================

    void TriggerSeasonNotification()
    {
        if (!m_IsLicensed)
        {
            return;
        }

        SDN_SeasonSettings curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);

        if (curr.SeasonNotifications)
        {
            if (curr.SeasonNotifications.Count() > 0)
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
                m_CurrentNotificationIndex = m_CurrentNotificationIndex + 1;
            }
        }
    }

    void SendNotificationToPlayer(PlayerIdentity identity, SDN_NotificationEntry notif)
    {
        if (!m_IsLicensed)
        {
            return;
        }

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
        if (!m_IsLicensed)
        {
            return;
        }

        if (m_Config.WelcomeNotification)
        {
            SendNotificationToPlayer(identity, m_Config.WelcomeNotification);
            Log("Notificacao de Boas Vindas enviada para: " + identity.GetName());
        }
    }

    void ScheduleWelcomeNotification(PlayerIdentity identity)
    {
        if (!m_IsLicensed)
        {
            return;
        }

        if (m_Config)
        {
            float delaySeconds = m_Config.JoinNotificationDelay;
            int delayMs = (int)(delaySeconds * 1000);

            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SendWelcomeNotification, delayMs, false, identity);
            Log("Notificacao de Boas Vindas agendada para " + identity.GetName() + " em " + delaySeconds + " s.");
        }
    }

    // ========================================================================
    // LOGICA DE ESTACAO E TEMPO
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
        if (!m_IsLicensed)
        {
            return;
        }

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
        }

        if (duration == 0)
        {
            return 0.0;
        }

        if (pct <= 0.001)
        {
            return 0.0;
        }

        float fCurrent = (float)GetTimestamp();
        float fStart = (float)start;
        float fDuration = (float)duration;
        float fElapsed = (fCurrent - fStart) / 60.0;
        float fTransStart = fDuration * (1.0 - pct);

        if (fElapsed < fTransStart)
        {
            return 0.0;
        }

        float fTransDur = fDuration - fTransStart;
        if (fTransDur <= 0.01)
        {
            return 1.0;
        }

        float result = (fElapsed - fTransStart) / fTransDur;
        return Math.Clamp(result, 0.0, 1.0);
    }

    SDN_SeasonSettings GetNextSeasonSettings()
    {
        int idx = 0;
        if (GetGame().IsServer())
        {
            idx = m_Data.CurrentSeasonIndex;
        }
        else
        {
            idx = m_ClientCurrentSeasonIndex;
        }

        int next = idx + 1;
        if (next > 3)
        {
            next = 0;
        }

        if (GetGame().IsServer())
        {
            return m_Config.Seasons.Get(next);
        }
        return m_ClientCurrentSettings;
    }

    void AdvanceSeason()
    {
        // Proteo extra
        if (!m_IsLicensed)
        {
            return;
        }

        m_Data.CurrentSeasonIndex = m_Data.CurrentSeasonIndex + 1;

        if (m_Data.CurrentSeasonIndex > 3)
        {
            m_Data.CurrentSeasonIndex = 0;
        }

        m_Data.SeasonStartTimestamp = GetTimestamp();
        SavePersistence();
        Log("AVANCO DE ESTACAO: Nova Estacao -> " + GetCurrentSeasonName());

        m_CurrentNotificationIndex = 0;

        SyncToAllClients();
        ApplyWeather(true);
        ApplyDateAndMoon();
    }

    void ForceSeasonIndex(int index)
    {
        // Proteo extra
        if (!m_IsLicensed)
        {
            return;
        }

        m_Data.CurrentSeasonIndex = index;
        m_Data.SeasonStartTimestamp = GetTimestamp();
        SavePersistence();
        SyncToAllClients();
        ApplyWeather(true);
        ApplyDateAndMoon();

        Log("ESTACAO FORCADA ADMIN: Index -> " + index);
        m_CurrentNotificationIndex = 0;
    }

    void SendStatusReport(PlayerIdentity identity)
    {
        if (!m_IsLicensed)
        {
            return;
        }

        if (!identity)
        {
            return;
        }

        float realTemp = 0.0;
        PlayerBase player = GetPlayerByIdentity(identity);

        if (player)
        {
            Environment env = new Environment(player);
            if (env)
            {
                realTemp = env.GetTemperature();
            }
        }
        else
        {
            realTemp = GetCurrentBaseTemp();
        }

        float baseTemp = GetCurrentBaseTemp();
        string seasonName = GetCurrentSeasonName();
        int remainingSeconds = GetSecondsRemaining();
        int remainingMinutes = remainingSeconds / 60;

        SendPrivateMessage(identity, "--- STATUS SDN SEASON ---");
        SendPrivateMessage(identity, "Estacao Atual: " + seasonName);
        SendPrivateMessage(identity, "Tempo Restante: " + remainingMinutes + " minutos");
        SendPrivateMessage(identity, "-------------------------");
        SendPrivateMessage(identity, "Temp Config (Base): " + baseTemp + " C");
        SendPrivateMessage(identity, "Temp Real (Mundo):  " + realTemp + " C");
        SendPrivateMessage(identity, "-------------------------");
    }

    // ========================================================================
    // APLICACAO DE AMBIENTE (LUA, CLIMA, INTERPOLAO)
    // ========================================================================

    void ApplyDateAndMoon()
    {
        if (!m_IsLicensed)
        {
            return;
        }

        SDN_SeasonSettings curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
        int y, m, d, h, mn;
        GetGame().GetWorld().GetDate(y, m, d, h, mn);

        float currentMonth = (float)curr.SeasonMonth;
        float targetDay = (float)d;
        float lerp = GetTransitionFactor();

        if (lerp > 0.01)
        {
            SDN_SeasonSettings next = GetNextSeasonSettings();
            float nextMonth = (float)next.SeasonMonth;

            if (nextMonth < currentMonth)
            {
                nextMonth = nextMonth + 12.0;
            }

            float interpolatedVal = Math.Lerp(currentMonth, nextMonth, lerp);

            if (interpolatedVal > 12.9)
            {
                interpolatedVal = interpolatedVal - 12.0;
            }

            int finalMonth = (int)interpolatedVal;
            float fraction = interpolatedVal - finalMonth;
            int finalDay = (int)(fraction * 28.0) + 1;

            currentMonth = (float)finalMonth;
            targetDay = (float)finalDay;
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
        if (!m_IsLicensed)
        {
            return;
        }

        Weather weather = GetGame().GetWeather();
        float smoothTime = GetInterpolatedValue("SmoothTime");

        if (smoothTime < 10.0)
        {
            smoothTime = 180.0;
        }

        if (forceChange)
        {
            smoothTime = 0.0;
        }

        // Obtm valores interpolados (transio suave)
        float tOvcMin = Math.Clamp(GetInterpolatedValue("OvercastMin"), 0.0, 1.0);
        float tOvcMax = Math.Clamp(GetInterpolatedValue("OvercastMax"), 0.0, 1.0);
        float tWind = Math.Clamp(GetInterpolatedValue("WindLevel"), 0.0, 1.0);
        float tRain = Math.Clamp(GetInterpolatedValue("RainChance"), 0.0, 1.0);
        float tFog = Math.Clamp(GetInterpolatedValue("FogChance"), 0.0, 1.0);

        float tRainMin = Math.Clamp(GetInterpolatedValue("RainIntensityMin"), 0.0, 1.0);
        float tRainMax = Math.Clamp(GetInterpolatedValue("RainIntensityMax"), 0.0, 1.0);
        float tFogMin = Math.Clamp(GetInterpolatedValue("FogIntensityMin"), 0.0, 1.0);
        float tFogMax = Math.Clamp(GetInterpolatedValue("FogIntensityMax"), 0.0, 1.0);

        if (tOvcMin > tOvcMax)
        {
            tOvcMin = tOvcMax;
        }

        float dice = Math.RandomFloat01();
        bool isRaining = false;

        if (tRain <= 0.01)
        {
            isRaining = false;
        }
        else if (dice < tRain)
        {
            isRaining = true;
        }

        // --- APLICACAO ---

        // 1. Nuvens (Overcast)
        weather.GetOvercast().SetLimits(0.0, 1.0);

        if (isRaining)
        {
            weather.GetOvercast().Set(Math.RandomFloat(0.85, 1.0), smoothTime);
        }
        else
        {
            weather.GetOvercast().Set(Math.RandomFloat(tOvcMin, tOvcMax), smoothTime);
        }

        // 2. Vento
        if (tWind <= 0.05)
        {
            weather.GetWindMagnitude().SetLimits(0.0, 0.0);
            weather.GetWindMagnitude().Set(0.0, smoothTime);
        }
        else
        {
            float wMin = 0.0;
            float wMax = 1.0;

            if (tWind > 0.7)
            {
                wMin = 0.85;
                wMax = 1.0;
            }
            else
            {
                wMin = tWind - 0.1;
                wMax = tWind + 0.1;
            }

            if (wMin < 0.1)
            {
                wMin = 0.1;
            }

            if (wMax > 1.0)
            {
                wMax = 1.0;
            }

            weather.GetWindMagnitude().SetLimits(wMin, wMax);
            weather.GetWindMagnitude().Set(tWind, smoothTime);
        }

        // 3. Chuva
        weather.GetRain().SetLimits(0.0, 1.0);

        if (isRaining)
        {
            // Se for Inverno (Index 3), fora cu nublado total para neve
            if (m_Data.CurrentSeasonIndex == 3)
            {
                weather.GetOvercast().Set(1.0, smoothTime);
            }

            if (tRainMin > tRainMax)
            {
                tRainMin = tRainMax;
            }

            weather.GetRain().Set(Math.RandomFloat(tRainMin, tRainMax), smoothTime);
        }
        else
        {
            weather.GetRain().Set(0.0, smoothTime);
        }

        // 4. Neblina
        weather.GetFog().SetLimits(0.0, 1.0);

        if (dice < tFog)
        {
            if (tFogMin > tFogMax)
            {
                tFogMin = tFogMax;
            }
            weather.GetFog().Set(Math.RandomFloat(tFogMin, tFogMax), smoothTime);
        }
        else
        {
            weather.GetFog().Set(0.0, smoothTime);
        }
    }

    // --- GETTERS (Expandidos para evitar one-liners) ---
    float GetCurrentBaseTemp()
    {
        return GetInterpolatedValue("BaseAirTemp");
    }
    float GetTempVariance()
    {
        return GetInterpolatedValue("TempVariance");
    }
    float GetWaterMultiplier()
    {
        return GetInterpolatedValue("Water");
    }
    float GetEnergyMultiplier()
    {
        return GetInterpolatedValue("Energy");
    }
    float GetFoodDecayMultiplier()
    {
        return GetInterpolatedValue("Food");
    }
    float GetItemDryingMultiplier()
    {
        return GetInterpolatedValue("Drying");
    }
    float GetStaminaRecoveryMultiplier()
    {
        return GetInterpolatedValue("Stamina");
    }
    float GetSicknessChance()
    {
        return GetInterpolatedValue("Sickness");
    }

    float GetInterpolatedValue(string type)
    {
        float val = 0.0;
        SDN_SeasonSettings curr;
        if (GetGame().IsServer())
        {
            curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
        }
        else if (m_ClientCurrentSettings)
        {
            curr = m_ClientCurrentSettings;
        }
        else
        {
            return 0.0;
        }

        if (!curr)
        {
            return 0.0;
        }

        // Mapeamento de valores atuais
        if (type == "BaseAirTemp")
        {
            val = curr.BaseAirTemp;
        }
        else if (type == "TempVariance")
        {
            val = curr.TempVariance;
        }
        else if (type == "Water")
        {
            val = curr.WaterDepletionMult;
        }
        else if (type == "Energy")
        {
            val = curr.EnergyDepletionMult;
        }
        else if (type == "Food")
        {
            val = curr.FoodDecayMult;
        }
        else if (type == "Drying")
        {
            val = curr.ItemDryingMult;
        }
        else if (type == "Stamina")
        {
            val = curr.StaminaRecoveryMult;
        }
        else if (type == "Sickness")
        {
            val = curr.SicknessChance;
        }
        else if (type == "OvercastMin")
        {
            val = curr.OvercastMin;
        }
        else if (type == "OvercastMax")
        {
            val = curr.OvercastMax;
        }
        else if (type == "WindLevel")
        {
            val = curr.WindLevel;
        }
        else if (type == "RainChance")
        {
            val = curr.RainChance;
        }
        else if (type == "FogChance")
        {
            val = curr.FogChance;
        }
        else if (type == "RainIntensityMin")
        {
            val = curr.RainIntensityMin;
        }
        else if (type == "RainIntensityMax")
        {
            val = curr.RainIntensityMax;
        }
        else if (type == "FogIntensityMin")
        {
            val = curr.FogIntensityMin;
        }
        else if (type == "FogIntensityMax")
        {
            val = curr.FogIntensityMax;
        }
        else if (type == "SmoothTime")
        {
            val = curr.SmoothTime;
        }

        // Lgica de Interpolao (Se estivermos no perodo de transio)
        if (GetGame().IsServer())
        {
            float lerp = GetTransitionFactor();

            if (lerp > 0.01)
            {
                SDN_SeasonSettings next = GetNextSeasonSettings();
                float nextVal = val;

                // Mapeamento de valores futuros (Expandido)
                if (type == "BaseAirTemp")
                {
                    nextVal = next.BaseAirTemp;
                }
                else if (type == "TempVariance")
                {
                    nextVal = next.TempVariance;
                }
                else if (type == "Water")
                {
                    nextVal = next.WaterDepletionMult;
                }
                else if (type == "Energy")
                {
                    nextVal = next.EnergyDepletionMult;
                }
                else if (type == "Food")
                {
                    nextVal = next.FoodDecayMult;
                }
                else if (type == "Drying")
                {
                    nextVal = next.ItemDryingMult;
                }
                else if (type == "Stamina")
                {
                    nextVal = next.StaminaRecoveryMult;
                }
                else if (type == "Sickness")
                {
                    nextVal = next.SicknessChance;
                }
                else if (type == "OvercastMin")
                {
                    nextVal = next.OvercastMin;
                }
                else if (type == "OvercastMax")
                {
                    nextVal = next.OvercastMax;
                }
                else if (type == "WindLevel")
                {
                    nextVal = next.WindLevel;
                }
                else if (type == "RainChance")
                {
                    nextVal = next.RainChance;
                }
                else if (type == "FogChance")
                {
                    nextVal = next.FogChance;
                }
                else if (type == "RainIntensityMin")
                {
                    nextVal = next.RainIntensityMin;
                }
                else if (type == "RainIntensityMax")
                {
                    nextVal = next.RainIntensityMax;
                }
                else if (type == "FogIntensityMin")
                {
                    nextVal = next.FogIntensityMin;
                }
                else if (type == "FogIntensityMax")
                {
                    nextVal = next.FogIntensityMax;
                }
                else if (type == "SmoothTime")
                {
                    nextVal = next.SmoothTime;
                }

                // Calcula o valor intermediario
                val = Math.Lerp(val, nextVal, lerp);
            }
        }
        return val;
    }

    string GetCurrentSeasonName()
    {
        if (GetGame().IsServer())
        {
            if (m_Config)
            {
                if (m_Config.Seasons)
                {
                    if (m_Data)
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
        if (!FileExist(SDN_Consts.MODS_DIR))
        {
            MakeDirectory(SDN_Consts.MODS_DIR);
        }

        if (!FileExist(SDN_Consts.BASE_DIR))
        {
            MakeDirectory(SDN_Consts.BASE_DIR);
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

        // Bloqueio de Sincronizao se no estiver licenciado
        if (!m_IsLicensed)
        {
            return;
        }

        PlayerBase targetPlayer = GetPlayerByIdentity(identity);

        if (targetPlayer)
        {
            SDN_SeasonSettings current = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
            ScriptRPC rpc = new ScriptRPC();
            rpc.Write(m_Data.CurrentSeasonIndex);
            rpc.Write(current);
            rpc.Write(m_Data.SeasonStartTimestamp);
            rpc.Write(m_Config.SeasonDurationMinutes);
            rpc.Send(targetPlayer, RPC_SYNC_SEASON_DATA, true, identity);
        }
    }

    void SyncToAllClients()
    {
        // Bloqueio de Sincronizao
        if (!m_IsLicensed)
        {
            return;
        }

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
            int idx;
            if (!ctx.Read(idx))
            {
                return;
            }
            SDN_SeasonSettings settings;
            if (!ctx.Read(settings))
            {
                return;
            }
            int startTime;
            if (!ctx.Read(startTime))
            {
                return;
            }
            int duration;
            if (!ctx.Read(duration))
            {
                return;
            }
            m_ClientCurrentSeasonIndex = idx;
            m_ClientCurrentSettings = settings;
            m_ClientStartTimestamp = startTime;
            m_ClientDurationMinutes = duration;
        }
        else if (rpc_type == RPC_SEND_MESSAGE)
        {
            string msg;
            if (!ctx.Read(msg))
            {
                return;
            }
            ChatMessageEventParams chatParams = new ChatMessageEventParams(CCSystem, "SDN System", msg, "");
            GetGame().GetMission().OnEvent(ChatMessageEventTypeID, chatParams);
        }
        else if (rpc_type == RPC_PLAY_SOUND)
        {
            string soundFile;
            if (!ctx.Read(soundFile))
            {
                return;
            }
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

        // Se a licena no for vlida, permite todos os animais (padro vanilla)
        if (!m_IsLicensed)
        {
            return true;
        }

        SDN_SeasonSettings curr = m_Config.Seasons.Get(m_Data.CurrentSeasonIndex);
        if (!curr.AllowedAnimals)
        {
            return true;
        }

        if (curr.AllowedAnimals.Count() == 0)
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