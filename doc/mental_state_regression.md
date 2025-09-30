# Regression Checklists

These focused checks ensure mental-state modifiers behave predictably after the
changes to poison, confusion-style affects, and sobriety recovery, and that the
ban/allow command pair safely handles wildcard input.

## Mental state regression checks

### Poison application and curing

1. Create or load a mortal test character with the `poison` spell available.
2. Note the character's current mental state via `mstat self` (expect roughly
   `-10`).
3. Cast `poison self` and confirm:
   - The character gains the poison affect (`aff self`).
   - The mental state is at least `10` for PK characters or `20` otherwise.
4. Cast `cure poison self` and verify the poison affect clears and the mental
   state immediately returns to its pre-poison value.

### SMAUG confusion/mania-style effects

1. Using an imm-test character, create a SMAUG spell that applies
   `AFF_POISON` (e.g., through `sset` with `smaug affect poison` data) and set a
   short duration.
2. Cast the custom spell on yourself and confirm your mental state rises to at
   least `30` while the affect is active.
3. Wait for the affect to expire or dispel it and verify the mental state drops
   back to its original baseline automatically.

### Sobriety recovery

## Ban and allow wildcard regression

1. Log in as an immortal capable of banning sites (for example, `trust <name>
   110`).
2. Run: `ban site * all`
   - The command should reject the request with `You must include more than a
     wildcard.`
   - The MUD remains responsive and does not crash.
3. Verify that `ban site` still lists existing entries to ensure the command
   continues functioning after the rejection.
4. Create a temporary ban to remove later (for example,
   `ban site test.example mortal`).
5. Attempt to remove all bans with a wildcard-only argument: `allow site *`
   - The command should respond with `You must include more than a wildcard.`
   - Existing bans are unchanged (`ban site` still shows the entry created in
     step 4).
6. Remove the temporary ban normally using either the ban number or the exact
   hostname to confirm normal operation still works.

1. Raise the character's drunk condition above `8` (e.g., `quaff beer` until the
   condition message indicates intoxication).
2. Observe the mental state climbing toward the positive range while the
   character remains drunk.
3. Allow the drunk condition to tick down to `0` naturally and confirm
   `better_mental_state` is invoked each tick by watching the mental state fall
   back toward `-10` as sobriety returns.
