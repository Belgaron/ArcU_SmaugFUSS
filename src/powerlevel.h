/****************************************************************************
 * PowerLevel Class - Modern power level management for SmaugFUSS
 * This replaces the scattered power level variables with a clean class
 ****************************************************************************/

#ifndef POWERLEVEL_H
#define POWERLEVEL_H

class PowerLevel {
private:
    long long base_pl;
    long long current_pl;
    long long logon_pl;
    
    // Configuration constants - removed arbitrary minimum
    static const long long ABSOLUTE_MINIMUM_POWER_LEVEL = 1;  // Can't go below 1
    
public:
    // Constructor - creates a new PowerLevel with starting values
    PowerLevel(long long starting_power = 10)  // Start at 10 for newbie area
        : base_pl(starting_power), current_pl(starting_power), logon_pl(starting_power) {
        // Only prevent completely invalid values
        if (base_pl < ABSOLUTE_MINIMUM_POWER_LEVEL) {
            base_pl = ABSOLUTE_MINIMUM_POWER_LEVEL;
        }
    }
    
    // Getters - functions to read the values
    long long get_base() const { return base_pl; }
    long long get_current() const { return current_pl; }
    long long get_logon() const { return logon_pl; }
    
    // Setters - functions to change the values safely
    void set_base(long long amount) {
        // Only enforce absolute minimum (prevent negative/zero)
        base_pl = (amount < ABSOLUTE_MINIMUM_POWER_LEVEL) ? ABSOLUTE_MINIMUM_POWER_LEVEL : amount;
        recalculate_current();
    }
    
    void add_base(long long amount) {
        set_base(base_pl + amount);
    }
    
    void set_logon() {
        logon_pl = current_pl;
    }
    
    // For compatibility with old exp field
    long long get_exp_equivalent() const {
        return base_pl;
    }
    
private:
    void recalculate_current() {
        // For now, just copy base to current
        // Later we can add transformation multipliers here
        current_pl = base_pl;
    }
};

#endif // POWERLEVEL_H