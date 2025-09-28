/****************************************************************************
 * HP/MP Training System - Skill-based character development
 ****************************************************************************/

#include "mud.h"

/* Physical skills that build HP */
static const char *physical_skills[] = {
    /* Hand-to-Hand Combat */
    "punch", "kick", "knee", "elbow", "headbutt", "bite",
    "grapple", "gouge", "slice", "bashdoor",
    
    /* Weapon Skills */
    "archery", "blowguns", "boomerangs", "bows", "crossbows",
    "firearms", "flexible arms", "long blades", "pugilism", 
    "short blades", "spears", "staves", "swords", "talonous arms",
    "missle weapons", "whips",
    
    /* Combat Techniques */
    "bash", "berserk", "disarm", "dual wield", "rescue", "trip",
    "first attack", "second attack", "third attack", "fourth attack", "fifth attack",
    
    /* Combat Styles */
    "aggressive style", "defensive style", "evasive style", "standard style",
    
    /* Physical Defense */
    "dodge", "parry", "grip", "enhanced damage",
    
    /* Physical Labor & Survival */
    "dig", "climb", "cook", "scan", "search", "detrap",
    
    /* Athletics & Endurance */
    "swim", "jump", "track", "hunt",
    
    NULL
};

/* Mental skills that build mana */
static const char *mental_skills[] = {
    /* Combat Magic */
    "fireball", "lightning bolt", "energy blast", "curse", "sleep",
    
    /* Transformation Magic */
    "aegis", "excalibur", "ragnarok", "titan", "valkyrie", "olympus",
    
    /* Mental Focus */
    "meditate", "focus", "concentration", "willpower", "lore", "fishing",
    
    /* Stealth & Mental Skills */
    "sneak", "hide", "steal", "lockpick", "peek",
    
    /* Magical Creation */
    "scribe", "enchant", "brew", "mix",
    
    /* Mental Knowledge */
    "identify", "appraise", "research", "study",
    
    /* Mental Strategy */
    "tactics", "leadership", "diplomacy", "persuasion",
    
    NULL
};

/* Check if a skill builds physical conditioning (HP) */
bool is_physical_skill(int skill_num) {
    const char *skill_name;
    
    if(skill_num < 0 || skill_num >= num_skills || !skill_table[skill_num])
        return FALSE;
        
    skill_name = skill_table[skill_num]->name;
    
    for(int i = 0; physical_skills[i] != NULL; i++) {
        if(!str_cmp(skill_name, physical_skills[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Check if a skill builds mental capacity (Mana) */
bool is_mental_skill(int skill_num) {
    const char *skill_name;
    
    if(skill_num < 0 || skill_num >= num_skills || !skill_table[skill_num])
        return FALSE;
        
    skill_name = skill_table[skill_num]->name;
    
    for(int i = 0; mental_skills[i] != NULL; i++) {
        if(!str_cmp(skill_name, mental_skills[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Calculate meter gain based on skill level achieved */
int get_physical_meter_gain(int skill_level) {
    if(skill_level < 25) return 2;      /* 2% per gain (very slow start) */
    if(skill_level < 50) return 4;      /* 4% per gain */
    if(skill_level < 65) return 6;      /* 6% per gain */
    if(skill_level < 75) return 8;      /* 8% per gain */
    if(skill_level < 85) return 10;     /* 10% per gain */
    if(skill_level < 95) return 25;     /* 25% per gain */
    return 45;                          /* 45% per gain (mastery) */
}

int get_mental_meter_gain(int skill_level) {
    if(skill_level < 25) return 3;      /* 3% per gain (slightly faster) */
    if(skill_level < 50) return 5;      /* 5% per gain */
    if(skill_level < 65) return 7;      /* 7% per gain */
    if(skill_level < 75) return 9;      /* 9% per gain */
    if(skill_level < 85) return 11;     /* 11% per gain */
    if(skill_level < 95) return 17;     /* 17% per gain */
    return 33;                          /* 33% per gain (mastery) */
}

/* Main function to update skill meters when skills are learned */
void update_skill_meters(CHAR_DATA *ch, int skill_num, int old_level, int new_level) {
    if(IS_NPC(ch) || !ch->pcdata || new_level <= old_level) 
        return;
    
    /* Process each skill point gained individually */
    for(int level = old_level + 1; level <= new_level; level++) {
        
        /* Check if physical skill */
        if(is_physical_skill(skill_num)) {
            int meter_gain = get_physical_meter_gain(level);
            ch->pcdata->physical_skill_meter += meter_gain;
            
            /* Check if meter filled (100% = 100 points) */
            while(ch->pcdata->physical_skill_meter >= 100) {
                /* Award HP and reset meter */
                int hp_bonus = number_range(6, 12); /* 6-12 HP */
                ch->max_hit += hp_bonus;
                ch->hit += hp_bonus; /* Also heal them */
                
                ch_printf(ch, "&RYour physical conditioning improves! You gain %d hit points!&x\r\n", hp_bonus);
                
                /* Reset meter, keep overflow */
                ch->pcdata->physical_skill_meter -= 100;
                save_char_obj(ch);
            }
        }
        
        /* Check if mental skill */
        else if(is_mental_skill(skill_num)) {
            int meter_gain = get_mental_meter_gain(level);
            ch->pcdata->mental_skill_meter += meter_gain;
            
            /* Check if meter filled */
            while(ch->pcdata->mental_skill_meter >= 100) {
                /* Award Mana and reset meter */
                int mana_bonus = number_range(2, 8); /* 2-8 mana */
                ch->max_mana += mana_bonus;
                ch->mana += mana_bonus; /* Also restore mana */
                
                ch_printf(ch, "&BYour mental focus deepens! You gain %d mana points!&x\r\n", mana_bonus);
                
                /* Reset meter, keep overflow */
                ch->pcdata->mental_skill_meter -= 100;
                save_char_obj(ch);
            }
        }
        
        /* Note: Language and utility skills give no meter progress */
    }
}


