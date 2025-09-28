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
                int hp_bonus = 10 + (ch->pcdata->physical_skill_meter / 50); /* 10-12 HP */
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
                int mana_bonus = 5 + (ch->pcdata->mental_skill_meter / 50); /* 5-7 mana */
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


void do_skillmeter(CHAR_DATA *ch, const char *argument) {
    CHAR_DATA *victim = NULL;

    if (IS_NPC(ch))
        return;

    if (argument[0] != '\0') {
        if (get_trust(ch) < LEVEL_IMMORTAL) {
            send_to_char("This command does not take arguments.\r\n", ch);
            return;
        }

        victim = get_char_world(ch, argument);
        if (!victim) {
            send_to_char("They aren't here.\r\n", ch);
            return;
        }
        if (IS_NPC(victim)) {
            send_to_char("NPCs don't have skill meters.\r\n", ch);
            return;
        }
    } else {
        victim = ch;
    }

    if (!victim->pcdata) {
        send_to_char("No skill data available.\r\n", ch);
        return;
    }

    recalc_skill_totals(victim);

    int total = victim->pcdata->skill_total_tenths;
    int cap = victim->pcdata->skill_cap_tenths <= 0 ? DEFAULT_SKILL_CAP_TENTHS : victim->pcdata->skill_cap_tenths;
    int free_cap = UMAX(0, cap - total);
    int percent = cap > 0 ? (total * 100) / cap : 0;
    int pending = total - victim->pcdata->skill_gain_pool;
    if (pending < 0)
        pending = 0;

    int physical = victim->pcdata->physical_skill_meter;
    int mental = victim->pcdata->mental_skill_meter;

    if (victim != ch)
        ch_printf(ch, "&W--- %s's Skill Usage Overview ---&D\r\n", victim->name);
    else
        send_to_char("&W--- Skill Usage Overview ---&D\r\n", ch);

    ch_printf(ch, "&WTotal Skill Points:&D %6.1f / %6.1f (%d%% of cap)\r\n",
              total / 10.0, cap / 10.0, percent);
    ch_printf(ch, "&WFree Capacity:&D    %6.1f\r\n", free_cap / 10.0);
    ch_printf(ch, "&WPending Gains:&D   %6.1f (waiting for room in your cap)\r\n", pending / 10.0);
    ch_printf(ch, "&WSkill Gain Pool:&D %6.1f\r\n", victim->pcdata->skill_gain_pool / 10.0);

    ch_printf(ch, "&WPhysical Meter:&D   %3d%% &g(%d%% to next HP bonus)&D\r\n", physical, UMAX(0, 100 - physical));
    ch_printf(ch, "&WMental Meter:&D     %3d%% &g(%d%% to next mana bonus)&D\r\n", mental, UMAX(0, 100 - mental));

    if (victim == ch)
        send_to_char("&WHint:&D Use '&WSKILL&D' to review locks and skill caps.\r\n", ch);
}

