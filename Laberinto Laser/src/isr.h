#pragma once

#ifdef IRAM_ATTR
void IRAM_ATTR rf_isr0();
void IRAM_ATTR rf_isr1();
void IRAM_ATTR rf_isr2();
void IRAM_ATTR rf_isr3();
#else
void rf_isr0();
void rf_isr1();
void rf_isr2();
void rf_isr3();
#endif
void handle_rf_isr(int idx);