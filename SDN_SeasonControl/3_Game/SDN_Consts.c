// ============================================================================
// PASTA: 3_Game
// ARQUIVO: SDN_Consts.c
// DESCRIÇÃO: Enums e Constantes globais para o sistema de estações.
// ============================================================================

enum SDN_SeasonEnum
{
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3
}

enum ESDN_SeasonParam
{
    BASE_AIR_TEMP,
    TEMP_VARIANCE,
    WATER_DEPLETION,
    ENERGY_DEPLETION,
    FOOD_DECAY,
    ITEM_DRYING,
    STAMINA_RECOVERY,
    SICKNESS_CHANCE,
    OVERCAST_MIN,
    OVERCAST_MAX,
    WIND_LEVEL,
    RAIN_CHANCE,
    FOG_CHANCE,
    RAIN_INTENSITY_MIN,
    RAIN_INTENSITY_MAX,
    FOG_INTENSITY_MIN,
    FOG_INTENSITY_MAX,
    SMOOTH_TIME
}

class SDN_Consts
{
    // Caminhos de Arquivo
    static const string CONFIG_DIR = "$profile:SDN_SeasonControl";
    static const string CONFIG_FILE = "$profile:SDN_SeasonControl/SeasonConfig.json";
    static const string SAVE_FILE = "$profile:SDN_SeasonControl/SeasonState.json";

    // RPC IDs
    static const int RPC_SYNC_SEASON_DATA = 894710;
    static const int RPC_ADMIN_CMD_RES = 894711;
}