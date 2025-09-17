#include "mud.h"

/* Flavorful transformation data - simple arrays with epic descriptions */
static const long long pl_requirements[6] = {
    1400000LL,      /* Aegis */
    12500000LL,     /* Titan */
    100000000LL,    /* Valkyrie */
    2000000000LL,   /* Olympus */
    25000000000LL,  /* Ragnarok */
    125000000000LL  /* Excalibur */
};

static const char *protocol_names[6] = {
    "Aegis", "Titan", "Valkyrie", "Olympus", "Ragnarok", "Excalibur"
};

static const char *protocol_commands[6] = {
    "aegis", "titan", "valkyrie", "olympus", "ragnarok", "excalibur"
};

static const char *component_names[6] = {
    "nano processor", "quantum core", "plasma conduit",
    "neural matrix", "fusion cell", "ethereal core"
};

static const char *component_types[6] = {
    "Nano Processors", "Quantum Cores", "Plasma Conduits",
    "Neural Matrices", "Fusion Cells", "Ethereal Cores"
};

static const char *discovery_messages[6] = {
    "&C*** MEMORY CORE ACTIVATION ***&x\r\n"
    "&WAs your power level increases, dormant memory cores come online...&x\r\n"
    "&CAEGIS PROTOCOL DISCOVERED: Defensive enhancement systems detected.&x\r\n"
    "&YThis legendary protocol grants the power of divine protection.&x\r\n"
    "&WRequired components: 5 Nano Processors&x",
    
    "&C*** ANCIENT SUBROUTINES UNLOCKED ***&x\r\n"
    "&WDeep within your neural matrix, ancient code awakens...&x\r\n"
    "&CTITAN PROTOCOL DISCOVERED: Primordial strength amplification systems.&x\r\n"
    "&YThe might of the first giants flows through these schematics.&x\r\n"
    "&WRequired components: 5 Quantum Cores&x",
    
    "&C*** WARRIOR PROTOCOLS INITIALIZED ***&x\r\n"
    "&WYour combat systems resonate with legendary battle algorithms...&x\r\n"
    "&CVALKYRIE PROTOCOL DISCOVERED: Elite warrior enhancement matrix.&x\r\n"
    "&YThe chosen warriors of legend have blessed your code.&x\r\n"
    "&WRequired components: 5 Plasma Conduits&x",
    
    "&C*** DIVINE AUTHORIZATION GRANTED ***&x\r\n"
    "&WCosmic energies align with your core programming...&x\r\n"
    "&COLYMPUS PROTOCOL DISCOVERED: Divine authority management system.&x\r\n"
    "&YThe gods themselves have granted you access to ultimate power.&x\r\n"
    "&WRequired components: 5 Neural Matrices&x",
    
    "&C*** APOCALYPSE ALGORITHMS ACTIVATED ***&x\r\n"
    "&WThe twilight protocols stir within your deepest memory banks...&x\r\n"
    "&CRAGNAROK PROTOCOL DISCOVERED: Endgame transformation matrix.&x\r\n"
    "&YYou have become the harbinger of endings and new beginnings.&x\r\n"
    "&WRequired components: 5 Fusion Cells&x",
    
    "&C*** LEGENDARY CORE AWAKENING ***&x\r\n"
    "&WThe ultimate protocol resonates through every circuit...&x\r\n"
    "&CEXCALIBUR PROTOCOL DISCOVERED: The blade that shapes destiny.&x\r\n"
    "&YYou have attained the legendary power to forge fate itself.&x\r\n"
    "&WRequired components: 5 Ethereal Cores&x"
};

static const char *component_descriptions[6] = {
    "A microscopic processing unit gleams with technological potential.",
    "A quantum processing core hums with subatomic energy.",
    "A sophisticated energy conduit crackles with contained plasma.",
    "An advanced neural processing matrix pulses with artificial intelligence.",
    "An experimental fusion power cell radiates tremendous energy.",
    "A transcendent ethereal core shimmers with otherworldly power."
};

static const char *success_messages[6] = {
    "Divine protection algorithms integrate seamlessly with your core systems.",
    "Primordial might courses through your enhanced frame.",
    "The chosen warriors' honor burns within your core.",
    "Divine authority flows through every circuit and servo.",
    "The twilight of gods - you have become the harbinger of endings.",
    "You have become the blade that shapes destiny itself."
};

/* Android component system - one function does everything */
void check_android_components( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *android = NULL;
    CHAR_DATA *gch;
    long long victim_pl, android_pl;
    int drop_chance = 0;
    
    if( !IS_NPC(victim) || !ch || IS_NPC(ch) )
        return;
    
    /* Find an android who needs components */
    for( gch = ch->in_room->first_person; gch && !android; gch = gch->next_in_room )
    {
        if( IS_ANDROID(gch) && !IS_BIO_ANDROID(gch) && !IS_NPC(gch) && gch->pcdata )
        {
            /* Check if they need any components for unlocked schematics */
            for( int i = 0; i < 6; i++ )
            {
                if( IS_SET(gch->pcdata->android_schematics, (1 << i)) && 
                    gch->pcdata->android_components[i] < 5 )
                {
                    android = gch;
                    break;
                }
            }
        }
    }
    
    if( !android )
        return;
    
    /* Calculate drop chance - simple and direct */
    victim_pl = get_power_level(victim);
    android_pl = get_power_level(android);
    
    /* Anti-farming: victim must be at least 1/10 android's PL */
    if( victim_pl < (android_pl / 10) )
        return;
    
    /* Group vs solo rates */
    if( is_same_group(android, ch) )
    {
        drop_chance = 2;  /* 2% for group */
    }
    else if( !android->leader && android == ch )
    {
        drop_chance = 1;  /* 1% for solo, only if android killed the enemy */
    }
    
    /* Check for drop */
    if( drop_chance > 0 && number_percent() <= drop_chance )
    {
        /* Find component they need most */
        int best = -1, lowest = 6;
        
        for( int i = 0; i < 6; i++ )
        {
            if( IS_SET(android->pcdata->android_schematics, (1 << i)) && 
                android->pcdata->android_components[i] < lowest )
            {
                lowest = android->pcdata->android_components[i];
                best = i;
            }
        }
        
        if( best >= 0 )
        {
            android->pcdata->android_components[best]++;
            
            ch_printf( android, "&C[COMPONENT ACQUIRED] Your sensors detect %s in the wreckage!&x\r\n", 
                      component_names[best] );
            act( AT_CYAN, "$n's sensors scan the wreckage and extract a technological component.", 
                 android, NULL, NULL, TO_ROOM );
            act( AT_CYAN, component_descriptions[best], android, NULL, NULL, TO_CHAR );
            
            ch_printf( android, "&YProgress: %d/5 %s&x\r\n", 
                      android->pcdata->android_components[best], component_types[best] );
            
            /* Check if they can install now */
            if( android->pcdata->android_components[best] == 5 )
            {
                send_to_char( "&G*** INSTALLATION READY ***&x\r\n", android );
                ch_printf( android, "&WYou have collected all components for the %s Protocol!&x\r\n", 
                          protocol_names[best] );
                ch_printf( android, "&YUse 'install %s' to activate this transformation!&x\r\n", 
                          protocol_commands[best] );
            }
            
            save_char_obj( android );
        }
    }
}

/* Check android schematics with epic discovery messages */
void check_android_schematics( CHAR_DATA *ch )
{
    if( !ch || IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) || !ch->pcdata )
        return;
    
    long long pl = get_power_level(ch);
    
    for( int i = 0; i < 6; i++ )
    {
        if( pl >= pl_requirements[i] && !IS_SET(ch->pcdata->android_schematics, (1 << i)) )
        {
            SET_BIT(ch->pcdata->android_schematics, (1 << i));
            
            /* Epic discovery message */
            send_to_char( discovery_messages[i], ch );
            send_to_char( "\r\n\r\n", ch );
            
            act( AT_MAGIC, "$n's systems emit a soft humming as new protocols come online.", 
                 ch, NULL, NULL, TO_ROOM );
            
            save_char_obj( ch );
        }
    }
}

void do_ainstall( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    int protocol = -1;
    
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only androids can install protocols.\r\n", ch );
        return;
    }
    
    one_argument( argument, arg );
    
    if( !arg[0] )
    {
        send_to_char( "Install which protocol? (aegis, titan, valkyrie, olympus, ragnarok, excalibur)\r\n", ch );
        return;
    }
    
    /* Find protocol */
    for( int i = 0; i < 6; i++ )
    {
        if( !str_cmp( arg, protocol_commands[i] ) )
        {
            protocol = i;
            break;
        }
    }
    
    if( protocol < 0 )
    {
        send_to_char( "Unknown protocol.\r\n", ch );
        return;
    }
    
    /* Check requirements */
    if( !IS_SET(ch->pcdata->android_schematics, (1 << protocol)) )
    {
        ch_printf( ch, "You haven't discovered the %s Protocol yet.\r\n", protocol_names[protocol] );
        return;
    }
    
    if( ch->pcdata->android_components[protocol] < 5 )
    {
        ch_printf( ch, "%s Protocol requires 5 %s. You have %d.\r\n", 
                  protocol_names[protocol], component_types[protocol], 
                  ch->pcdata->android_components[protocol] );
        return;
    }
    
    if( IS_SET(ch->pcdata->android_installed, (1 << protocol)) )
    {
        ch_printf( ch, "%s Protocol is already installed.\r\n", protocol_names[protocol] );
        return;
    }
    
    /* Install it with epic messaging */
    ch->pcdata->android_components[protocol] -= 5;
    SET_BIT(ch->pcdata->android_installed, (1 << protocol));
    
    send_to_char( "&W*** QUANTUM UPGRADE INSTALLED ***&x\r\n", ch );
    ch_printf( ch, "&C%s Protocol successfully integrated!&x\r\n", protocol_names[protocol] );
    ch_printf( ch, "&W%s&x\r\n", success_messages[protocol] );
    ch_printf( ch, "&GYou can now 'morph android_%s'! Use 'powerdown' or 'revert' to return to normal.&x\r\n", 
              protocol_commands[protocol] );
    
    act( AT_MAGIC, "$n's systems reconfigure as new quantum upgrade protocols are installed!", 
         ch, NULL, NULL, TO_ROOM );
    
    save_char_obj( ch );
}

/* Calculate component drop chance using existing PL ratio logic */
int android_component_drop_chance( CHAR_DATA *android, CHAR_DATA *victim )
{
    long long android_pl, victim_pl;
    double ratio;
    
    if( !android || !victim || IS_NPC(android) || !IS_NPC(victim) )
        return 0;
    
    android_pl = get_power_level(android);
    victim_pl = get_power_level(victim);
    
    /* Use SAME anti-farming logic as existing PL gain system */
    if( victim_pl < (android_pl / 10) )
        return 0;  /* Too weak for components */
    
    /* Use SAME ratio calculation as existing PL system */
    ratio = (double)victim_pl / (double)android_pl;
    
    /* Base 2% with simple difficulty adjustment */
    if( ratio >= 1.0 )  return 3;  /* Equal/stronger enemies = 3% */
    if( ratio >= 0.1 )  return 2;  /* Appropriate enemies = 2% */
    return 1;                      /* Easier enemies = 1% */
}

/* Give component to android based on what they need most */
void give_android_component( CHAR_DATA *android )
{
    int best_component = -1;
    int lowest_count = 6;  /* Higher than max needed */
    
    if( !android || !android->pcdata )
        return;
    
    /* Find component type they need most (lowest count of available schematics) */
    for( int i = 0; i < 6; i++ )
    {
        if( IS_SET(android->pcdata->android_schematics, (1 << i)) )  /* Has schematic */
        {
            if( android->pcdata->android_components[i] < 5 &&  /* Needs more */
                android->pcdata->android_components[i] < lowest_count )  /* Has fewer than current best */
            {
                lowest_count = android->pcdata->android_components[i];
                best_component = i;
            }
        }
    }
    
    if( best_component >= 0 )
    {
        android->pcdata->android_components[best_component]++;
        
        ch_printf( android, "&C[COMPONENT ACQUIRED] Your sensors detect %s in the wreckage!&x\r\n", 
                  component_names[best_component] );
        act( AT_CYAN, "$n's sensors scan the wreckage and extract a technological component.", 
             android, NULL, NULL, TO_ROOM );
        act( AT_CYAN, component_descriptions[best_component], android, NULL, NULL, TO_CHAR );
        
        ch_printf( android, "&YProgress: %d/5 %s&x\r\n", 
                  android->pcdata->android_components[best_component], 
                  component_types[best_component] );
        
        /* Check if they can install now */
        if( android->pcdata->android_components[best_component] == 5 )
        {
            send_to_char( "&G*** INSTALLATION READY ***&x\r\n", android );
            ch_printf( android, "&WYou have collected all components for the %s Protocol!&x\r\n", 
                      protocol_names[best_component] );
            ch_printf( android, "&YUse 'install %s' to activate this transformation!&x\r\n", 
                      protocol_commands[best_component] );
        }
        
        save_char_obj( android );
    }
}

/* Simple Android Component Set Command */
void do_acomp( CHAR_DATA *ch, const char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value, i;
    
    static const char *comp_names[6] = { "nano", "quantum", "plasma", "neural", "fusion", "ethereal" };

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char( "Syntax: aset <player> <component> <amount>\r\n", ch );
        send_to_char( "Components: nano, quantum, plasma, neural, fusion, ethereal (0-5)\r\n", ch );
        send_to_char( "Special: aset <player> all 5 (sets all components to 5)\r\n", ch );
        return;
    }

    if( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if( IS_NPC( victim ) || !IS_ANDROID( victim ) || IS_BIO_ANDROID( victim ) )
    {
        send_to_char( "They are not a pure android.\r\n", ch );
        return;
    }

    value = atoi( arg3 );
    if( value < 0 || value > 5 )
    {
        send_to_char( "Amount must be 0-5.\r\n", ch );
        return;
    }

    /* Handle "all" */
    if( !str_cmp( arg2, "all" ) )
    {
        for( i = 0; i < 6; i++ )
            victim->pcdata->android_components[i] = value;
        victim->pcdata->android_schematics = 63; /* Unlock all */
        ch_printf( ch, "Set all of %s's android components to %d.\r\n", victim->name, value );
        save_char_obj( victim );
        return;
    }

    /* Handle individual components */
    for( i = 0; i < 6; i++ )
    {
        if( !str_cmp( arg2, comp_names[i] ) )
        {
            victim->pcdata->android_components[i] = value;
            SET_BIT( victim->pcdata->android_schematics, (1 << i) ); /* Auto-unlock */
            ch_printf( ch, "Set %s's %s components to %d.\r\n", victim->name, comp_names[i], value );
            save_char_obj( victim );
            return;
        }
    }

    send_to_char( "Invalid component. Use: nano, quantum, plasma, neural, fusion, ethereal, or all.\r\n", ch );
}

/* Android Protocol Spell Functions */
/* Add these to android.c */

/* Aegis Protocol - Divine Shield Transformation */
ch_ret spell_aegis( int sn, int level, CHAR_DATA *ch, void *vo )
{
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only pure androids can access protocol transformations.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( !IS_SET(ch->pcdata->android_installed, (1 << 0)) )
    {
        send_to_char( "You don't have the Aegis Protocol installed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    /* Look up and call the morph */
    MORPH_DATA *morph = get_morph( "android_aegis" );
    if( !morph )
    {
        send_to_char( "Aegis transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    do_morph( ch, morph );
    return rNONE;
}

/* Titan Protocol - Primordial Giant Transformation */
ch_ret spell_titan( int sn, int level, CHAR_DATA *ch, void *vo )
{
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only pure androids can access protocol transformations.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( !IS_SET(ch->pcdata->android_installed, (1 << 1)) )
    {
        send_to_char( "You don't have the Titan Protocol installed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    /* Look up and call the morph */
    MORPH_DATA *morph = get_morph( "android_titan" );
    if( !morph )
    {
        send_to_char( "Titan transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    do_morph( ch, morph );
    return rNONE;
}

/* Valkyrie Protocol - Chosen Warrior Transformation */
ch_ret spell_valkyrie( int sn, int level, CHAR_DATA *ch, void *vo )
{
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only pure androids can access protocol transformations.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( !IS_SET(ch->pcdata->android_installed, (1 << 2)) )
    {
        send_to_char( "You don't have the Valkyrie Protocol installed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    /* Look up and call the morph */
    MORPH_DATA *morph = get_morph( "android_valkyrie" );
    if( !morph )
    {
        send_to_char( "Valkyrie transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    do_morph( ch, morph );
    return rNONE;
}

/* Olympus Protocol - Divine Authority Transformation */
ch_ret spell_olympus( int sn, int level, CHAR_DATA *ch, void *vo )
{
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only pure androids can access protocol transformations.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( !IS_SET(ch->pcdata->android_installed, (1 << 3)) )
    {
        send_to_char( "You don't have the Olympus Protocol installed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    /* Look up and call the morph */
    MORPH_DATA *morph = get_morph( "android_olympus" );
    if( !morph )
    {
        send_to_char( "Olympus transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    do_morph( ch, morph );
    return rNONE;
}

/* Ragnarok Protocol - Twilight of Gods Transformation */
ch_ret spell_ragnarok( int sn, int level, CHAR_DATA *ch, void *vo )
{
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only pure androids can access protocol transformations.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( !IS_SET(ch->pcdata->android_installed, (1 << 4)) )
    {
        send_to_char( "You don't have the Ragnarok Protocol installed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    /* Look up and call the morph */
    MORPH_DATA *morph = get_morph( "android_ragnarok" );
    if( !morph )
    {
        send_to_char( "Ragnarok transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    do_morph( ch, morph );
    return rNONE;
}

/* Excalibur Protocol - Legendary Blade Transformation */
ch_ret spell_excalibur( int sn, int level, CHAR_DATA *ch, void *vo )
{
    if( IS_NPC(ch) || !IS_ANDROID(ch) || IS_BIO_ANDROID(ch) )
    {
        send_to_char( "Only pure androids can access protocol transformations.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    if( !IS_SET(ch->pcdata->android_installed, (1 << 5)) )
    {
        send_to_char( "You don't have the Excalibur Protocol installed.\r\n", ch );
        return rSPELL_FAILED;
    }
    
    /* Look up and call the morph */
    MORPH_DATA *morph = get_morph( "android_excalibur" );
    if( !morph )
    {
        send_to_char( "Excalibur transformation not available.\r\n", ch );
        return rSPELL_FAILED;
    }
    do_morph( ch, morph );
    return rNONE;
}