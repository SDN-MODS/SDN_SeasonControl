// ============================================================================
// PASTA: 5_Mission
// ARQUIVO: SDN_MissionServer.c
// EXECUÇÃO: Server
// DESCRIÇÃO: Comandos Admin e Hooks de Conexão.
// ATUALIZAÇÃO: Código totalmente expandido (Zero One-Liners).
// ============================================================================

modded class MissionServer
{
    private ref array<string> m_SDN_Admins;

    private const string SDN_BASE_DIR = "$profile:SDN_MODS";
    private const string SDN_ADMIN_DIR = SDN_BASE_DIR + "/SDN_SeasonControl";
    private const string SDN_ADMIN_FILE = SDN_ADMIN_DIR + "/Admins.txt";

    override void OnInit()
    {
        super.OnInit();

        m_SDN_Admins = new array<string>;
        LoadAdmins();
        
        Print("[SDN] Inicializando Mod...");
        
        // Garante a inicialização do Manager
        if (SDN_SeasonManager.GetInstance())
        {
            SDN_SeasonManager.GetInstance().Init();
        }
    }

    void LoadAdmins()
    {
        // Garante pasta base
        if (!FileExist(SDN_BASE_DIR))
        {
            MakeDirectory(SDN_BASE_DIR);
        }

        // Garante subpasta
        if (!FileExist(SDN_ADMIN_DIR)) 
        {
            MakeDirectory(SDN_ADMIN_DIR);
        }

        // Cria o arquivo se não existir
        if (!FileExist(SDN_ADMIN_FILE)) 
        { 
            FileHandle file = OpenFile(SDN_ADMIN_FILE, FileMode.WRITE); 
            if (file != 0) 
            { 
                FPrintln(file, "// Adicione GUIDs ou SteamIDs (Um por linha)"); 
                CloseFile(file); 
            } 
            return; 
        }
        
        // Lê os admins
        FileHandle f = OpenFile(SDN_ADMIN_FILE, FileMode.READ);
        if (f != 0) 
        { 
            string line; 
            while (FGets(f, line) > 0) 
            { 
                line.Trim(); 

                // Ignora linhas vazias e comentários
                if (line.Length() > 0 && line[0] != '/')
                {
                    m_SDN_Admins.Insert(line);
                }
            } 
            CloseFile(f); 
        }
    }
}

    bool IsSDNAdmin(PlayerIdentity identity)
    {
        if (!identity) 
        {
            return false;
        }
        
        // Se a lista estiver vazia e for ambiente Debug/Offline, permite acesso
        if (m_SDN_Admins.Count() == 0 && GetGame().IsDebug()) 
        {
            return true;
        }
        
        // Verifica ID e Steam64 ID
        if (m_SDN_Admins.Find(identity.GetId()) != -1)
        {
            return true;
        }
        
        if (m_SDN_Admins.Find(identity.GetPlainId()) != -1)
        {
            return true;
        }
        
        return false;
    }

    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);
        
        if (eventTypeId != ChatMessageEventTypeID) 
        {
            return;
        }
        
        ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
        if (!chatParams) 
        {
            return;
        }
        
        string message = chatParams.param3;
        
        // Verifica se é um comando (inicia com !)
        if (message.IndexOf("!") == 0) 
        { 
            // Busca a identidade pelo nome (param2), pois param1 é int no chat event
            string playerName = chatParams.param2; 
            PlayerIdentity sender = GetPlayerIdentityByName(playerName); 
            
            if (sender) 
            {
                ProcessCommand(sender, message); 
            }
        }
    }

    PlayerIdentity GetPlayerIdentityByName(string name)
    {
        array<Man> players = new array<Man>; 
        GetGame().GetPlayers(players);
        
        foreach(Man p : players) 
        { 
            if (p.GetIdentity())
            {
                if (p.GetIdentity().GetName() == name) 
                {
                    return p.GetIdentity(); 
                }
            }
        }
        return null;
    }

    void ProcessCommand(PlayerIdentity sender, string commandLine)
    {
        if (!IsSDNAdmin(sender)) 
        {
            return;
        }
        
        TStringArray args = new TStringArray; 
        commandLine.Split(" ", args);
        
        if (args.Count() == 0)
        {
            return;
        }
        
        string cmd = args[0]; 
        cmd.ToLower(); 
        
        if (cmd == "!season")
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            if (!manager)
            {
                return;
            }
            
            if (args.Count() < 2) 
            { 
                manager.SendPrivateMessage(sender, "Uso: !season <status|next|set|reload>"); 
                return; 
            }
            
            string action = args[1]; 
            action.ToLower();
            
            if (action == "status") 
            {
                // Envia relatório com temperatura real
                manager.SendStatusReport(sender);
            }
            else if (action == "next") 
            { 
                manager.AdvanceSeason(); 
                manager.SendPrivateMessage(sender, "Estacao avancada manualmente."); 
            }
            else if (action == "reload") 
            { 
                manager.LoadConfig(); 
                manager.ApplyWeather(true); 
                manager.SendPrivateMessage(sender, "Config recarregada."); 
            }
            else if (action == "set")
            {
                if (args.Count() < 3) 
                {
                    manager.SendPrivateMessage(sender, "Uso: !season set <estacao>");
                    return;
                }
                
                string targetSeason = args[2]; 
                targetSeason.ToLower();
                
                int newIndex = -1;
                if (targetSeason == "spring") newIndex = 0; 
                if (targetSeason == "summer") newIndex = 1; 
                if (targetSeason == "autumn") newIndex = 2; 
                if (targetSeason == "winter") newIndex = 3;
                
                if (newIndex != -1) 
                { 
                    manager.ForceSeasonIndex(newIndex); 
                    manager.SendPrivateMessage(sender, "Estacao definida: " + targetSeason); 
                }
                else
                {
                    manager.SendPrivateMessage(sender, "Estacao invalida. Use: spring, summer, autumn, winter");
                }
            }
        }
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        
        if (identity) 
        {
            // Sincroniza dados iniciais (Delay curto 2s) para garantir que o cliente receba
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SyncSeasonToPlayer, 2000, false, identity);
            
            // Agenda a Notificação de Boas Vindas
            // Usa CallLater para chamar a função ScheduleWelcome, que vai ler a config de tempo
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ScheduleWelcome, 5000, false, identity);
        }
    }
    
    void SyncSeasonToPlayer(PlayerIdentity identity) 
    { 
        if (SDN_SeasonManager.GetInstance()) 
        {
            SDN_SeasonManager.GetInstance().SyncToPlayer(identity); 
        }
    }

    void ScheduleWelcome(PlayerIdentity identity)
    {
        SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
        if (manager)
        {
            // Agenda a notificação real baseada no tempo do JSON
            manager.ScheduleWelcomeNotification(identity);
        }
    }
}