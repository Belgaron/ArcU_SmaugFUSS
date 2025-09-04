Adds these 2 abilities to mwhere:
1) Use mwhere to list only mobs in the area you are in. mwhere <mob> area
2) Use mwhere to list all mobs in the current area.     mwhere all area

The first one helps get rid of spam if you know what area the mob is in that
you are looking for.  Helpful for finding mobs with common names.

The second is good for area building, you can look and easily see how many
mobs are reset in the area at the current time so you can look and be able
to easily know whether to tell your builders to put some more mobs in there :)

Most of the original mwhere function is there, so thanks to the Rom people for
that code to build on.  It was mainly just copying the chunk and adding a few
if else's in there and changing the conditions slightly on the inside if 
statements.  I figure it is easiest to just copy & paste the whole function
in though then trying to change it little by little, so I'm putting the whole
function in here.  Just replace the function and it's ready to go.  Might want
to make sure you add in the helpfile for mwhere about the 2 additional syntaxes
for it.

If anyone wants to post this on a snippets page feel free to do so with this
header in tact :) I just ask that you e-mail me to let me know.

I also ask that if you use this snippet you e-mail me, I like to know that I
helped someone :)

E-mail address:  zanthras@columbus.rr.com

------------------------------------------------------------------------------
*** in act_wiz.c
*** in function: do_mwhere

Replace the mwhere function with this one:
/*
 * do_mwhere changed by Zanthras of Mystical Realities MUD
 * so you can do "mwhere <mob> area" to look for mobs only
 * in the current area.
 * Also added ability to do "mwhere all area" to list
 * all mobs in the current area.
 */
void do_mwhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    found = FALSE;
    buffer = new_buf();

    if ( arg1[0] == '\0' )
    {
	DESCRIPTOR_DATA *d;

	/* show characters logged */

	for (d = descriptor_list; d != NULL; d = d->next)
	{
	    if (d->character != NULL && d->connected == CON_PLAYING
	    &&  d->character->in_room != NULL && can_see(ch,d->character)
	    &&  can_see_room(ch,d->character->in_room))
	    {
		victim = d->character;
		count++;
		if (d->original != NULL)
		    sprintf(buf,"%3d) %s (in the body of %s) is in %s [%d]\n\r",
			count, d->original->name,victim->short_descr,
			victim->in_room->name,victim->in_room->vnum);
		else
		    sprintf(buf,"%3d) %s is in %s [%d]\n\r",
			count, victim->name,victim->in_room->name,
			victim->in_room->vnum);
		add_buf(buffer,buf);
	    }
	}

        page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
	return;
    }
    else if(arg2[0] == '\0' )
    {
      for ( victim = char_list; victim != NULL; victim = victim->next )
      {
	if ( victim->in_room != NULL
	&&   is_name( arg1, victim->name ) )
	{
	    found = TRUE;
	    count++;
	    sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		IS_NPC(victim) ? victim->short_descr : victim->name,
		victim->in_room->vnum,
		victim->in_room->name );
	    add_buf(buffer,buf);
	}
      }
    }
    else if(!str_cmp(arg1,"all") && !str_cmp(arg2,"area"))
    {
      found = FALSE;
      buffer = new_buf();
      for ( victim = char_list; victim != NULL; victim = victim->next )
      {
	if ( victim->in_room != NULL
        &&   victim->in_room->area == ch->in_room->area
	&&   IS_NPC(victim) )
	{
	    found = TRUE;
	    count++;
	    sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		IS_NPC(victim) ? victim->short_descr : victim->name,
		victim->in_room->vnum,
		victim->in_room->name );
	    add_buf(buffer,buf);
	}
      }
    }
    else if(!str_cmp(arg2,"area"))
    {
      found = FALSE;
      buffer = new_buf();
      for ( victim = char_list; victim != NULL; victim = victim->next )
      {
	if ( victim->in_room != NULL
        &&   victim->in_room->area == ch->in_room->area
	&&   is_name( arg1, victim->name ) )
	{
	    found = TRUE;
	    count++;
	    sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		IS_NPC(victim) ? victim->short_descr : victim->name,
		victim->in_room->vnum,
		victim->in_room->name );
	    add_buf(buffer,buf);
	}
      }
    }



    if ( !found )
    {     
      if ( !str_cmp(arg2,"area") )
         act("You didn't find any $T in this area.", ch, NULL, arg1, TO_CHAR );
      else
	 act("You didn't find any $T.", ch, NULL, arg1, TO_CHAR );
    }
    else
    {
    	page_to_char(buf_string(buffer),ch);
    }
    free_buf(buffer);

    return;
}