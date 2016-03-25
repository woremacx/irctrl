#ifndef __DEBUG_UTIL_H__
#define __DEBUG_UTIL_H__

#define BV(bit)               (1 << bit)
#define set_bit(sfr, bit)     (_SFR_BYTE(sfr) |= BV(bit))  // old sbi()
#define clear_bit(sfr, bit)   (_SFR_BYTE(sfr) &= ~BV(bit)) // old cbi()
#define toggle_bit(sfr, bit)  (_SFR_BYTE(sfr) ^= BV(bit))

#define set_val(sfr, val)     (_SFR_BYTE(sfr) |= val)
#define clear_val(sfr, val)   (_SFR_BYTE(sfr) &= ~val)

#endif /* __DEBUG_UTIL_H__ */
