// ============================================================================
// PASTA: 4_World/SDN_Entities
// ARQUIVO: SDN_PlayerBase.c
// DESCRIÇÃO: Player Base Passivo (Reage ao Ambiente Real).
// CORREÇÃO: Removida a adição artificial de umidade nas roupas.
// ============================================================================

modded class PlayerBase
{
    protected float m_SDN_SurvivalTimer;
    protected float m_SDN_LastWaterLevel; 
    protected bool m_SDN_IsDizzy;

    override void Init()
    {
        super.Init();
        m_SDN_LastWaterLevel = -99999; 
        m_SDN_IsDizzy = false;
    }

    override void OnScheduledTick(float deltaTime)
    {
        super.OnScheduledTick(deltaTime);
        
        if (GetGame().IsClient() && IsControlledPlayer())
        {
            HandleClientVisuals();
        }

        if (GetGame().IsServer())
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            if (manager)
            {
                HandleHydrationDetection(manager); 
                HandleMetabolism(manager, deltaTime);
                HandlePhysiology(manager, deltaTime);

                m_SDN_SurvivalTimer += deltaTime;
                if (m_SDN_SurvivalTimer >= 2.0)
                {
                    HandleClothingPhysics(manager);
                    m_SDN_SurvivalTimer = 0;
                }
            }
        }
    }

    void HandleHydrationDetection(SDN_SeasonManager manager)
    {
        float currentWater = GetStatWater().Get();
        
        if (m_SDN_LastWaterLevel == -99999) 
        { 
            m_SDN_LastWaterLevel = currentWater; 
            return; 
        }
        
        m_SDN_LastWaterLevel = currentWater;
    }

    void HandleClientVisuals()
    {
        float heat = GetStatHeatComfort().Get();
        
        if (heat > 1.2) 
        {
            PPEffects.SetBlurFever(0.6); 
        }
        else if (heat > 0.6) 
        {
            PPEffects.SetBlurFever(0.2);
        }
        else 
        {
            PPEffects.SetBlurFever(0.0);
        }
    }

    void HandleMetabolism(SDN_SeasonManager manager, float deltaTime)
    {
        float loss = 0.0;
        float waterMult = manager.GetWaterMultiplier();
        
        if (waterMult != 1.0) 
        { 
            loss = (waterMult - 1.0) * 0.01 * deltaTime; 
            
            if (loss > 0) 
            {
                GetStatWater().Add(-loss); 
            }
        }
        
        float energyMult = manager.GetEnergyMultiplier();
        
        if (energyMult != 1.0) 
        { 
            loss = (energyMult - 1.0) * 0.01 * deltaTime; 
            
            if (loss > 0) 
            {
                GetStatEnergy().Add(-loss); 
            }
        }
    }

    void HandlePhysiology(SDN_SeasonManager manager, float deltaTime)
    {
        float heatComfort = GetStatHeatComfort().Get();
        
        // TONTURA (Hipertermia Real)
        if (heatComfort > 2.0)
        {
            if (!m_SDN_IsDizzy)
            {
                if (m_SymptomManager)
                {
                    m_SymptomManager.QueueUpSecondarySymptom(SymptomIDs.SYMPTOM_FEVERBLUR);
                    m_SDN_IsDizzy = true;
                }
            }
            
            AddHealth("GlobalHealth", "Health", -1.0 * deltaTime);
        }
        else if (heatComfort < 1.0)
        {
            if (m_SDN_IsDizzy)
            {
                if (m_SymptomManager) 
                {
                    m_SymptomManager.RemoveSecondarySymptom(SymptomIDs.SYMPTOM_FEVERBLUR);
                }
                m_SDN_IsDizzy = false;
            }
        }

        if (heatComfort < -0.8) 
        {
            AddHealth("GlobalHealth", "Health", -0.5 * deltaTime);
        }
    }

    void HandleClothingPhysics(SDN_SeasonManager manager)
    {
        float dryingMult = manager.GetItemDryingMultiplier();
        
        // Se a diferença for insignificante, não processamos
        if (Math.AbsFloat(dryingMult - 1.0) < 0.1) 
        {
            return;
        }
        
        int slots[] = {InventorySlots.HEADGEAR, InventorySlots.MASK, InventorySlots.BODY, InventorySlots.HIPS, InventorySlots.LEGS, InventorySlots.FEET, InventorySlots.GLOVES, InventorySlots.BACK};
        
        for (int i = 0; i < 8; i++)
        {
            ItemBase item = ItemBase.Cast(GetInventory().FindAttachment(slots[i]));
            
            if (item)
            {
                // CORREÇÃO: O script só atua se o item JÁ ESTIVER molhado.
                if (item.GetWet() > 0.01)
                {
                    // Apenas ACELERA a secagem no verão.
                    // No inverno (dryingMult < 1.0), não fazemos nada.
                    // Isso remove o "molhar mágico".
                    if (dryingMult > 1.0) 
                    {
                        item.AddWet(-(0.05 * dryingMult)); 
                    }
                }
            }
        }
    }

    override float GetImmunity()
    {
        float immunity = super.GetImmunity();
        
        if (GetGame().IsServer())
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            
            if (manager) 
            {
                immunity = immunity * (1.0 - (manager.GetSicknessChance() * 0.5));
            }
        }
        
        return immunity;
    }

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);
        
        if (rpc_type == 894712 || rpc_type == 894714)
        {
            SDN_SeasonManager manager = SDN_SeasonManager.GetInstance();
            
            if (manager) 
            {
                manager.OnRPC(sender, rpc_type, ctx);
            }
        }
    }
}