// ============================================================================
// PASTA: 3_Game
// ARQUIVO: SDN_DataClasses.c
// DESCRIÇÃO: Configuração Mestre (Refinado).
// ATUALIZAÇÃO: JSON Limpo (Icone, Som e Cor removidos da config visível).
// ============================================================================

// Nova Classe para definir uma Notificação (Simplificada)
class SDN_NotificationEntry
{
    string Title;
    string Text;
    float Duration;

    // Construtor simplificado (Visual padronizado no código)
    void SDN_NotificationEntry(string title, string text, float duration)
    {
        Title = title;
        Text = text;
        Duration = duration;
    }
}

class SDN_SeasonSettings
{
    string SeasonName;              
    float BaseAirTemp;              
    float TempVariance;             
    
    // Metabolismo
    float WaterDepletionMult;       
    float EnergyDepletionMult;      
    
    // Clima Avançado
    float RainChance;
    float RainIntensityMin; 
    float RainIntensityMax;
    
    float FogChance;
    float FogIntensityMin;  
    float FogIntensityMax;
    
    float OvercastMin;
    float OvercastMax;
    float WindLevel;                
    
    float SmoothTime; 

    // Sobrevivência
    float FoodDecayMult;        
    float ItemDryingMult;       
    float StaminaRecoveryMult;  
    float SicknessChance;       
    
    int ForceFullMoon;          
    int SeasonMonth; 
    
    // Fauna
    ref array<string> AllowedAnimals;

    // Notificações da Estação
    ref array<ref SDN_NotificationEntry> SeasonNotifications;

    void SDN_SeasonSettings(string name)
    {
        SeasonName = name;
        BaseAirTemp = 20.0;
        TempVariance = 5.0;
        WaterDepletionMult = 1.0;
        EnergyDepletionMult = 1.0;
        
        RainChance = 0.0;
        RainIntensityMin = 0.4;
        RainIntensityMax = 0.8;
        
        FogChance = 0.0;
        FogIntensityMin = 0.1;
        FogIntensityMax = 0.3;
        
        OvercastMin = 0.0;
        OvercastMax = 1.0;
        WindLevel = 0.1;
        SmoothTime = 300.0; 
        
        FoodDecayMult = 1.0;
        ItemDryingMult = 1.0;
        StaminaRecoveryMult = 1.0;
        SicknessChance = 0.1;
        ForceFullMoon = -1;
        SeasonMonth = 7;
        
        AllowedAnimals = new array<string>;
        SeasonNotifications = new array<ref SDN_NotificationEntry>;
    }
}

class SDN_SeasonConfig
{
    bool EnableDebugLogs;
    int SeasonDurationMinutes;
    float TransitionPercent;
    
    // Configurações Globais de Notificação
    float JoinNotificationDelay;  
    float NotificationInterval;   
    ref SDN_NotificationEntry WelcomeNotification; 
    
    ref array<ref SDN_SeasonSettings> Seasons;

    void SDN_SeasonConfig()
    {
        EnableDebugLogs = true;
        SeasonDurationMinutes = 1440; 
        TransitionPercent = 0.2;
        
        JoinNotificationDelay = 180.0; 
        NotificationInterval = 600.0;

        // Visual (Icone/Som/Cor) agora é fixo no código. Aqui definimos apenas texto.
        WelcomeNotification = new SDN_NotificationEntry("BEM VINDO", "Este servidor usa SDN Season Control.", 5.0);
        
        Seasons = new array<ref SDN_SeasonSettings>;

        SDN_SeasonSettings s;

        // --- PRIMAVERA ---
        s = new SDN_SeasonSettings("Spring");
        s.BaseAirTemp = 18.0;       
        s.TempVariance = 8.0;
        s.WaterDepletionMult = 1.0; 
        s.EnergyDepletionMult = 1.0;
        s.RainChance = 0.3;
        s.RainIntensityMin = 0.2;
        s.RainIntensityMax = 0.5;
        s.FogChance = 0.1; 
        s.FogIntensityMin = 0.05;
        s.FogIntensityMax = 0.2;
        s.OvercastMin = 0.0; 
        s.OvercastMax = 0.5; 
        s.WindLevel = 0.3;
        s.SmoothTime = 300.0;
        s.FoodDecayMult = 1.5; 
        s.ItemDryingMult = 0.8; 
        s.StaminaRecoveryMult = 1.0; 
        s.SicknessChance = 0.1; 
        s.ForceFullMoon = -1;
        s.SeasonMonth = 5;
        s.AllowedAnimals.Insert("Animal_BosTaurus"); 
        s.AllowedAnimals.Insert("Animal_SusDomesticus");
        s.AllowedAnimals.Insert("Animal_CervusElaphus");
        
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("PRIMAVERA", "O clima esta agradavel, aproveite para plantar.", 5.0));
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("DICA", "As chuvas sao frequentes, cuide da sua plantacao.", 4.0));
        
        Seasons.Insert(s);
        
        // --- VERÃO ---
        s = new SDN_SeasonSettings("Summer");
        s.BaseAirTemp = 42.0;       
        s.TempVariance = 10.0; 
        s.WaterDepletionMult = 2.5; 
        s.EnergyDepletionMult = 0.8;
        s.RainChance = 0.1;
        s.RainIntensityMin = 0.8;
        s.RainIntensityMax = 1.0;
        s.FogChance = 0.0; 
        s.FogIntensityMin = 0.0;
        s.FogIntensityMax = 0.0;
        s.OvercastMin = 0.0; 
        s.OvercastMax = 0.1;        
        s.WindLevel = 0.0;
        s.SmoothTime = 600.0;
        s.FoodDecayMult = 5.0;      
        s.ItemDryingMult = 4.0; 
        s.StaminaRecoveryMult = 0.7; 
        s.SicknessChance = 0.05; 
        s.ForceFullMoon = 1;
        s.SeasonMonth = 7;
        s.AllowedAnimals.Insert("Animal_SusScrofa");
        s.AllowedAnimals.Insert("Animal_UrsusArctos");
        
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("VERAO", "Calor extremo! Beba muita agua.", 5.0));
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("ALERTA", "Comida apodrece rapidamente no calor.", 4.0));
        
        Seasons.Insert(s);
        
        // --- OUTONO ---
        s = new SDN_SeasonSettings("Autumn");
        s.BaseAirTemp = 8.0;        
        s.TempVariance = 5.0;
        s.WaterDepletionMult = 1.0; 
        s.EnergyDepletionMult = 1.2; 
        s.RainChance = 0.5; 
        s.RainIntensityMin = 0.3;
        s.RainIntensityMax = 0.6;
        s.FogChance = 0.8;          
        s.FogIntensityMin = 0.2;
        s.FogIntensityMax = 0.5;
        s.OvercastMin = 0.5; 
        s.OvercastMax = 0.8; 
        s.WindLevel = 0.6; 
        s.SmoothTime = 240.0;
        s.FoodDecayMult = 1.0; 
        s.ItemDryingMult = 0.7; 
        s.StaminaRecoveryMult = 1.0; 
        s.SicknessChance = 0.3; 
        s.ForceFullMoon = 0;
        s.SeasonMonth = 10;
        s.AllowedAnimals.Insert("Animal_CanisLupus_Grey"); 
        s.AllowedAnimals.Insert("Animal_VulpesVulpes"); 
        
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("OUTONO-DICAS", "O vento esta forte e o frio comeca a chegar.", 5.0));
		s.SeasonNotifications.Insert(new SDN_NotificationEntry("OUTONO-DICAS", "O vento esta forte e o frio comeca a chegar FOME SERA UM PROBLEMA.", 5.0));
        
        Seasons.Insert(s);
        
        // --- INVERNO ---
        s = new SDN_SeasonSettings("Winter");
        s.BaseAirTemp = -15.0;      
        s.TempVariance = 5.0;
        s.WaterDepletionMult = 0.8; 
        s.EnergyDepletionMult = 2.0; 
        s.RainChance = 0.4; 
        s.RainIntensityMin = 0.1;
        s.RainIntensityMax = 0.4;
        s.FogChance = 0.6; 
        s.FogIntensityMin = 0.4;
        s.FogIntensityMax = 0.8;
        s.OvercastMin = 0.8; 
        s.OvercastMax = 1.0; 
        s.WindLevel = 1.0; 
        s.SmoothTime = 180.0;
        s.FoodDecayMult = 0.1; 
        s.ItemDryingMult = 0.2; 
        s.StaminaRecoveryMult = 0.8; 
        s.SicknessChance = 0.6; 
        s.ForceFullMoon = 0;
        s.SeasonMonth = 12;
        s.AllowedAnimals.Insert("Animal_CanisLupus_Grey");
        s.AllowedAnimals.Insert("Animal_UrsusArctos");
        
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("INVERNO", "Risco de hipotermia! Mantenha-se aquecido.", 5.0));
        s.SeasonNotifications.Insert(new SDN_NotificationEntry("SOBREVIVENCIA", "Comida congelada dura mais tempo.", 4.0));
        
        Seasons.Insert(s);
    }
}

class SDN_SeasonSaveData
{
    int CurrentSeasonIndex;
    int SeasonStartTimestamp;

    void SDN_SeasonSaveData()
    {
        CurrentSeasonIndex = 0;
        SeasonStartTimestamp = 0;
    }
}