/**
 * ARQUIVO: config.cpp
 * LOCAL: Raiz do Mod (SDN_SeasonControl/config.cpp)
 * DESCRIÇÃO: Define a estrutura do PBO e DEPENDÊNCIA DO CF.
 */

class CfgPatches
{
	class SDN_SeasonControl
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		// ADICIONADO: "JM_CF_Scripts" para garantir acesso ao Community Framework
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "JM_CF_Scripts"};
	};
};

class CfgMods
{
	class SDN_SeasonControl
	{
		dir = "SDN_SeasonControl";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "SDN_SeasonControl";
		credits = "SeuNome";
		author = "SeuNome";
		authorID = "0"; 
		version = "1.0";
		extra = 0;
		type = "mod";
		
		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"SDN_SeasonControl/3_Game"};
			};

			class worldScriptModule
			{
				value = "";
				files[] = {"SDN_SeasonControl/4_World"};
			};

			class missionScriptModule
			{
				value = "";
				files[] = {"SDN_SeasonControl/5_Mission"};
			};
		};
	};
};