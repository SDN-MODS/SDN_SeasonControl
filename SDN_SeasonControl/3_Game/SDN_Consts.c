// ============================================================================
// PASTA: 3_Game
// ARQUIVO: SDN_Consts.c
// CAMADA: 3_Game
// EXECUO: Shared
// DESCRIO:
// Definicoes de constantes globais, IDs de RPC e Enums para o sistema de estacoes.
// ============================================================================

enum SDN_SeasonEnum
{
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3
}

class SDN_Consts
{
    // Caminhos de Arquivo
    static const string MODS_DIR = "$profile:SDN_MODS";
    static const string BASE_DIR = "$profile:SDN_MODS/SDN_SeasonControl";
    static const string CONFIG_FILE = "$profile:SDN_MODS/SDN_SeasonControl/SeasonConfig.json";
    static const string SAVE_FILE = "$profile:SDN_MODS/SDN_SeasonControl/SeasonState.json";
    static const string LOG_DIR = "$profile:SDN_MODS/SDN_SeasonControl/Logs";
    static const string ADMIN_FILE = "$profile:SDN_MODS/SDN_SeasonControl/Admins.txt";

    // RPC IDs (Certifique-se que no conflitem com outros mods se no usar Frameworks de RPC automtico)
    static const int RPC_SYNC_SEASON_DATA = 894710;
    static const int RPC_ADMIN_CMD_RES = 894711;
}
