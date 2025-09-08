/* custom_slay.h - Customizable Slay messages for Arc Unleashed (SmaugFUSS 1.9.x)
 * Lightweight module to load/save user-defined slay styles and expose editing command.
 */
#ifndef CUSTOM_SLAY_H
#define CUSTOM_SLAY_H

typedef struct custom_slay CUSTOM_SLAY;

struct custom_slay
{
   CUSTOM_SLAY *next;
   CUSTOM_SLAY *prev;
   int          number;     /* stable ID/order used in save/load (Snmbr) */

   const char *name;      /* the type players enter after 'slay'  */
   const char *owner;     /* "" = public; otherwise the owner name */
   const char *show_vict; /* message shown to victim               */
   const char *show_char; /* message shown to slayer               */
   const char *show_room; /* message shown to room                 */
};

extern CUSTOM_SLAY *first_cslay, *last_cslay;

/* Boot/load/save */
void boot_custom_slays( void );
void save_custom_slays( void );

/* Gameplay helpers */
void custom_slay_show_list( CHAR_DATA *ch );
bool apply_custom_slay( CHAR_DATA *ch, CHAR_DATA *victim, const char *type );


#endif /* CUSTOM_SLAY_H */
