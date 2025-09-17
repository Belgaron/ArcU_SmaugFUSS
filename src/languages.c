/*
 * LANGUAGE SYSTEM IMPLEMENTATION
 */

#include "mud.h"
#include "languages.h"

/*
 * To modify languages, just edit this table!
 * 
 */
const LANGUAGE_INFO language_table[] = 
{
    // flag,           name,         learnable, racial
    { LANG_COMMON,     "common",     false,     false },  // Everyone knows this
    { LANG_ELDER,      "elder",      true,      true  },  // Eldarian racial
    { LANG_HOLLOW,     "hollow",     true,      true  },  // Hollow racial  
    { LANG_PIXIE,      "pixie",      true,      false },
    { LANG_OGRE,       "ogre",       true,      false },
    { LANG_ORCISH,     "orcish",     true,      false },
    { LANG_TROLLISH,   "trollese",   true,      false },  // Note: "trollese" not "trollish"
    { LANG_RODENT,     "rodent",     true,      false },
    { LANG_INSECTOID,  "insectoid",  true,      false },
    { LANG_MAMMAL,     "mammal",     true,      false },
    { LANG_REPTILE,    "reptile",    true,      false },
    { LANG_DRAGON,     "dragon",     true,      false },
    { LANG_SPIRITUAL,  "spiritual",  true,      false },
    { LANG_MAGICAL,    "magical",    true,      false },
    { LANG_GOBLIN,     "goblin",     true,      false },
    { LANG_GOD,        "god",        true,      false },
    { LANG_ANCIENT,    "ancient",    true,      false },
    { LANG_ECHOSPEAK,  "echospeak",  true,      true  },  // Vampire racial
    { LANG_CLAN,       "clan",       false,     false },  // Special clan language
    { LANG_GITH,       "gith",       true,      false },
    { LANG_LYCAN,      "lycan",      true,      true  },  // Lycan racial
    { LANG_ARCOSIAN,   "arcosian",   true,      true  },  // Arcosian racial
    { LANG_MACHINE,    "machine",    true,      true  },  // Android racial
    { LANG_UNKNOWN,    "",           false,     false }   // Terminator
};

const int max_languages = sizeof(language_table) / sizeof(LANGUAGE_INFO) - 1; // -1 for terminator

const char *const lang_names[] = 
{
    "common", "elder", "hollow", "pixie", "ogre", "orcish", "trollese", 
    "rodent", "insectoid", "mammal", "reptile", "dragon", "spiritual", 
    "magical", "goblin", "god", "ancient", "echospeak", "clan", "gith", 
    "lycan", "arcosian", "machine", ""
};

/*
 * Initialize the language system - call this once at startup
 */
void init_language_system( void )
{
    // Validate that our table is properly set up
    int i;
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( !language_table[i].name || language_table[i].name[0] == '\0' )
        {
            bug( "Language table entry %d has no name!", i );
        }
    }
    log_printf( "Language system initialized with %d languages.", i );
}

/*
 * Get language number by name - replaces the missing function
 */
int get_langnum( const char *name )
{
    int i;
    
    if( !name || name[0] == '\0' )
        return -1;
        
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( !str_cmp( name, language_table[i].name ) )
            return i;
    }
    return -1;
}

/*
 * Get language flag by name - replaces the missing function  
 */
int get_langflag( const char *name )
{
    int i;
    
    if( !name || name[0] == '\0' )
        return LANG_UNKNOWN;
        
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( !str_cmp( name, language_table[i].name ) )
            return language_table[i].flag;
    }
    return LANG_UNKNOWN;
}

/*
 * Get language name by flag
 */
const char *get_langname( int flag )
{
    int i;
    
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( language_table[i].flag == flag )
            return language_table[i].name;
    }
    return "unknown";
}

/*
 * Check if language flag is valid
 */
bool is_valid_language( int flag )
{
    return ( flag & VALID_LANGS ) != 0;
}

/*
 * Check if language can be learned by players
 */
bool is_learnable_language( int flag )
{
    int i;
    
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( language_table[i].flag == flag )
            return language_table[i].learnable;
    }
    return false;
}

/*
 * Count how many languages a character knows
 */
int count_languages( int language_flags )
{
    int count = 0;
    int i;
    
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( language_table[i].flag == LANG_CLAN )
            continue;  // Don't count clan language
            
        if( language_flags & language_table[i].flag )
            count++;
    }
    return count;
}

/*
 * Updated language functions using the new system
 */

// Replace your existing countlangs() function with this:
int countlangs( int languages )
{
    return count_languages( languages );
}

// Update existing functions to use the new system:
void do_speak( CHAR_DATA* ch, const char* argument )
{
    int i;
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if( !str_cmp( arg, "all" ) && IS_IMMORTAL( ch ) )
    {
        set_char_color( AT_SAY, ch );
        ch->speaking = ~LANG_CLAN;
        send_to_char( "Now speaking all languages.\r\n", ch );
        return;
    }
    
    for( i = 0; language_table[i].flag != LANG_UNKNOWN; i++ )
    {
        if( !str_prefix( arg, language_table[i].name ) )
        {
            if( knows_language( ch, language_table[i].flag, ch ) )
            {
                if( language_table[i].flag == LANG_CLAN && ( IS_NPC( ch ) || !ch->pcdata->clan ) )
                    continue;
                    
                ch->speaking = language_table[i].flag;
                set_char_color( AT_SAY, ch );
                ch_printf( ch, "You now speak %s.\r\n", language_table[i].name );
                return;
            }
        }
    }
    
    set_char_color( AT_SAY, ch );
    send_to_char( "You do not know that language.\r\n", ch );
}