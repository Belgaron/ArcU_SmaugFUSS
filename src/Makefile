CC      = g++
#PROF    = -p

#Uncomment to compile in Cygwin
CYGWIN = -DCYGWIN

#Uncomment the line below if you are getting undefined references to dlsym, dlopen, and dlclose.
#Comment it out if you get errors about ldl not being found.
NEED_DL = -ldl

#Some systems need this for dynamic linking to work.
EXPORT_SYMBOLS = -export-dynamic

# Uncomment the two lines below if compiling on a Solaris box
#SOLARIS_FLAG = -Dsun -DSYSV
#SOLARIS_LINK = -lnsl -lsocket

#Memory debugging flags. Cygwin lacks libasan support required for this to work.
ifndef CYGWIN
M_FLAGS = -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer
endif

W_FLAGS = -std=c++17 -Wall -Wshadow -Wformat-security -Wpointer-arith -Wcast-align -Wredundant-decls

C_FLAGS = -g2 $(W_FLAGS) $(M_FLAGS) $(SOLARIS_FLAG) $(PROF) $(EXPORT_SYMBOLS)
L_FLAGS = $(M_FLAGS) $(PROF) $(SOLARIS_LINK) -lz $(NEED_DL)

#D_FLAGS : For the DNS Slave process. No need in linking all the extra libs for this.
D_FLAGS = -g2 -O $(M_FLAGS) $(PROF) $(SOLARIS_LINK)

C_FILES = act_comm.c act_info.c act_move.c act_obj.c act_wiz.c ban.c android.c boards.c \
          build.c calendar.c chess.c clans.c color.c combat_messages.c comm.c comments.c const.c custom_slay.c \
                    cyber.c db.c deity.c dns.c fight.c fishing.c grub.c handler.c hashstr.c hint.c hotboot.c \
          house.c imm_host.c interp.c liquids.c magic.c makeobjs.c mapout.c mapper.c mccp.c \
          misc.c mpxset.c mssp.c mud_comm.c mud_prog.c news.c planes.c player.c polymorph.c powerup.c \
          racial_transformations.c renumber.c reset.c save.c services.c sha256.c shops.c skill_training.c skills.c special.c tables.c \
          track.c update.c variables.c weather.c

O_FILES := $(patsubst %.c,o/%.o,$(C_FILES))

H_FILES = $(wildcard *.h)

all:
	$(info    Using C-Flags: $(C_FLAGS))
	$(info    Using L-Flags: $(L_FLAGS))
	$(MAKE) -s smaug
	$(MAKE) -s dns

# pull in dependency info for *existing* .o files
# -include dependencies.d

-include $(O_FILES:.o=.d)

ifdef CYGWIN
smaug: $(O_FILES)
	rm -f smaug.exe
	$(CC) -Wl,--export-all-symbols -o smaug.exe $(O_FILES) $(L_FLAGS)
	@echo "Done compiling mud.";
	chmod g+w smaug.exe
	chmod a+x smaug.exe
	chmod g+w $(O_FILES)

else
smaug: $(O_FILES)
	rm -f smaug
	$(CC) -o smaug $(O_FILES) $(L_FLAGS)
	@echo "Done compiling mud."
endif

clean:
	@rm -f o/*.o smaug smaug.exe dependencies.d resolver resolver.exe resolver.o *~

dns: resolver.o
	rm -f resolver
	$(CC) $(D_FLAGS) -o resolver resolver.o
	@echo "Done compiling DNS resolver.";
	chmod g+w resolver
	chmod a+x resolver
	chmod g+w resolver.o

indent:
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(C_FILES)
	indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l125 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(H_FILES)

indentclean:
	rm *.c~ *.h~

o/%.o: %.c
	echo "  Compiling $@...";
	$(CC) -c $(C_FLAGS) -MMD -MP -MF $(@:.o=.d) $< -o $@
