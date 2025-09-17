/*
 * CONSOLIDATED LANGUAGE SYSTEM
 * 
 */

#ifndef LANGUAGES_H
#define LANGUAGES_H

/*
 * Language bit flags - Use consecutive bits for efficiency
 */
#define LANG_COMMON      BV00   /* Human base language */
#define LANG_ELDER       BV01   /* Eldarian Language */
#define LANG_HOLLOW      BV02   /* Hollow Language */
#define LANG_PIXIE       BV03   /* Pixie/Fairy base language */
#define LANG_OGRE        BV04   /* Ogre base language */
#define LANG_ORCISH      BV05   /* Orc base language */
#define LANG_TROLLISH    BV06   /* Troll base language */
#define LANG_RODENT      BV07   /* Small mammals */
#define LANG_INSECTOID   BV08   /* Insects */
#define LANG_MAMMAL      BV09   /* Larger mammals */
#define LANG_REPTILE     BV10   /* Small reptiles */
#define LANG_DRAGON      BV11   /* Large reptiles, Dragons */
#define LANG_SPIRITUAL   BV12   /* Necromancers or undeads/spectres */
#define LANG_MAGICAL     BV13   /* Spells maybe? Magical creatures */
#define LANG_GOBLIN      BV14   /* Goblin base language */
#define LANG_GOD         BV15   /* Clerics possibly? God creatures */
#define LANG_ANCIENT     BV16   /* Prelude to a glyph read skill? */
#define LANG_ECHOSPEAK   BV17   /* Vampire Language */
#define LANG_CLAN        BV18   /* Clan language */
#define LANG_GITH        BV19   /* Gith Language */
#define LANG_LYCAN       BV20   /* Lycan Language */
#define LANG_ARCOSIAN    BV21   /* Arcosian Language */
#define LANG_MACHINE     BV22   /* Android Language */
#define LANG_UNKNOWN        0   /* Anything that doesn't fit a category */

/*
 * Language structure - consolidates all language info in one place
 */
typedef struct language_info_type
{
    int flag;                   /* The bit flag (LANG_COMMON, etc.) */
    const char *name;           /* The language name ("common", "elder", etc.) */
    bool learnable;             /* Can players learn this language? */
    bool racial;                /* Is this a racial language? */
} LANGUAGE_INFO;

/*
 * Master language table - ALL language info in ONE place
 * To add/remove languages, just modify this table!
 */
extern const LANGUAGE_INFO language_table[];
extern const int max_languages;
extern const char *const lang_names[];

/*
 * Valid languages that players can learn
 */
#define VALID_LANGS    ( LANG_COMMON | LANG_ELDER | LANG_MACHINE | LANG_HOLLOW  \
                       | LANG_OGRE | LANG_ORCISH | LANG_TROLLISH | LANG_GOBLIN \
                       | LANG_SPIRITUAL | LANG_GITH | LANG_ARCOSIAN | LANG_PIXIE \
                       | LANG_RODENT | LANG_INSECTOID | LANG_MAMMAL | LANG_REPTILE \
                       | LANG_DRAGON | LANG_MAGICAL | LANG_GOD | LANG_ANCIENT \
                       | LANG_ECHOSPEAK | LANG_LYCAN )

/*
 * Language system functions - cleaner interface
 */
int get_langnum( const char *name );
int get_langflag( const char *name );
const char *get_langname( int flag );
bool is_valid_language( int flag );
bool is_learnable_language( int flag );
int count_languages( int language_flags );
void init_language_system( void );

#endif /* LANGUAGES_H */