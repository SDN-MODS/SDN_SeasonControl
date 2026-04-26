enum SDN_SeasonEnum
{
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3
}

class SDN_Consts
{
    // Diretórios base
    static const string CONFIG_DIR = "$profile:SDN_MODS";
    static const string SEASON_DIR = CONFIG_DIR + "/SDN_SeasonControl";

    // Arquivos
    static const string CONFIG_FILE = SEASON_DIR + "/SeasonConfig.json";
    static const string SAVE_FILE   = SEASON_DIR + "/SeasonState.json";

    // RPC IDs
    static const int RPC_SYNC_SEASON_DATA = 894710;
    static const int RPC_ADMIN_CMD_RES    = 894711;
}
