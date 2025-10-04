/****************************************************************************
 * DBSC 2.5.2 Power Up System Implementation for SmaugFUSS 1.9.8
 * 
 * This implements a tiered power up system where characters can temporarily
 * boost their power level based on their base power level with multipliers.
 * NO EXPERIENCE OR LEVELS - PURE POWER LEVEL SYSTEM
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mud.h"

/****************************************************************************
 * Power Up System Functions - Pure Power Level Based
 ****************************************************************************/

/* Get maximum available power up tier for character based on their power level */
short get_max_powerup_level( CHAR_DATA *ch )
{
    if( !ch || IS_NPC(ch) )
        return 0;
        
    if( get_power_level(ch) >= 5000000 )
        return 7;
    else if( get_power_level(ch) >= 2000000 )
        return 6;
    else if( get_power_level(ch) >= 1000000 )
        return 5;
    else if( get_power_level(ch) >= 500000 )
        return 4;
    else if( get_power_level(ch) >= 250000 )
        return 3;
    else if( get_power_level(ch) >= 100000 )
        return 2;
    else
        return 1;
}

/* Calculate power up multiplier based on character's current power level */
double get_powerup_multiplier( CHAR_DATA *ch, short powerup_tier )
{
    short max_tier;
    
    if( !ch || IS_NPC(ch) || powerup_tier < 1 || powerup_tier > 7 )
        return 1.0;
        
    max_tier = get_max_powerup_level(ch);
    
    /* Can only use tiers up to your maximum */
    if( powerup_tier > max_tier )
        return 1.0;
    
    switch( powerup_tier )
    {
        case 1: return 1.1;
        case 2: return 1.2;
        case 3: return 1.3;
        case 4: return 1.4;
        case 5: return 1.5;
        case 6: return 1.6;
        case 7: return 1.7;
        default: return 1.0;
    }
}

/* Apply power up transformation effects */
void apply_powerup_effects( CHAR_DATA *ch, short powerup_tier )
{
    double multiplier;
    
    if( !ch || IS_NPC(ch) || powerup_tier < 1 || powerup_tier > 7 )
        return;
        
    multiplier = get_powerup_multiplier(ch, powerup_tier);
    if( multiplier <= 1.0 )
        return;
        
    /* Note: PowerLevel class will handle current power level calculation
     * The multiplier effect is applied in get_power_level() function */
    
    /* Visual effects based on race - adjust race checks as needed */
    if( IS_ANDROID(ch) )
    {
        switch( powerup_tier )
        {
            case 1:
                act( AT_WHITE, "$n's mechanical systems hum with increased power.", ch, NULL, NULL, TO_ROOM );
                act( AT_WHITE, "Your systems surge with enhanced power.", ch, NULL, NULL, TO_CHAR );
                break;
            case 2:
                act( AT_YELLOW, "$n's optical sensors glow brighter as power increases.", ch, NULL, NULL, TO_ROOM );
                act( AT_YELLOW, "Your optical sensors brighten as power courses through your circuits.", ch, NULL, NULL, TO_CHAR );
                break;
            case 3:
                act( AT_ORANGE, "$n's frame begins to emit a faint mechanical whirring.", ch, NULL, NULL, TO_ROOM );
                act( AT_ORANGE, "Your internal systems whir as power reaches new levels.", ch, NULL, NULL, TO_CHAR );
                break;
            case 4:
                act( AT_RED, "$n's body sparks with electrical energy!", ch, NULL, NULL, TO_ROOM );
                act( AT_RED, "Electricity crackles through your enhanced systems!", ch, NULL, NULL, TO_CHAR );
                break;
            case 5:
                act( AT_BLUE, "$n's entire frame glows with intense power!", ch, NULL, NULL, TO_ROOM );
                act( AT_BLUE, "Your entire body radiates with intense power!", ch, NULL, NULL, TO_CHAR );
                break;
            case 6:
                act( AT_PURPLE, "$n's form becomes wreathed in energy fields!", ch, NULL, NULL, TO_ROOM );
                act( AT_PURPLE, "Energy fields crackle around your enhanced form!", ch, NULL, NULL, TO_CHAR );
                break;
            case 7:
                act( AT_WHITE, "$n ascends to maximum power, energy radiating in all directions!", ch, NULL, NULL, TO_ROOM );
                act( AT_WHITE, "You achieve maximum power! Energy radiates from every circuit!", ch, NULL, NULL, TO_CHAR );
                break;
        }
    }
    else  /* Organic races */
    {
        switch( powerup_tier )
        {
            case 1:
                act( AT_WHITE, "$n steadies $s breath as a faint {A}aura{x} gathers around $m.", ch, NULL, NULL, TO_ROOM );
                act( AT_WHITE, "You steady your breath as a faint {A}aura{x} gathers around you.", ch, NULL, NULL, TO_CHAR );
                break;
            case 2:
                act( AT_YELLOW, "A low hum builds around $n as $s {A}aura{x} tightens.", ch, NULL, NULL, TO_ROOM );
                act( AT_YELLOW, "Your heartbeat drums; your {A}aura{x} tightens and hums against your skin.", ch, NULL, NULL, TO_CHAR );
                break;
            case 3:
                act( AT_ORANGE, "The air prickles; dust skitters as $n’s {A}aura{x} snaps and lifts $s hair.", ch, NULL, NULL, TO_ROOM );
                act( AT_ORANGE, "{A}Power sparks{x} to life! Dust skitters and your hair lifts in the current.", ch, NULL, NULL, TO_CHAR );
                break;
            case 4:
                act( AT_RED, "A pressure ripple bursts from $n! $S {A}aura flares{x} and hairline cracks spread underfoot.", ch, NULL, NULL, TO_ROOM );
                act( AT_RED, "A pressure wave ripples out as your {A}aura flares{x}, cracking the ground.", ch, NULL, NULL, TO_CHAR );
                break;
            case 5:
                act( AT_BLUE, "$n’s {A}aura roars{x} to life, hurling bright &Yarcs&D while the air bends with heat.", ch, NULL, NULL, TO_ROOM );
                act( AT_BLUE, "Your {A}aura roars{x}, throwing off bright &Yarcs&D as the air warps with heat.", ch, NULL, NULL, TO_CHAR );
                break;
            case 6:
                act( AT_PURPLE, "Power clamps around $n; a tight halo snaps into place and an outward {A}pulse{x} kicks a ring of dust from $s feet.", ch, NULL, NULL, TO_ROOM );
                act( AT_PURPLE, "Power locks in; a tight halo snaps into place and an outward {A}pulse{x} kicks a ring of dust from your feet.", ch, NULL, NULL, TO_CHAR );
                break;
            case 7:
                act( AT_WHITE, "$n explodes past $s limit -- the floor shudders as a {A}shockwave{x} rips through the room and light burns at $s edges.", ch, NULL, NULL, TO_ROOM );
                act( AT_WHITE, "You blow past your limit -- the floor shudders, wind tears past in a {A}shockwave{x}, and light burns at your edges.", ch, NULL, NULL, TO_CHAR );
                break;
        }
    }
    
    ch->powerup = powerup_tier;
    
    /* Apply to split forms if this character has them */
    /* apply_powerup_to_splits(ch, powerup_tier); - Implement this based on your split system */
}

/* Apply power up to split forms (placeholder - implement based on your split system) */
void apply_powerup_to_splits( CHAR_DATA *ch, short powerup_tier )
{
    /* This function should iterate through any split forms and apply
     * the same power up level to them. Implementation depends on how
     * your split system works. Example framework:
     *
     * CHAR_DATA *split;
     * for( split = first_split_of(ch); split; split = next_split_of(ch) )
     * {
     *     if( split != ch )
     *         apply_powerup_effects(split, powerup_tier);
     * }
     */
}

typedef struct powerup_aura_desc POWERUP_AURA_DESC;

struct powerup_aura_desc
{
    const char *self;
    const char *other;
};

static const POWERUP_AURA_DESC powerup_aura_table[] =
{
    { NULL, NULL },
    { "A faint aura flickers around you.",
      "%s is wrapped in a faint, shimmering aura." },
    { "Your aura hums and tightens against your skin.",
      "%s's aura hums and tightens in the air." },
    { "Power crackles around you, stirring dust at your feet.",
      "Power crackles around %s, stirring dust across the ground." },
    { "A pressure wave ripples from you as your aura flares.",
      "A pressure wave ripples from %s as the surrounding aura flares." },
    { "Blazing arcs of energy roar outward from your aura.",
      "Blazing arcs of energy roar outward from %s's aura." },
    { "A tight halo of power snaps into place and kicks up a dust ring.",
      "A tight halo of power snaps around %s, kicking a ring of dust outward." },
    { "Light burns at your edges as shockwaves tear through the air.",
      "Light burns at %s's edges while shockwaves tear through the air." }
};

void show_powerup_aura_to_char( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char aura_buf[MAX_STRING_LENGTH];
    short tier, max_tier_index;
    const POWERUP_AURA_DESC *desc;

    if( !victim || !ch )
        return;

    if( victim->powerup <= 0 )
        return;

    max_tier_index = (short)( sizeof( powerup_aura_table ) / sizeof( powerup_aura_table[0] ) ) - 1;
    tier = URANGE( 1, victim->powerup, max_tier_index );
    desc = &powerup_aura_table[tier];

    if( !desc->self || !desc->other )
        return;

    if( ch == victim )
        strlcpy( aura_buf, desc->self, sizeof( aura_buf ) );
    else
        snprintf( aura_buf, sizeof( aura_buf ), desc->other, PERS( victim, ch ) );

    ch_printf_color( ch, "%s%s&D\r\n", get_energy_color_token( victim ), aura_buf );
}

/* Remove transformation stat bonuses (placeholder) */
void transStatRemove( CHAR_DATA *ch )
{
    /* This function should remove any stat bonuses applied by transformations
     * Implementation depends on your transformation system
     * Basic example: */

    /* affect_strip(ch, gsn_powerup); - Remove powerup affects if you use affects */
    /* Remove any other transformation bonuses here */
}

/****************************************************************************
 * Command Functions
 ****************************************************************************/

void do_powerup( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    short current_tier, max_tier, target_tier;
        
    if( IS_NPC(ch) )
    {
        send_to_char( "NPCs cannot power up.\r\n", ch );
        return;
    }
    
    one_argument( argument, arg );
    
    current_tier = ch->powerup;
    max_tier = get_max_powerup_level(ch);
    
    if( max_tier == 0 )
    {
        send_to_char( "You don't have enough power level to power up.\r\n", ch );
        return;
    }
    
    /* Determine target tier */
    if( arg[0] == '\0' )
    {
        target_tier = current_tier + 1;  /* No argument = go up one tier */
    }
    else if( !str_cmp( arg, "max" ) )
    {
        target_tier = max_tier;  /* "powerup max" = go to max tier */
    }
    else
    {
        send_to_char( "Usage: 'powerup' or 'powerup max'\r\n", ch );
        return;
    }
    
    if( target_tier > max_tier )
    {
        ch_printf( ch, "You are already at maximum power tier (%d)!\r\n", max_tier );
        return;
    }
    
    if( target_tier <= current_tier )
    {
        ch_printf( ch, "You are already at tier %d.\r\n", current_tier );
        return;
    }
    
    apply_powerup_effects(ch, target_tier);
    
}

void do_powerdown( CHAR_DATA *ch, const char *argument )
{
    short current_tier;
    bool had_powerup = FALSE;
    bool had_morph = FALSE;
        
    if( IS_NPC(ch) )
    {
        send_to_char( "NPCs cannot power down.\r\n", ch );
        return;
    }
    
    current_tier = ch->powerup;
    
    // Check what the character has active
    if( current_tier > 0 )
        had_powerup = TRUE;
    if( ch->morph )
        had_morph = TRUE;
    
    // If they have neither, inform them
    if( !had_powerup && !had_morph )
    {
        send_to_char( "You are not powered up or transformed.\r\n", ch );
        return;
    }
    
    // Handle transformation removal
    if( had_morph )
    {
        // Remove morph transformation
        do_unmorph_char( ch );
        send_to_char( "You change back into your basic form.\r\n", ch );
        act( AT_MAGIC, "$n changes back into $s basic form.", ch, NULL, NULL, TO_ROOM );
        return;
    }
    
    // Handle powerup tier reduction
    if( had_powerup )
    {
        short new_tier = current_tier - 1;
        
        /* Visual effect based on race */
        if( IS_ANDROID(ch) )  // Use your existing IS_ANDROID macro
        {
            if( new_tier == 0 )
            {
                act( AT_CYAN, "$n's systems power down with a mechanical whir.", ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN, "Your systems power down to normal levels.", ch, NULL, NULL, TO_CHAR );
            }
            else
            {
                act( AT_CYAN, "$n's systems reduce power output.", ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN, "Your systems reduce power to a lower level.", ch, NULL, NULL, TO_CHAR );
            }
        }
        else  // Organic races
        {
            if( new_tier == 0 )
            {
                act( AT_CYAN, "$n's aura fades as $e returns to normal power levels.", ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN, "Your aura fades as you return to your base power level.", ch, NULL, NULL, TO_CHAR );
            }
            else
            {
                act( AT_CYAN, "$n's aura dims as $e reduces $s power.", ch, NULL, NULL, TO_ROOM );
                act( AT_CYAN, "Your aura dims as you reduce your power level.", ch, NULL, NULL, TO_CHAR );
            }
        }
        
        /* Set new power tier */
        ch->powerup = new_tier;
        
        ch_printf( ch, "Power level reduced to tier %d.\r\n", new_tier );
        
        /* Apply to split forms if you have that system */
        /* apply_powerup_to_splits(ch, new_tier); */
    }
}